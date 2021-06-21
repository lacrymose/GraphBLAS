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

// op(A)  function:  GB (_unop_apply__abs_int16_int16)
// op(A') function:  GB (_unop_tran__abs_int16_int16)

// C type:   int16_t
// A type:   int16_t
// cast:     int16_t cij = aij
// unaryop:  cij = GB_IABS (aij)

#define GB_ATYPE \
    int16_t

#define GB_CTYPE \
    int16_t

// aij = Ax [pA]
#define GB_GETA(aij,Ax,pA) \
    int16_t aij = Ax [pA]

#define GB_CX(p) Cx [p]

// unary operator
#define GB_OP(z, x) \
    z = GB_IABS (x) ;

// casting
#define GB_CAST(z, aij) \
    int16_t z = aij ;

// cij = op (aij)
#define GB_CAST_OP(pC,pA)           \
{                                   \
    /* aij = Ax [pA] */             \
    int16_t aij = Ax [pA] ;          \
    /* Cx [pC] = op (cast (aij)) */ \
    int16_t z = aij ;               \
    Cx [pC] = GB_IABS (z) ;        \
}

// disable this operator and use the generic case if these conditions hold
#define GB_DISABLE \
    (GxB_NO_ABS || GxB_NO_INT16)

//------------------------------------------------------------------------------
// Cx = op (cast (Ax)): apply a unary operator
//------------------------------------------------------------------------------


GrB_Info GB (_unop_apply__abs_int16_int16)
(
    int16_t *Cx,       // Cx and Ax may be aliased
    const int16_t *Ax,
    const int8_t *restrict Ab,   // A->b if A is bitmap
    int64_t anz,
    int nthreads
)
{
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    int64_t p ;
    if (Ab == NULL)
    { 
        #pragma omp parallel for num_threads(nthreads) schedule(static)
        for (p = 0 ; p < anz ; p++)
        {
            int16_t aij = Ax [p] ;
            int16_t z = aij ;
            Cx [p] = GB_IABS (z) ;
        }
    }
    else
    { 
        // bitmap case, no transpose; A->b already memcpy'd into C->b
        #pragma omp parallel for num_threads(nthreads) schedule(static)
        for (p = 0 ; p < anz ; p++)
        {
            if (!Ab [p]) continue ;
            int16_t aij = Ax [p] ;
            int16_t z = aij ;
            Cx [p] = GB_IABS (z) ;
        }
    }
    return (GrB_SUCCESS) ;
    #endif
}


//------------------------------------------------------------------------------
// C = op (cast (A')): transpose, typecast, and apply a unary operator
//------------------------------------------------------------------------------

GrB_Info GB (_unop_tran__abs_int16_int16)
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

