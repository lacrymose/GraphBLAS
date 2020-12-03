//------------------------------------------------------------------------------
// GB_bitmap_assign_noM_noaccum_whole:  assign to C bitmap
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
// C<> = A             assign
// C<> = A             subassign

// C<repl> = A         assign
// C<repl> = A         subassign

// C<!> = A            assign
// C<!> = A            subassign

// C<!,repl> = A       assign
// C<!,repl> = A       subassign
//------------------------------------------------------------------------------

// C:           bitmap
// M:           none
// Mask_comp:   true or false
// Mask_struct: true or false (ignored)
// C_replace:   true or false
// accum:       not present
// A:           matrix (hyper, sparse, bitmap, or full), or scalar
// kind:        assign or subassign (same action)

// If M is not present and Mask_comp is true, then an empty mask is
// complemented.  This case is been handled by GB_assign_prep by calling this
// method with no matrix A, but with a scalar (which is unused).

#include "GB_bitmap_assign_methods.h"

#define GB_FREE_ALL ;

GrB_Info GB_bitmap_assign_noM_noaccum_whole
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    // inputs:
    const bool C_replace,       // descriptor for C
//  const GrB_Matrix M,         // mask matrix, not present here
    const bool Mask_comp,       // true for !M, false for M
    const bool Mask_struct,     // true if M is structural, false if valued
//  const GrB_BinaryOp accum,   // not present
    const GrB_Matrix A,         // input matrix, not transposed
    const void *scalar,         // input scalar
    const GrB_Type scalar_type, // type of input scalar
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GBURBLE_BITMAP_ASSIGN ("bit6:whole", NULL, Mask_comp, NULL,
        GB_ALL, GB_ALL, GB_ASSIGN) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign: noM, noaccum", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (A, "A for bitmap assign: noM, noaccum", GB0) ;

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    GB_GET_C_BITMAP ;           // C must be bitmap
    GB_GET_A

    //--------------------------------------------------------------------------
    // C_replace phase
    //--------------------------------------------------------------------------

    if (C_replace)
    { 
        // for row assign: set Cb(i,:) to zero
        // for col assign: set Cb(:,j) to zero
        // for assign: set all Cb(:,:) to zero
        // for subassign: set all Cb(I,J) to zero
        #define GB_CIJ_WORK(pC)                 \
        {                                       \
            int8_t cb = Cb [pC] ;               \
            Cb [pC] = 0 ;                       \
            cnvals -= (cb == 1) ;               \
        }
        #include "GB_bitmap_assign_C_template.c"
    }

    //--------------------------------------------------------------------------
    // assignment phase
    //--------------------------------------------------------------------------

    if (!Mask_comp)
    {

        if (A == NULL)
        { 

            //------------------------------------------------------------------
            // scalar assignment: C(I,J) = scalar
            //------------------------------------------------------------------

            // for all IxJ
            #undef  GB_IXJ_WORK
            #define GB_IXJ_WORK(pC,ignore)          \
            {                                       \
                int8_t cb = Cb [pC] ;               \
                /* Cx [pC] = scalar */              \
                GB_ASSIGN_SCALAR (pC) ;             \
                Cb [pC] = 1 ;                       \
                cnvals += (cb == 0) ;               \
            }
            #include "GB_bitmap_assign_IxJ_template.c"

        }
        else
        { 

            //------------------------------------------------------------------
            // matrix assignment: C(I,J) = A
            //------------------------------------------------------------------

            if (!C_replace)
            { 
                // delete all entries in C(I,J)
                #undef  GB_IXJ_WORK
                #define GB_IXJ_WORK(pC,ignore)          \
                {                                       \
                    int8_t cb = Cb [pC] ;               \
                    Cb [pC] = 0 ;                       \
                    cnvals -= (cb == 1) ;               \
                }
                #include "GB_bitmap_assign_IxJ_template.c"
            }

            // for all entries aij in A (A hyper, sparse, bitmap, or full)
            //      Cx(p) = aij     // C(iC,jC) inserted or updated
            //      Cb(p) = 1

            #define GB_AIJ_WORK(pC,pA)              \
            {                                       \
                int8_t cb = Cb [pC] ;               \
                /* Cx [pC] = Ax [pA] */             \
                GB_ASSIGN_AIJ (pC, pA) ;            \
                Cb [pC] = 1 ;                       \
            }
            #include "GB_bitmap_assign_A_template.c"

            cnvals += GB_NNZ (A) ;
        }
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    C->nvals = cnvals ;
    ASSERT_MATRIX_OK (C, "final C for bitmap assign: noM, noaccum", GB0) ;
    return (GrB_SUCCESS) ;
}
