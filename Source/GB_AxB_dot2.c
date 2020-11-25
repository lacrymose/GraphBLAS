//------------------------------------------------------------------------------
// GB_AxB_dot2: compute C=A'*B or C<!M>=A'*B in parallel, in-place
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// TODO: rename to GB_bitmap_AxB_dot.c

//------------------------------------------------------------------------------

// The C<M>=A'*B dot product when C is sparse is computed by GB_AxB_dot3.
// This method always constructs C as bitmap.

#include "GB_mxm.h"
#include "GB_subref.h"
#include "GB_binop.h"
#ifndef GBCOMPACT
#include "GB_AxB__include.h"
#endif

#define GB_FREE_ALL                                             \
{                                                               \
    GB_Matrix_free (&M2) ;                                      \
    GB_FREE (A_slice) ;                                         \
    GB_FREE (B_slice) ;                                         \
}

GB_PUBLIC   // accessed by the MATLAB tests in GraphBLAS/Test only
GrB_Info GB_AxB_dot2                // C=A'*B or C<!M>=A'*B, dot product method
(
    GrB_Matrix *Chandle,            // output matrix
    const GrB_Matrix M_in,          // mask matrix for C<!M>=A'*B, may be NULL
    const bool Mask_comp,           // if true, use !M
    const bool Mask_struct,         // if true, use the only structure of M
    const GrB_Matrix A_in,          // input matrix
    const GrB_Matrix B_in,          // input matrix
    const GrB_Semiring semiring,    // semiring that defines C=A*B
    const bool flipxy,              // if true, do z=fmult(b,a) vs fmult(a,b)
    GB_Context Context
)
{
double ttt = omp_get_wtime ( ) ;

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;

    ASSERT (Chandle != NULL) ;
    ASSERT (*Chandle == NULL) ;
    ASSERT_MATRIX_OK_OR_NULL (M_in, "M for dot A'*B", GB0) ;
    ASSERT_MATRIX_OK (A_in, "A for dot A'*B", GB0) ;
    ASSERT_MATRIX_OK (B_in, "B for dot A'*B", GB0) ;

    ASSERT (!GB_ZOMBIES (M_in)) ;
    ASSERT (GB_JUMBLED_OK (M_in)) ;
    ASSERT (!GB_PENDING (M_in)) ;
    ASSERT (!GB_ZOMBIES (A_in)) ;
    ASSERT (!GB_JUMBLED (A_in)) ;
    ASSERT (!GB_PENDING (A_in)) ;
    ASSERT (!GB_ZOMBIES (B_in)) ;
    ASSERT (!GB_JUMBLED (B_in)) ;
    ASSERT (!GB_PENDING (B_in)) ;

    ASSERT_SEMIRING_OK (semiring, "semiring for numeric A'*B", GB0) ;
    ASSERT (A_in->vlen == B_in->vlen) ;

    (*Chandle) = NULL ;

    //--------------------------------------------------------------------------
    // construct shallow copies of A and B, if hypersparse
    //--------------------------------------------------------------------------

    // If A_in is hypersparse, a new sparse matrix A is constructed with
    // A->vdim = A_in->nvec and the same vlen as A_in, and then the packed
    // C->vlen will equal A->vdim < cvlen_final.

    // If B_in is hypersparse, a new sparse matrix B is constructed with
    // B->vdim = B_in->nvec and the same vlen as B_in, and then the packed
    // C->vdim will equal B->vdim < cvdim_final.

    int64_t cvlen_final = A_in->vdim ;
    int64_t cvdim_final = B_in->vdim ;
    bool A_is_hyper = GB_IS_HYPERSPARSE (A_in) ;
    bool B_is_hyper = GB_IS_HYPERSPARSE (B_in) ;
    bool A_or_B_hyper = A_is_hyper || B_is_hyper ;
    GrB_Index *GB_RESTRICT Ah = A_in->h ;
    GrB_Index *GB_RESTRICT Bh = B_in->h ;
    struct GB_Matrix_opaque A_header, B_header ;
    GrB_Matrix A = (A_is_hyper) ? GB_hyper_pack (&A_header, A_in) : A_in ;
    GrB_Matrix B = (B_is_hyper) ? GB_hyper_pack (&B_header, B_in) : B_in ;
    ASSERT (!GB_IS_HYPERSPARSE (A)) ;
    ASSERT (!GB_IS_HYPERSPARSE (B)) ;

    //--------------------------------------------------------------------------
    // determine the size of C
    //--------------------------------------------------------------------------

    int64_t *GB_RESTRICT A_slice = NULL ;
    int64_t *GB_RESTRICT B_slice = NULL ;
    int64_t cnvec = B->nvec ;
    int64_t cvlen = A->vdim ;
    int64_t cvdim = B->vdim ;

    int64_t cnz ;
    if (!GB_Index_multiply ((GrB_Index *) (&cnz), cvlen, cvdim))
    {
        // problem too large
        return (GrB_OUT_OF_MEMORY) ;
    }

    //--------------------------------------------------------------------------
    // extract the submask if A or B are hypersparse 
    //--------------------------------------------------------------------------

    GrB_Matrix M, M2 = NULL ;
    if (A_or_B_hyper && M_in != NULL)
    {
        // M2 = M_in (Ah, Bh)
        GB_OK (GB_subref (&M2, M_in->is_csc, M_in,
            (A_is_hyper) ? Ah : GrB_ALL, cvlen,
            (B_is_hyper) ? Bh : GrB_ALL, cvdim, false, Context)) ;
        // TODO: if Mask_struct is true, only extract the pattern of M_in
        M = M2 ;
        ASSERT_MATRIX_OK_OR_NULL (M, "M submask dot A'*B", GB0) ;
    }
    else
    {
        // use the mask as-is
        M = M_in ;
    }

    //--------------------------------------------------------------------------
    // determine the number of threads to use
    //--------------------------------------------------------------------------

    int64_t naslice = 0 ;
    int64_t nbslice = 0 ;

    int64_t anvec = A->nvec ;
    int64_t anz   = GB_NNZ_HELD (A) ;

    int64_t bnvec = B->nvec ;
    int64_t bnz   = GB_NNZ_HELD (B) ;

    GB_GET_NTHREADS_MAX (nthreads_max, chunk, Context) ;
    int nthreads = GB_nthreads (anz + bnz, chunk, nthreads_max) ;

    #define GB_NTASKS_PER_THREAD 32

    if (nthreads == 1)
    { 
        // do the entire computation with a single thread
        naslice = 1 ;
        nbslice = 1 ;
    }
    else
    {
        // determine number of slices for A' and B
        if (bnvec == 1)
        { 
            // C and B are single vectors
            naslice = GB_NTASKS_PER_THREAD * nthreads ;
            nbslice = 1 ;
        }
        else if (anvec == 1 || bnvec == 0
            || bnvec > GB_NTASKS_PER_THREAD * nthreads)
        { 
            // A is a single vector, or B is empty, or B is large: just slice B
            naslice = 1 ;
            nbslice = GB_NTASKS_PER_THREAD * nthreads ;
        }
        else
        { 
            // slice B into individual vectors
            nbslice = bnvec ;

            // slice A' to get a total of about 16*nthreads tasks
            naslice = (GB_NTASKS_PER_THREAD * nthreads) / nbslice ;

            // but do not slice A too finely
            naslice = GB_IMIN (naslice, anvec/4) ;
            naslice = GB_IMAX (naslice, nthreads) ;
        }
    }

    //--------------------------------------------------------------------------
    // get the semiring operators
    //--------------------------------------------------------------------------

    GrB_BinaryOp mult = semiring->multiply ;
    GrB_Monoid add = semiring->add ;
    ASSERT (mult->ztype == add->op->ztype) ;
    bool A_is_pattern, B_is_pattern ;
    GB_AxB_pattern (&A_is_pattern, &B_is_pattern, flipxy, mult->opcode) ;

    //--------------------------------------------------------------------------
    // allocate workspace and slice A and B
    //--------------------------------------------------------------------------

    // A and B can have any sparsity: full, bitmap, sparse, or hypersparse.
    // C is always created as bitmap

    if (!GB_pslice (&A_slice, A->p, A->nvec, naslice, false) ||
        !GB_pslice (&B_slice, B->p, B->nvec, nbslice, false))
    { 
        // out of memory
        GB_FREE_ALL ;
        return (GrB_OUT_OF_MEMORY) ;
    }

ttt = omp_get_wtime ( ) - ttt ;
GB_Global_timing_add (17, ttt) ;
ttt = omp_get_wtime ( ) ;

    //--------------------------------------------------------------------------
    // allocate C
    //--------------------------------------------------------------------------

    // if M is sparse/hyper, then calloc C->b; otherwise use malloc
    bool C_bitmap_calloc = (M != NULL) &&
        (GB_IS_SPARSE (M) || GB_IS_HYPERSPARSE (M)) ;
    GrB_Type ctype = add->op->ztype ;
    GB_OK (GB_new_bix (Chandle, // bitmap, new header
        ctype, cvlen, cvdim, GB_Ap_malloc, true,
        GxB_BITMAP, C_bitmap_calloc, B->hyper_switch, cnvec, cnz, true,
        Context)) ;
    GrB_Matrix C = (*Chandle) ;

ttt = omp_get_wtime ( ) - ttt ;
GB_Global_timing_add (18, ttt) ;
ttt = omp_get_wtime ( ) ;

    //--------------------------------------------------------------------------
    // TODO: if M is sparse, scatter it into the C bitmap
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // C<#>=A'*B, computing each entry with a dot product, via builtin semiring
    //--------------------------------------------------------------------------

    bool done = false ;

    #ifndef GBCOMPACT

        //----------------------------------------------------------------------
        // define the worker for the switch factory
        //----------------------------------------------------------------------

        #define GB_Adot2B(add,mult,xname) GB_Adot2B_ ## add ## mult ## xname

        #define GB_AxB_WORKER(add,mult,xname)                                \
        {                                                                    \
            info = GB_Adot2B (add,mult,xname) (C, M, Mask_comp, Mask_struct, \
                A, A_is_pattern, A_slice, B, B_is_pattern, B_slice,          \
                nthreads, naslice, nbslice) ;                                \
            done = (info != GrB_NO_VALUE) ;                                  \
        }                                                                    \
        break ;

        //----------------------------------------------------------------------
        // launch the switch factory
        //----------------------------------------------------------------------

        GB_Opcode mult_opcode, add_opcode ;
        GB_Type_code xcode, ycode, zcode ;

        if (GB_AxB_semiring_builtin (A, A_is_pattern, B, B_is_pattern, semiring,
            flipxy, &mult_opcode, &add_opcode, &xcode, &ycode, &zcode))
        { 
            #include "GB_AxB_factory.c"
        }
        ASSERT (info == GrB_SUCCESS || info == GrB_NO_VALUE) ;

    #endif

    //--------------------------------------------------------------------------
    // C = A'*B, computing each entry with a dot product, with typecasting
    //--------------------------------------------------------------------------

    if (!done)
    { 
        #define GB_DOT2_GENERIC
        GB_BURBLE_MATRIX (C, "(generic C%s=A'*B) ", (M == NULL) ? "" :
            (Mask_comp ? "<!M>" : "<M>")) ;
        #include "GB_AxB_dot_generic.c"
    }

    //--------------------------------------------------------------------------
    // free workspace
    //--------------------------------------------------------------------------

    GB_FREE_ALL ;
    C->magic = GB_MAGIC ;
    ASSERT_MATRIX_OK (C, "dot2: C = A'*B output", GB0) ;

    //--------------------------------------------------------------------------
    // unpack C if A or B are hypersparse
    //--------------------------------------------------------------------------

    if (A_or_B_hyper)
    {

        //----------------------------------------------------------------------
        // unpack C from bitmap to sparse/hyper
        //----------------------------------------------------------------------

        // C is currently A_in->nvec by B_in->nvec, in bitmap form.  It must be
        // unpacked into sparse/hypersparse form, with zombies.

        //----------------------------------------------------------------------
        // allocate the sparse/hypersparse structure of the final C
        //----------------------------------------------------------------------

        int64_t *GB_RESTRICT Cp = GB_MALLOC (cvdim+1, int64_t) ;
        int64_t *GB_RESTRICT Ch =
            B_is_hyper ? GB_MALLOC (cvdim, int64_t) : NULL ;
        int64_t *GB_RESTRICT Ci = GB_MALLOC (cnz, int64_t) ;
        if (Cp == NULL || (B_is_hyper && Ch == NULL) || Ci == NULL)
        { 
            // out of memory
            GB_Matrix_free (Chandle) ;
            GB_FREE (Cp) ;
            GB_FREE (Ch) ;
            GB_FREE (Ci) ;
            return (GrB_OUT_OF_MEMORY) ;
        }

        //----------------------------------------------------------------------
        // construct the hyperlist of C, if B is hypersparse
        //----------------------------------------------------------------------

        nthreads = GB_nthreads (cvdim, chunk, nthreads_max) ;
        if (B_is_hyper)
        { 
            // C becomes hypersparse
            ASSERT (cvdim == B_in->nvec) ;
            GB_memcpy (Ch, B_in->h, cvdim * sizeof (int64_t), nthreads) ;
        }

        //----------------------------------------------------------------------
        // construct the vector pointers of C
        //----------------------------------------------------------------------

        int64_t pC ;
        #pragma omp parallel for num_threads(nthreads) schedule(static)
        for (pC = 0 ; pC < cvdim+1 ; pC++)
        { 
            Cp [pC] = pC * cvlen ;
        }

        //----------------------------------------------------------------------
        // construct the pattern of C from its bitmap
        //----------------------------------------------------------------------

        // C(i,j) becomes a zombie if not present in the bitmap
        nthreads = GB_nthreads (cnz, chunk, nthreads_max) ;

        int8_t *GB_RESTRICT Cb = C->b ;
        if (A_is_hyper)
        { 
            ASSERT (cvlen == A_in->nvec) ;
            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (pC = 0 ; pC < cnz ; pC++)
            {
                int64_t i = Ah [pC % cvlen] ;
                Ci [pC] = (Cb [pC]) ? i : GB_FLIP (i) ;
            }
        }
        else
        { 
            ASSERT (cvlen == cvlen_final && cvlen == A->vdim) ;
            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (pC = 0 ; pC < cnz ; pC++)
            {
                int64_t i = pC % cvlen ;
                Ci [pC] = (Cb [pC]) ? i : GB_FLIP (i) ;
            }
        }

        //----------------------------------------------------------------------
        // transplant the new content and finalize C
        //----------------------------------------------------------------------

        C->p = Cp ; Cp = NULL ;
        C->h = Ch ; Ch = NULL ;
        C->i = Ci ; Ci = NULL ;
        C->nzombies = cnz - C->nvals ;
        C->vdim = cvdim_final ;
        C->vlen = cvlen_final ;
        C->nvals = -1 ;
        C->nvec = cvdim ;
        C->plen = cvdim ;
        C->nvec_nonempty = (cvlen == 0) ? 0 : cvdim ;

        // free the bitmap
        GB_FREE (C->b) ;

        // C is now sparse or hypersparse
        ASSERT_MATRIX_OK (C, "dot2: unpacked C", GB0) ;
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT (*Chandle == C) ;
    ASSERT (GB_ZOMBIES_OK (C)) ;
    ASSERT (!GB_JUMBLED (C)) ;
    ASSERT (!GB_PENDING (C)) ;

ttt = omp_get_wtime ( ) - ttt ;
GB_Global_timing_add (19, ttt) ;
ttt = omp_get_wtime ( ) ;

    return (GrB_SUCCESS) ;
}

