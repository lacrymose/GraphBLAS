//------------------------------------------------------------------------------
// GxB_Vector_subassign: w(Rows)<M> = accum (w(Rows),u)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Compare with GrB_Vector_assign, which uses M and C_replace differently

#include "GB_subassign.h"
#include "GB_get_mask.h"

GrB_Info GxB_Vector_subassign       // w(Rows)<M> = accum (w(Rows),u)
(
    GrB_Vector w,                   // input/output matrix for results
    const GrB_Vector M_in,          // optional mask for w(Rows), unused if NULL
    const GrB_BinaryOp accum,       // optional accum for z=accum(w(Rows),t)
    const GrB_Vector u,             // first input:  vector u
    const GrB_Index *Rows,          // row indices
    GrB_Index nRows,                // number of row indices
    const GrB_Descriptor desc       // descriptor for w(Rows) and M
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE (w, "GxB_Vector_subassign (w, M, accum, u, Rows, nRows, desc)") ;
    GB_BURBLE_START ("GxB_subassign") ;
    GB_RETURN_IF_NULL_OR_FAULTY (w) ;
    GB_RETURN_IF_FAULTY (M_in) ;
    GB_RETURN_IF_NULL_OR_FAULTY (u) ;

    ASSERT (GB_VECTOR_OK (w)) ;
    ASSERT (M_in == NULL || GB_VECTOR_OK (M_in)) ;
    ASSERT (GB_VECTOR_OK (u)) ;

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,
        xx1, xx2, xx3, xx7) ;

    // get the mask
    GrB_Matrix M = GB_get_mask ((GrB_Matrix) M_in, &Mask_comp, &Mask_struct) ;

    //--------------------------------------------------------------------------
    // w(Rows)<M> = accum (w(Rows), u) and variations
    //--------------------------------------------------------------------------

    info = GB_subassign (
        (GrB_Matrix) w,     C_replace,  // w vector and its descriptor
        M, Mask_comp, Mask_struct,      // mask and its descriptor
        false,                          // do not transpose the mask
        accum,                          // for accum (C(Rows,:),A)
        (GrB_Matrix) u,     false,      // u as a matrix; never transposed
        Rows, nRows,                    // row indices
        GrB_ALL, 1,                     // all column indices
        false, NULL, GB_ignore_code,    // no scalar expansion
        Context) ;

    GB_BURBLE_END ;
    return (info) ;
}

