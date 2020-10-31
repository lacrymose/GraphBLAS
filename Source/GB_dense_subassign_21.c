//------------------------------------------------------------------------------
// GB_dense_subassign_21: C(:,:) = x where x is a scalar
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// C(:,:) = x where C is a matrix and x is a scalar
// C can have any sparsity on input; it is recreated as a full matrix.

#include "GB_dense.h"
#include "GB_select.h"
#include "GB_Pending.h"

GrB_Info GB_dense_subassign_21      // C(:,:) = x; C is a matrix and x a scalar
(
    GrB_Matrix C,                   // input/output matrix
    const void *scalar,             // input scalar
    const GrB_Type atype,           // type of the input scalar
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT_MATRIX_OK (C, "C for C(:,:)=x", GB0) ;
    ASSERT (scalar != NULL) ;
    // any prior pending tuples are discarded, and all zombies will be killed,
    // so C can be anything on input.
    ASSERT (GB_ZOMBIES_OK (C)) ;
    ASSERT (GB_JUMBLED_OK (C)) ;
    ASSERT (GB_PENDING_OK (C)) ;
    ASSERT_TYPE_OK (atype, "atype for C(:,:)=x", GB0) ;

    //--------------------------------------------------------------------------
    // determine the number of threads to use
    //--------------------------------------------------------------------------

    int64_t cvdim = C->vdim ;
    int64_t cvlen = C->vlen ;
    GrB_Index cnzmax ;
    bool ok = GB_Index_multiply (&cnzmax, cvlen, cvdim) ;
    if (!ok)
    { 
        // problem too large
        return (GrB_OUT_OF_MEMORY) ;
    }

    GB_GET_NTHREADS_MAX (nthreads_max, chunk, Context) ;
    int nthreads = GB_nthreads (cnzmax, chunk, nthreads_max) ;

    //--------------------------------------------------------------------------
    // typecast the scalar into the same type as C
    //--------------------------------------------------------------------------

    int64_t csize = C->type->size ;
    GB_cast_function
        cast_A_to_C = GB_cast_factory (C->type->code, atype->code) ;
    GB_void cwork [GB_VLA(csize)] ;
    cast_A_to_C (cwork, scalar, atype->size) ;

    //--------------------------------------------------------------------------
    // ensure C is a full matrix
    //--------------------------------------------------------------------------

    // discard any prior pending tuples
    GB_Pending_free (&(C->Pending)) ;

    if (!GB_IS_FULL (C))
    {
        // clear prior content and recreate it; use exising header for C.
        GB_phbix_free (C) ;
        int C_sparsity = C->sparsity ;  // save the sparsity control of C
        info = GB_new_bix (&C,  // full, old header
            C->type, cvlen, cvdim, GB_Ap_null, C->is_csc,
            GxB_FULL, C->hyper_switch, -1, cnzmax, true, Context) ;
        if (info != GrB_SUCCESS)
        { 
            // out of memory
            return (GrB_OUT_OF_MEMORY) ;
        }
        C->magic = GB_MAGIC ;
        C->nvec_nonempty = (cvlen == 0) ? 0 : cvdim ;
        C->sparsity = C_sparsity ;      // restore the sparsity control of C
    }

    //--------------------------------------------------------------------------
    // parallel memset if the scalar is zero
    //--------------------------------------------------------------------------

    if (!GB_is_nonzero (cwork, csize))
    { 
        // set all of C->x to zero
        GB_memset (C->x, 0, cnzmax * csize, nthreads) ;
        ASSERT_MATRIX_OK (C, "C(:,:)=0 output", GB0) ;
        ASSERT (GB_IS_FULL (C)) ;
        ASSERT (!GB_ZOMBIES (C)) ;
        ASSERT (!GB_JUMBLED (C)) ;
        ASSERT (!GB_PENDING (C)) ;
        return (GrB_SUCCESS) ;
    }

    //--------------------------------------------------------------------------
    // define the worker for the switch factory
    //--------------------------------------------------------------------------

    int64_t pC ;

    // worker for built-in types
    #define GB_WORKER(ctype)                                                \
    {                                                                       \
        ctype *GB_RESTRICT Cx = (ctype *) C->x ;                            \
        ctype x = (*(ctype *) cwork) ;                                      \
        GB_PRAGMA (omp parallel for num_threads(nthreads) schedule(static)) \
        for (pC = 0 ; pC < cnzmax ; pC++)                                   \
        {                                                                   \
            Cx [pC] = x ;                                                   \
        }                                                                   \
    }                                                                       \
    break ;

    //--------------------------------------------------------------------------
    // launch the switch factory
    //--------------------------------------------------------------------------

    switch (C->type->code)
    {
        case GB_BOOL_code   : GB_WORKER (bool) ;
        case GB_INT8_code   : GB_WORKER (int8_t) ;
        case GB_INT16_code  : GB_WORKER (int16_t) ;
        case GB_INT32_code  : GB_WORKER (int32_t) ;
        case GB_INT64_code  : GB_WORKER (int64_t) ;
        case GB_UINT8_code  : GB_WORKER (uint8_t) ;
        case GB_UINT16_code : GB_WORKER (uint16_t) ;
        case GB_UINT32_code : GB_WORKER (uint32_t) ;
        case GB_UINT64_code : GB_WORKER (uint64_t) ;
        case GB_FP32_code   : GB_WORKER (float) ;
        case GB_FP64_code   : GB_WORKER (double) ;
        case GB_FC32_code   : GB_WORKER (GxB_FC32_t) ;
        case GB_FC64_code   : GB_WORKER (GxB_FC64_t) ;
        default:
            {
                // worker for all user-defined types
                GB_BURBLE_N (cnzmax, "(generic C(:,:)=x assign) ") ;
                GB_void *GB_RESTRICT Cx = (GB_void *) C->x ;
                #pragma omp parallel for num_threads(nthreads) schedule(static)
                for (pC = 0 ; pC < cnzmax ; pC++)
                { 
                    memcpy (Cx +((pC)*csize), cwork, csize) ;
                }
            }
            break ;
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (C, "C(:,:)=x output", GB0) ;
    ASSERT (GB_IS_FULL (C)) ;
    ASSERT (!GB_ZOMBIES (C)) ;
    ASSERT (!GB_JUMBLED (C)) ;
    ASSERT (!GB_PENDING (C)) ;
    return (GrB_SUCCESS) ;
}

