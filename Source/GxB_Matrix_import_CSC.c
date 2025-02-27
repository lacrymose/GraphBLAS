//------------------------------------------------------------------------------
// GxB_Matrix_import_CSC: import a matrix in CSC format
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_export.h"

GrB_Info GxB_Matrix_import_CSC      // import a CSC matrix
(
    GrB_Matrix *A,      // handle of matrix to create
    GrB_Type type,      // type of matrix to create
    GrB_Index nrows,    // number of rows of the matrix
    GrB_Index ncols,    // number of columns of the matrix

    GrB_Index **Ap,     // column "pointers"
    GrB_Index **Ai,     // row indices
    void **Ax,          // values
    GrB_Index Ap_size,  // size of Ap in bytes
    GrB_Index Ai_size,  // size of Ai in bytes
    GrB_Index Ax_size,  // size of Ax in bytes
    bool iso,           // if true, A is iso

    bool jumbled,       // if true, indices in each column may be unsorted
    const GrB_Descriptor desc
)
{ 

    //--------------------------------------------------------------------------
    // check inputs and get the descriptor
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Matrix_import_CSC (&A, type, nrows, ncols, "
        "&Ap, &Ai, &Ax, Ap_size, Ai_size, Ax_size, iso, "
        "jumbled, desc)") ;
    GB_BURBLE_START ("GxB_Matrix_import_CSC") ;
    GB_GET_DESCRIPTOR (info, desc, xx1, xx2, xx3, xx4, xx5, xx6, xx7) ;
    GB_GET_DESCRIPTOR_IMPORT (desc, fast_import) ;

    //--------------------------------------------------------------------------
    // import the matrix
    //--------------------------------------------------------------------------

    info = GB_import (false, A, type, nrows, ncols, false,
        Ap,   Ap_size,  // Ap
        NULL, 0,        // Ah
        NULL, 0,        // Ab
        Ai,   Ai_size,  // Ai
        Ax,   Ax_size,  // Ax
        0, jumbled, 0,                      // jumbled or not
        GxB_SPARSE, true,                   // sparse by col
        iso, fast_import, true, Context) ;

    GB_BURBLE_END ;
    return (info) ;
}

