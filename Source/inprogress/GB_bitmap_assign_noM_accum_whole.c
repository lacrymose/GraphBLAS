//------------------------------------------------------------------------------
// GB_bitmap_assign_noM_accum_whole:  assign to C bitmap, mask M is not present
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
// C<> += A            assign
// C<> += A            subassign

// C<repl> += A        assign
// C<repl> += A        subassign

// C<!> += A           assign: no work to do
// C<!> += A           subassign: no work to do

// C<!,repl> += A      assign: just clear C(I,J) of all entries
// C<!,repl> += A      subassign: just clear C(I,J) of all entries
//------------------------------------------------------------------------------

// C:           bitmap
// M:           none
// Mask_comp:   true or false
// Mask_struct: true or false
// C_replace:   true or false
// accum:       present
// A:           matrix (hyper, sparse, bitmap, or full), or scalar
// kind:        assign or subassign (same action)

// If Mask_comp is true, then an empty mask is complemented.  This case has
// already been handled by GB_assign_prep, which calls
// GB_bitmap_assign_noM_noaccum, with a scalar (which is unused).

#include "GB_bitmap_assign_methods.h"

#define GB_FREE_ALL ;

GrB_Info GB_bitmap_assign_noM_accum_whole
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    // inputs:
    const bool C_replace,       // descriptor for C
//  const GrB_Matrix M,         // mask matrix, not present here
    const bool Mask_comp,       // true for !M, false for M
    const bool Mask_struct,     // true if M is structural, false if valued
    const GrB_BinaryOp accum,   // present
    const GrB_Matrix A,         // input matrix, not transposed
    const void *scalar,         // input scalar
    const GrB_Type scalar_type, // type of input scalar
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GBURBLE_BITMAP_ASSIGN ("bit5:whole", NULL, Mask_comp, accum,
        GB_ALL, GB_ALL, GB_ASSIGN) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign, no M, accum", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (A, "A for bitmap assign, no M, accum", GB0) ;

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    GB_GET_C_BITMAP ;           // C must be bitmap
    GB_GET_A
    GB_GET_ACCUM

    //--------------------------------------------------------------------------
    // do the assignment
    //--------------------------------------------------------------------------

    if (!Mask_comp)
    {

        //----------------------------------------------------------------------
        // C(I,J) += A or += scalar
        //----------------------------------------------------------------------

        if (A == NULL)
        { 

            //------------------------------------------------------------------
            // scalar assignment: C(I,J) += scalar
            //------------------------------------------------------------------

            // for all entries in IxJ
            #define GB_IXJ_WORK(pC,ignore)          \
            {                                       \
                int8_t cb = Cb [pC] ;               \
                if (cb == 0)                        \
                {                                   \
                    /* Cx [pC] = scalar */          \
                    GB_ASSIGN_SCALAR (pC) ;         \
                    Cb [pC] = 1 ;                   \
                    cnvals++ ;                      \
                }                                   \
                else                                \
                {                                   \
                    /* Cx [pC] += scalar */         \
                    GB_ACCUM_SCALAR (pC) ;          \
                }                                   \
            }
            #include "GB_bitmap_assign_IxJ_template.c"

        }
        else
        { 

            //------------------------------------------------------------------
            // matrix assignment: C(I,J) += A
            //------------------------------------------------------------------

            // for all entries aij in A (A hyper, sparse, bitmap, or full)
            //        if Cb(p) == 0
            //            Cx(p) = aij
            //            Cb(p) = 1       // C(iC,jC) is now present, insert
            //        else // if Cb(p) == 1:
            //            Cx(p) += aij    // C(iC,jC) still present, updated
            //            cnvals++

            #define GB_AIJ_WORK(pC,pA)              \
            {                                       \
                int8_t cb = Cb [pC] ;               \
                if (cb == 0)                        \
                {                                   \
                    /* Cx [pC] = Ax [pA] */         \
                    GB_ASSIGN_AIJ (pC, pA) ;        \
                    Cb [pC] = 1 ;                   \
                    cnvals++ ;                      \
                }                                   \
                else                                \
                {                                   \
                    /* Cx [pC] += Ax [pA] */        \
                    GB_ACCUM_AIJ (pC, pA) ;         \
                }                                   \
            }
            #include "GB_bitmap_assign_A_template.c"
        }

    }

#if 0
    else if (C_replace)
    {

        //----------------------------------------------------------------------
        // This case is handled by GB_assign_prep and is thus not needed here.
        //----------------------------------------------------------------------

        // mask not present yet complemented: C_replace phase only

        // for row assign: for all entries in C(i,:)
        // for col assign: for all entries in C(:,j)
        // for assign: for all entries in C(:,:)
        // for subassign: for all entries in C(I,J)
        //      M not present; so effective value of the mask is mij==0
        //      set Cb(p) = 0

        #define GB_CIJ_WORK(pC)         \
        {                               \
            int8_t cb = Cb [pC] ;       \
            Cb [pC] = 0 ;               \
            cnvals -= (cb == 1) ;       \
        }
        #include "GB_bitmap_assign_C_template.c"
    }
#endif

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    C->nvals = cnvals ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign, no M, accum", GB0) ;
    return (GrB_SUCCESS) ;
}
