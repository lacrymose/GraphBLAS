//------------------------------------------------------------------------------
// GB_bitmap_assign_M_accum_whole:  assign to C bitmap 
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
// C<M> += A       assign
// C<M> += A       subassign

// C<M,repl> += A       assign
// C<M,repl> += A       subassign
//------------------------------------------------------------------------------

// C:           bitmap
// M:           present, hypersparse or sparse (not bitmap or full)
// Mask_comp:   false
// Mask_struct: true or false
// C_replace:   true or false
// accum:       present
// A:           matrix (hyper, sparse, bitmap, or full), or scalar
// kind:        assign or subassign (same action)

#include "GB_bitmap_assign_methods.h"

#define GB_FREE_ALL \
    GB_ek_slice_free (&pstart_Mslice, &kfirst_Mslice, &klast_Mslice) ;

GrB_Info GB_bitmap_assign_M_accum_whole
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    // inputs:
    const bool C_replace,       // descriptor for C
    const GrB_Matrix M,         // mask matrix, which is not NULL here
//  const bool Mask_comp,       // false here
    const bool Mask_struct,     // true if M is structural, false if valued
    const GrB_BinaryOp accum,   // present here
    const GrB_Matrix A,         // input matrix, not transposed
    const void *scalar,         // input scalar
    const GrB_Type scalar_type, // type of input scalar
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GBURBLE_BITMAP_ASSIGN ("bit3:whole", M, false, accum,
        GB_ALL, GB_ALL, GB_ASSIGN) ;
    ASSERT (GB_IS_HYPERSPARSE (M) || GB_IS_SPARSE (M)) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign, M, accum", GB0) ;
    ASSERT_MATRIX_OK (M, "M for bitmap assign, M, accum", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (A, "A for bitmap assign, M, accum", GB0) ;

    //--------------------------------------------------------------------------
    // get C, M, A, and accum
    //--------------------------------------------------------------------------

    GB_GET_C_BITMAP ;           // C must be bitmap
    GB_SLICE_M
    GB_GET_A
    GB_GET_ACCUM

    //--------------------------------------------------------------------------
    // do the assignment
    //--------------------------------------------------------------------------

    if (A == NULL)
    { 

        //----------------------------------------------------------------------
        // scalar assignment: C<M, replace or !replace> += scalar
        //----------------------------------------------------------------------

        if (C_replace)
        { 

            //------------------------------------------------------------------
            // C<M,replace> += scalar
            //------------------------------------------------------------------

            // Cb [pC] += 2 for each entry M(i,j) in the mask
            GB_bitmap_M_scatter_whole (C,
                M, Mask_struct, GB_BITMAP_M_SCATTER_PLUS_2,
                pstart_Mslice, kfirst_Mslice, klast_Mslice,
                mthreads, mtasks, Context) ;
            // the bitmap of C now contains:
            //  Cb (i,j) = 0:   cij not present, mij zero
            //  Cb (i,j) = 1:   cij present, mij zero
            //  Cb (i,j) = 2:   cij not present, mij 1
            //  Cb (i,j) = 3:   cij present, mij 1

            #undef  GB_CIJ_WORK
            #define GB_CIJ_WORK(pC)                                 \
            {                                                       \
                switch (Cb [pC])                                    \
                {                                                   \
                    case 1: /* C(i,j) present, M(i,j) = 0 */        \
                        /* delete this entry */                     \
                        Cb [pC] = 0 ;                               \
                        cnvals-- ;                                  \
                        break ;                                     \
                    case 2: /* C(i,j) not present, M(i,j) = 1 */    \
                        /* Cx [pC] = scalar */                      \
                        GB_ASSIGN_SCALAR (pC) ;                     \
                        Cb [pC] = 1 ;                               \
                        cnvals++ ;                                  \
                        break ;                                     \
                    case 3: /* C(i,j) present, M(i,j) = 1 */        \
                        /* Cx [pC] += scalar */                     \
                        GB_ACCUM_SCALAR (pC) ;                      \
                        Cb [pC] = 1 ;                               \
                        break ;                                     \
                    default: ;                                      \
                }                                                   \
            }
            #include "GB_bitmap_assign_C_whole_template.c"

        }
        else
        { 

            //------------------------------------------------------------------
            // C<M> += scalar
            //------------------------------------------------------------------

            #undef  GB_MASK_WORK
            #define GB_MASK_WORK(pC)                        \
            {                                               \
                if (Cb [pC])                                \
                {                                           \
                    /* C(i,j) present, M(i,j) = 1 */        \
                    /* Cx [pC] += scalar */                 \
                    GB_ACCUM_SCALAR (pC) ;                  \
                }                                           \
                else                                        \
                {                                           \
                    /* C(i,j) not present, M(i,j) = 1 */    \
                    /* Cx [pC] = scalar */                  \
                    GB_ASSIGN_SCALAR (pC) ;                 \
                    Cb [pC] = 1 ;                           \
                    cnvals++ ;                              \
                }                                           \
            }
            #include "GB_bitmap_assign_M_all_template.c"
        }

    }
    else
    { 

        //----------------------------------------------------------------------
        // matrix assignment: C<M, replace or !replace> += A
        //----------------------------------------------------------------------

        if (GB_IS_BITMAP (A) || GB_IS_FULL (A))
        {

            //------------------------------------------------------------------
            // C<M, replace or !replace> += A where A is bitmap or full
            //------------------------------------------------------------------

            if (C_replace)
            {

                //--------------------------------------------------------------
                // C<M, replace> += A where A is bitmap or full
                //--------------------------------------------------------------

                // Cb [pC] += 2 for each entry M(i,j) in the mask
                GB_bitmap_M_scatter_whole (C,
                    M, Mask_struct, GB_BITMAP_M_SCATTER_PLUS_2,
                    pstart_Mslice, kfirst_Mslice, klast_Mslice,
                    mthreads, mtasks, Context) ;
                // the bitmap of C now contains:
                //  Cb (i,j) = 0:   cij not present, mij zero
                //  Cb (i,j) = 1:   cij present, mij zero
                //  Cb (i,j) = 2:   cij not present, mij 1
                //  Cb (i,j) = 3:   cij present, mij 1

                #undef  GB_CIJ_WORK
                #define GB_CIJ_WORK(pC)                                 \
                {                                                       \
                    switch (Cb [pC])                                    \
                    {                                                   \
                        case 1: /* C(i,j) present, M(i,j) = 0 */        \
                            /* delete this entry */                     \
                            Cb [pC] = 0 ;                               \
                            cnvals-- ;                                  \
                            break ;                                     \
                        case 2: /* C(i,j) not present, M(i,j) = 1 */    \
                            if (GBB (Ab, pC))                           \
                            {                                           \
                                /* Cx [pC] = Ax [pC] */                 \
                                GB_ASSIGN_AIJ (pC, pC) ;                \
                                Cb [pC] = 1 ;                           \
                                cnvals++ ;                              \
                            }                                           \
                            else                                        \
                            {                                           \
                                /* clear the mask from C */             \
                                Cb [pC] = 0 ;                           \
                            }                                           \
                            break ;                                     \
                        case 3: /* C(i,j) present, M(i,j) = 1 */        \
                            if (GBB (Ab, pC))                           \
                            {                                           \
                                /* Cx [pC] += Ax [pC] */                \
                                GB_ACCUM_AIJ (pC, pC) ;                 \
                            }                                           \
                            Cb [pC] = 1 ;                               \
                            break ;                                     \
                        default: ;                                      \
                    }                                                   \
                }
                #include "GB_bitmap_assign_C_whole_template.c"

            }
            else
            {

                //--------------------------------------------------------------
                // C<M> += A where A is bitmap or full
                //--------------------------------------------------------------

                #undef  GB_MASK_WORK
                #define GB_MASK_WORK(pC)                            \
                {                                                   \
                    if (GBB (Ab, pC))                               \
                    {                                               \
                        /* A(i,j) is present */                     \
                        if (Cb [pC])                                \
                        {                                           \
                            /* C(i,j) present, M(i,j) = 1 */        \
                            /* Cx [pC] += Ax [pC] */                \
                            GB_ACCUM_AIJ (pC, pC) ;                 \
                        }                                           \
                        else                                        \
                        {                                           \
                            /* C(i,j) not present, M(i,j) = 1 */    \
                            /* Cx [pC] = Ax [pC] */                 \
                            GB_ASSIGN_AIJ (pC, pC) ;                \
                            Cb [pC] = 1 ;                           \
                            cnvals++ ;                              \
                        }                                           \
                    }                                               \
                }
                #include "GB_bitmap_assign_M_all_template.c"

            }
        }
        else
        {

            //------------------------------------------------------------------
            // C<M, replace or !replace> += A where A is sparse or hyper
            //------------------------------------------------------------------

            // Cb [pC] += 2 for each entry M(i,j) in the mask
            GB_bitmap_M_scatter_whole (C,
                M, Mask_struct, GB_BITMAP_M_SCATTER_PLUS_2,
                pstart_Mslice, kfirst_Mslice, klast_Mslice,
                mthreads, mtasks, Context) ;
            // the bitmap of C now contains:
            //  Cb (i,j) = 0:   cij not present, mij zero
            //  Cb (i,j) = 1:   cij present, mij zero
            //  Cb (i,j) = 2:   cij not present, mij 1
            //  Cb (i,j) = 3:   cij present, mij 1

            // assign or accumulate entries from A into C
            #undef  GB_AIJ_WORK
            #define GB_AIJ_WORK(pC,pA)                      \
            {                                               \
                /* A(i,j) is present */                     \
                int8_t cb = Cb [pC] ;                       \
                if (cb == 2)                                \
                {                                           \
                    /* C(i,j) not present, M(i,j) = 1 */    \
                    /* Cx [pC] = Ax [pA] */                 \
                    GB_ASSIGN_AIJ (pC, pA) ;                \
                    Cb [pC] = 3 ;                           \
                    cnvals++ ;                              \
                }                                           \
                else if (cb == 3)                           \
                {                                           \
                    /* C(i,j) present, M(i,j) = 1 */        \
                    /* Cx [pC] += Ax [pA] */                \
                    GB_ACCUM_AIJ (pC, pA) ;                 \
                }                                           \
            }
            #include "GB_bitmap_assign_A_whole_template.c"

            if (C_replace)
            { 
                // clear the mask and delete entries not assigned
                #undef  GB_CIJ_WORK
                #define GB_CIJ_WORK(pC)                 \
                {                                       \
                    int8_t cb = Cb [pC] ;               \
                    Cb [pC] = (cb == 3) ;               \
                    cnvals -= (cb == 1) ;               \
                }
                #include "GB_bitmap_assign_C_whole_template.c"
            }
            else
            { 
                // clear the mask
                // Cb [pC] -= 2 for each entry M(i,j) in the mask
                GB_bitmap_M_scatter_whole (C,
                    M, Mask_struct, GB_BITMAP_M_SCATTER_MINUS_2,
                    pstart_Mslice, kfirst_Mslice, klast_Mslice,
                    mthreads, mtasks, Context) ;
            }
        }
    }

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    C->nvals = cnvals ;
    GB_FREE_ALL ;
    ASSERT_MATRIX_OK (C, "final C for bitmap assign, M, accum, whole", GB0) ;
    return (GrB_SUCCESS) ;
}
