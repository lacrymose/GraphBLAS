//------------------------------------------------------------------------------
// gb_export_to_mxstruct: export a GrB_Matrix to a MATLAB struct
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// The input GrB_Matrix A is exported to a GraphBLAS matrix struct G, and freed.

// The input GrB_Matrix A must be deep.  The output is a MATLAB struct
// holding the content of the GrB_Matrix.

#include "gb_matlab.h"

// for hypersparse, sparse, or full matrices
static const char *MatrixFields [6] =
{
    "GraphBLASv4",      // 0: "logical", "int8", ... "double",
                        //    "single complex", or "double complex"
    "s",                // 1: all scalar info goes here
    "x",                // 2: array of uint8, size (sizeof(type) * nzmax)
    "p",                // 3: array of int64_t, size plen+1
    "i",                // 4: array of int64_t, size nzmax
    "h"                 // 5: array of int64_t, size plen if hypersparse
} ;

// for bitmap matrices only
static const char *Bitmap_MatrixFields [4] =
{
    "GraphBLASv4",      // 0: "logical", "int8", ... "double",
                        //    "single complex", or "double complex"
    "s",                // 1: all scalar info goes here
    "x",                // 2: array of uint8, size (sizeof(type) * nzmax)
    "b"                 // 3: array of int8_t, size nzmax, for bitmap only
} ;

//------------------------------------------------------------------------------

mxArray *gb_export_to_mxstruct  // return exported MATLAB struct G
(
    GrB_Matrix *A_handle        // matrix to export; freed on output
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    CHECK_ERROR (A_handle == NULL, "matrix missing") ;

    GrB_Matrix T = NULL ;
    if (GB_is_shallow (*A_handle))
    {
        // A is shallow so make a deep copy
        // TODO:: do this in GxB*export*
        OK (GrB_Matrix_dup (&T, *A_handle)) ;
        OK (GrB_Matrix_free (A_handle)) ;
        (*A_handle) = T ;
    }

    GrB_Matrix A = (*A_handle) ;

    //--------------------------------------------------------------------------
    // make sure the matrix is finished
    //--------------------------------------------------------------------------

    // TODO: this is done in GxB*export* and can be removed here
    OK1 (A, GrB_Matrix_wait (&A)) ;

    //--------------------------------------------------------------------------
    // get the sparsity_status and CSR/CSC format
    //--------------------------------------------------------------------------

    GxB_Format_Value fmt ;
    int sparsity_status, sparsity_control ;
    OK (GxB_Matrix_Option_get (A, GxB_SPARSITY_STATUS,  &sparsity_status)) ;
    OK (GxB_Matrix_Option_get (A, GxB_SPARSITY_CONTROL, &sparsity_control)) ;
    OK (GxB_Matrix_Option_get (A, GxB_FORMAT, &fmt)) ;

    //--------------------------------------------------------------------------
    // extract the opaque content not provided by GxB*export
    //--------------------------------------------------------------------------

    // TODO: this content is opaque, try to remove it here
    int64_t nzmax = A->nzmax ;
    int64_t plen = A->plen ;
    int64_t nvec_nonempty = A->nvec_nonempty ;

    //--------------------------------------------------------------------------
    // extract the content of the GrB_Matrix and free it
    //--------------------------------------------------------------------------

    size_t type_size = 0 ;
    GrB_Type type = NULL ;
    GrB_Index nrows = 0, ncols = 0 ;
    int8_t *Ab = NULL ;
    int64_t *Ap = NULL, *Ah = NULL, *Ai = NULL ;
    void *Ax = NULL ;
    int64_t Ap_size = 0, Ah_size = 0, Ab_size = 0, Ai_size = 0, Ax_size = 0 ;
    int64_t nvals = 0, nvec = 0 ;
    bool by_col = (fmt == GxB_BY_COL) ;

    // TODO write GxB_Matrix_export which exports the matrix as-is

    switch (sparsity_status)
    {
        case GxB_FULL :
            if (by_col)
            {
                OK (GxB_Matrix_export_FullC (&A, &type, &nrows, &ncols,
                    &Ax, &Ax_size, NULL)) ;
            }
            else
            {
                OK (GxB_Matrix_export_FullR (&A, &type, &nrows, &ncols,
                    &Ax, &Ax_size, NULL)) ;
            }
            break ;

        case GxB_SPARSE :
            if (by_col)
            {
                OK (GxB_Matrix_export_CSC (&A, &type, &nrows, &ncols,
                    &Ap, &Ai, &Ax, &Ap_size, &Ai_size, &Ax_size, NULL, NULL)) ;
            }
            else
            {
                OK (GxB_Matrix_export_CSR (&A, &type, &nrows, &ncols,
                    &Ap, &Ai, &Ax, &Ap_size, &Ai_size, &Ax_size, NULL, NULL)) ;
            }
            break ;

        case GxB_HYPERSPARSE :
            if (by_col)
            {
                OK (GxB_Matrix_export_HyperCSC (&A, &type, &nrows, &ncols,
                    &Ap, &Ah, &Ai, &Ax, &Ap_size, &Ah_size, &Ai_size, &Ax_size,
                    &nvec, NULL, NULL)) ;
            }
            else
            {
                OK (GxB_Matrix_export_HyperCSR (&A, &type, &nrows, &ncols,
                    &Ap, &Ah, &Ai, &Ax, &Ap_size, &Ah_size, &Ai_size, &Ax_size,
                    &nvec, NULL, NULL)) ;
            }
            break ;

        case GxB_BITMAP :
            if (by_col)
            {
                OK (GxB_Matrix_export_BitmapC (&A, &type, &nrows, &ncols,
                    &Ab, &Ax, &Ab_size, &Ax_size, &nvals, NULL)) ;
            }
            else
            {
                OK (GxB_Matrix_export_BitmapR (&A, &type, &nrows, &ncols,
                    &Ab, &Ax, &Ab_size, &Ax_size, &nvals, NULL)) ;
            }
            break ;

        default: ;
    }

    OK (GxB_Type_size (&type_size, type)) ;

    //--------------------------------------------------------------------------
    // construct the output struct
    //--------------------------------------------------------------------------

    mxArray *G ;
    switch (sparsity_status)
    {
        case GxB_FULL :
            // A is full, with 3 fields: GraphBLASv4, s, x
            G = mxCreateStructMatrix (1, 1, 3, MatrixFields) ;
            break ;

        case GxB_SPARSE :
            // A is sparse, with 5 fields: GraphBLASv4, s, x, p, i
            G = mxCreateStructMatrix (1, 1, 5, MatrixFields) ;
            break ;

        case GxB_HYPERSPARSE :
            // A is hypersparse, with 6 fields: GraphBLASv4, s, x, p, i, h
            G = mxCreateStructMatrix (1, 1, 6, MatrixFields) ;
            break ;

        case GxB_BITMAP :
            // A is bitmap, with 4 fields: GraphBLASv4, s, x, b
            G = mxCreateStructMatrix (1, 1, 4, Bitmap_MatrixFields) ;
            break ;

        default : ERROR ("invalid GraphBLAS struct") ;
    }

    //--------------------------------------------------------------------------
    // export content into the output struct
    //--------------------------------------------------------------------------

    // export the GraphBLAS type as a string
    mxSetFieldByNumber (G, 0, 0, gb_type_to_mxstring (type)) ;

    // export the scalar content
    mxArray *opaque = mxCreateNumericMatrix (1, 9, mxINT64_CLASS, mxREAL) ;
    int64_t *s = mxGetInt64s (opaque) ;
    s [0] = plen ;
    s [1] = (by_col) ? nrows : ncols ;  // was A->vlen ;
    s [2] = (by_col) ? ncols : nrows ;  // was A->vdim ;
    s [3] = (sparsity_status == GxB_HYPERSPARSE) ? nvec : (s [2]) ;
    s [4] = nvec_nonempty ;
    s [5] = sparsity_control ;
    s [6] = (int64_t) by_col ;
    s [7] = nzmax ;
    s [8] = nvals ;
    mxSetFieldByNumber (G, 0, 1, opaque) ;

    // These components do not need to be exported: Pending, nzombies,
    // queue_next, queue_head, enqueued, *_shallow, jumbled, logger,
    // hyper_switch, bitmap_switch.

    if (sparsity_status == GxB_SPARSE || sparsity_status == GxB_HYPERSPARSE)
    {
        // export the pointers
        mxArray *Ap_mx = mxCreateNumericMatrix (1, 0, mxINT64_CLASS, mxREAL) ;
        mxSetN (Ap_mx, Ap_size) ;
        void *p = mxGetInt64s (Ap_mx) ; gb_mxfree (&p) ;
        mxSetInt64s (Ap_mx, Ap) ;
        mxSetFieldByNumber (G, 0, 3, Ap_mx) ;

        // export the indices
        mxArray *Ai_mx = mxCreateNumericMatrix (1, 0, mxINT64_CLASS, mxREAL) ;
        if (Ai_size > 0)
        { 
            mxSetN (Ai_mx, Ai_size) ;
            p = mxGetInt64s (Ai_mx) ; gb_mxfree (&p) ;
            mxSetInt64s (Ai_mx, Ai) ;
        }
        mxSetFieldByNumber (G, 0, 4, Ai_mx) ;
    }

    // export the values as uint8
    mxArray *Ax_mx = mxCreateNumericMatrix (1, 0, mxUINT8_CLASS, mxREAL) ;
    if (Ax_size > 0)
    { 
        mxSetN (Ax_mx, Ax_size * type_size) ;
        void *p = mxGetUint8s (Ax_mx) ; gb_mxfree (&p) ;
        mxSetUint8s (Ax_mx, Ax) ;
    }
    mxSetFieldByNumber (G, 0, 2, Ax_mx) ;

    if (sparsity_status == GxB_HYPERSPARSE)
    {
        // export the hyperlist
        mxArray *Ah_mx = mxCreateNumericMatrix (1, 0, mxINT64_CLASS, mxREAL) ;
        if (Ah_size > nvec)
        {
            memset (Ah + nvec, 0, (Ah_size - nvec) * sizeof (int64_t)) ;
        }
        if (Ah_size > 0)
        { 
            mxSetN (Ah_mx, Ah_size) ;
            void *p = mxGetInt64s (Ah_mx) ; gb_mxfree (&p) ;
            mxSetInt64s (Ah_mx, Ah) ;
        }
        mxSetFieldByNumber (G, 0, 5, Ah_mx) ;
    }

    if (sparsity_status == GxB_BITMAP)
    { 
        // export the bitmap
        mxArray *Ab_mx = mxCreateNumericMatrix (1, 0, mxINT8_CLASS, mxREAL) ;
        if (Ab_size > 0)
        { 
            mxSetN (Ab_mx, Ab_size) ;
            void *p = mxGetInt8s (Ab_mx) ; gb_mxfree (&p) ;
            mxSetInt8s (Ab_mx, Ab) ;
        }
        mxSetFieldByNumber (G, 0, 3, Ab_mx) ;
    }

    //--------------------------------------------------------------------------
    // return the MATLAB struct
    //--------------------------------------------------------------------------

    return (G) ;
}

