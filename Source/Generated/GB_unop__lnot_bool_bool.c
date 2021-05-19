//------------------------------------------------------------------------------
// GB_unop:  hard-coded functions for each built-in unary operator
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// If this file is in the Generated/ folder, do not edit it (auto-generated).

#include "GB.h"
#ifndef GBCOMPACT
#include "GB_control.h"
#include "GB_atomics.h"
#include "GB_unop__include.h"

// C=unop(A) is defined by the following types and operators:

// op(A)  function:  GB (_unop_apply__lnot_bool_bool)
// op(A') function:  GB (_unop_tran__lnot_bool_bool)

// C type:   bool
// A type:   bool
// cast:     bool cij = aij
// unaryop:  cij = !aij

#define GB_ATYPE \
    bool

#define GB_CTYPE \
    bool

// aij = Ax [pA]
#define GB_GETA(aij,Ax,pA,A_iso) \
    bool aij = GBX (Ax, pA, A_iso)

#define GB_CX(p) Cx [p]

// unary operator
#define GB_OP(z, x) \
    z = !x ;

// casting
#define GB_CAST(z, aij) \
    bool z = aij ;

// cij = op (aij)
#define GB_CAST_OP(pC,pA,A_iso)     \
{                                   \
    /* aij = Ax [pA] */             \
    bool aij = GBX (Ax, pA, A_iso) ;   \
    /* Cx [pC] = op (cast (aij)) */ \
    bool z = aij ;               \
    Cx [pC] = !z ;        \
}

// true if operator is the identity op with no typecasting
#define GB_OP_IS_IDENTITY_WITH_NO_TYPECAST \
    0

// disable this operator and use the generic case if these conditions hold
#define GB_DISABLE \
    (GxB_NO_LNOT || GxB_NO_BOOL)

//------------------------------------------------------------------------------
// Cx = op (cast (Ax)): apply a unary operator
//------------------------------------------------------------------------------

GrB_Info GB (_unop_apply__lnot_bool_bool)
(
    bool *Cx,       // Cx and Ax may be aliased
    const bool *Ax,
    const bool A_iso,
    const int8_t *restrict Ab,   // A->b if A is bitmap
    int64_t anz,
    int nthreads
)
{
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    int64_t p ;

    // TODO: if OP is ONE and iso-valued matrices are exploited, then
    // do this in O(1) time.  Or, if C is also iso-valued, do in O(1) time.

    if (Ab == NULL)
    { 
        #if ( GB_OP_IS_IDENTITY_WITH_NO_TYPECAST )
            GB_memcpy (Cx, Ax, anz * sizeof (bool), nthreads) ;
        #else
            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (p = 0 ; p < anz ; p++)
            {
                bool aij = GBX (Ax, p, A_iso) ;
                bool z = aij ;
                Cx [p] = !z ;
            }
        #endif
    }
    else
    { 
        // bitmap case, no transpose; A->b already memcpy'd into C->b
        #pragma omp parallel for num_threads(nthreads) schedule(static)
        for (p = 0 ; p < anz ; p++)
        {
            if (!Ab [p]) continue ;
            bool aij = GBX (Ax, p, A_iso) ;
            bool z = aij ;
            Cx [p] = !z ;
        }
    }
    return (GrB_SUCCESS) ;
    #endif
}

//------------------------------------------------------------------------------
// C = op (cast (A')): transpose, typecast, and apply a unary operator
//------------------------------------------------------------------------------

GrB_Info GB (_unop_tran__lnot_bool_bool)
(
    GrB_Matrix C,
    const GrB_Matrix A,
    int64_t *restrict *Workspaces,
    const int64_t *restrict A_slice,
    int nworkspaces,
    int nthreads
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    #include "GB_unop_transpose.c"
    return (GrB_SUCCESS) ;
    #endif
}

#endif

