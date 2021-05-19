//------------------------------------------------------------------------------
// GB_AxB_defs__max_div_int8: definitions for a single semiring
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// If this file is in the Generated/ folder, do not edit it (auto-generated).

#include "GB_dev.h"
#ifndef GBCOMPACT
#include "GB.h"
#include "GB_control.h"
#include "GB_bracket.h"
#include "GB_sort.h"
#include "GB_atomics.h"
#include "GB_AxB_saxpy.h"
#include "GB_AxB__include.h"
#include "GB_unused.h"
#include "GB_bitmap_assign_methods.h"
#include "GB_ek_slice_search.c"

// This C=A*B semiring is defined by the following types and operators:

// A'*B (dot2):        GB (_Adot2B__max_div_int8)
// A'*B (dot3):        GB (_Adot3B__max_div_int8)
// C+=A'*B (dot4):     GB (_Adot4B__max_div_int8)
// A*B (saxpy3):       GB (_Asaxpy3B__max_div_int8)
//     no mask:        GB (_Asaxpy3B_noM__max_div_int8)
//     mask M:         GB (_Asaxpy3B_M__max_div_int8)
//     mask !M:        GB (_Asaxpy3B_notM__max_div_int8)
// A*B (saxpy bitmap): GB (_AsaxbitB__max_div_int8)

// C type:   int8_t
// A type:   int8_t
// B type:   int8_t

// Multiply: z = GB_IDIV_SIGNED (aik, bkj, 8)
// Add:      if (cij < z) { cij = z ; }
//           'any' monoid?  0
//           atomic?        1
//           OpenMP atomic? 0
// MultAdd:  int8_t x_op_y = GB_IDIV_SIGNED (aik, bkj, 8) ; cij = GB_IMAX (cij, x_op_y)
// Identity: INT8_MIN
// Terminal: if (cij == INT8_MAX) { break ; }

#define GB_ATYPE \
    int8_t

#define GB_BTYPE \
    int8_t

#define GB_CTYPE \
    int8_t

#define GB_ASIZE (sizeof (GB_BTYPE))
#define GB_BSIZE (sizeof (GB_BTYPE))
#define GB_CSIZE (sizeof (GB_CTYPE))

// true for int64, uint64, float, double, float complex, and double complex 
#define GB_CTYPE_IGNORE_OVERFLOW \
    0

// aik = Ax [pA]
#define GB_GETA(aik,Ax,pA,A_iso) \
    int8_t aik = GBX (Ax, pA, A_iso)

// bkj = Bx [pB]
#define GB_GETB(bkj,Bx,pB,B_iso) \
    int8_t bkj = GBX (Bx, pB, B_iso)

// Gx [pG] = Ax [pA]
#define GB_LOADA(Gx,pG,Ax,pA,A_iso) \
    Gx [pG] = GBX (Ax, pA, A_iso)

// Gx [pG] = Bx [pB]
#define GB_LOADB(Gx,pG,Bx,pB,B_iso) \
    Gx [pG] = GBX (Bx, pB, B_iso)

#define GB_CX(p) Cx [p]

// multiply operator
#define GB_MULT(z, x, y, i, k, j) \
    z = GB_IDIV_SIGNED (x, y, 8)

// cast from a real scalar (or 2, if C is complex) to the type of C
#define GB_CTYPE_CAST(x,y) \
    ((int8_t) x)

// cast from a real scalar (or 2, if A is complex) to the type of A
#define GB_ATYPE_CAST(x,y) \
    ((int8_t) x)

// multiply-add
#define GB_MULTADD(z, x, y, i, k, j) \
    int8_t x_op_y = GB_IDIV_SIGNED (x, y, 8) ; z = GB_IMAX (z, x_op_y)

// monoid identity value
#define GB_IDENTITY \
    INT8_MIN

// 1 if the identity value can be assigned via memset, with all bytes the same
#define GB_HAS_IDENTITY_BYTE \
    0

// identity byte, for memset
#define GB_IDENTITY_BYTE \
    (none)

// break if cij reaches the terminal value (dot product only)
#define GB_DOT_TERMINAL(cij) \
    if (cij == INT8_MAX) { break ; }

// simd pragma for dot-product loop vectorization
#define GB_PRAGMA_SIMD_DOT(cij) \
    ;

// simd pragma for other loop vectorization
#define GB_PRAGMA_SIMD_VECTORIZE GB_PRAGMA_SIMD

// 1 for the PLUS_PAIR_(real) semirings, not for the complex case
#define GB_IS_PLUS_PAIR_REAL_SEMIRING \
    0

// // 1 for performance-critical semirings, which get extra optimization
// #define GB_IS_PERFORMANCE_CRITICAL_SEMIRING \
//     GB_is_performance_critical_semiring

// declare the cij scalar
#if GB_IS_PLUS_PAIR_REAL_SEMIRING
    // also initialize cij to zero
    #define GB_CIJ_DECLARE(cij) \
        int8_t cij = 0
#else
    // all other semirings: just declare cij, do not initialize it
    #define GB_CIJ_DECLARE(cij) \
        int8_t cij
#endif

// cij = Cx [pC]
#define GB_GETC(cij,p) cij = Cx [p]

// Cx [pC] = cij
#define GB_PUTC(cij,p) Cx [p] = cij

// Cx [p] = t
#define GB_CIJ_WRITE(p,t) Cx [p] = t

// C(i,j) += t
#define GB_CIJ_UPDATE(p,t) \
    if (Cx [p] < t) { Cx [p] = t ; }

// x + y
#define GB_ADD_FUNCTION(x,y) \
    GB_IMAX (x, y)

// bit pattern for bool, 8-bit, 16-bit, and 32-bit integers
#define GB_CTYPE_BITS \
    0xffL

// 1 if monoid update can skipped entirely (the ANY monoid)
#define GB_IS_ANY_MONOID \
    0

// 1 if monoid update is EQ
#define GB_IS_EQ_MONOID \
    0

// 1 if monoid update can be done atomically, 0 otherwise
#define GB_HAS_ATOMIC \
    1

// 1 if monoid update can be done with an OpenMP atomic update, 0 otherwise
#if GB_MICROSOFT
    #define GB_HAS_OMP_ATOMIC \
        0
#else
    #define GB_HAS_OMP_ATOMIC \
        0
#endif

// 1 for the ANY_PAIR semirings
#define GB_IS_ANY_PAIR_SEMIRING \
    0

// 1 if PAIR is the multiply operator 
#define GB_IS_PAIR_MULTIPLIER \
    0

// 1 if monoid is PLUS_FC32
#define GB_IS_PLUS_FC32_MONOID \
    0

// 1 if monoid is PLUS_FC64
#define GB_IS_PLUS_FC64_MONOID \
    0

// 1 if monoid is ANY_FC32
#define GB_IS_ANY_FC32_MONOID \
    0

// 1 if monoid is ANY_FC64
#define GB_IS_ANY_FC64_MONOID \
    0

// 1 if monoid is MIN for signed or unsigned integers
#define GB_IS_IMIN_MONOID \
    0

// 1 if monoid is MAX for signed or unsigned integers
#define GB_IS_IMAX_MONOID \
    1

// 1 if monoid is MIN for float or double
#define GB_IS_FMIN_MONOID \
    0

// 1 if monoid is MAX for float or double
#define GB_IS_FMAX_MONOID \
    0

// 1 for the FIRSTI or FIRSTI1 multiply operator
#define GB_IS_FIRSTI_MULTIPLIER \
    0

// 1 for the FIRSTJ or FIRSTJ1 multiply operator
#define GB_IS_FIRSTJ_MULTIPLIER \
    0

// 1 for the SECONDJ or SECONDJ1 multiply operator
#define GB_IS_SECONDJ_MULTIPLIER \
    0

// atomic compare-exchange
#define GB_ATOMIC_COMPARE_EXCHANGE(target, expected, desired) \
    GB_ATOMIC_COMPARE_EXCHANGE_8 (target, expected, desired)

#if GB_IS_ANY_PAIR_SEMIRING

    // result is purely symbolic; no numeric work to do.  Hx is not used.
    #define GB_HX_WRITE(i,t)
    #define GB_CIJ_GATHER(p,i)
    #define GB_CIJ_GATHER_UPDATE(p,i)
    #define GB_HX_UPDATE(i,t)
    #define GB_CIJ_MEMCPY(p,i,len)

#else

    // Hx [i] = t
    #define GB_HX_WRITE(i,t) Hx [i] = t

    // Hx [i] = identity
    #define GB_HX_CLEAR(i) Hx [i] = GB_IDENTITY

    // Cx [p] = Hx [i]
    #define GB_CIJ_GATHER(p,i) Cx [p] = Hx [i]

    // Cx [p] += Hx [i]
    #define GB_CIJ_GATHER_UPDATE(p,i) \
        if (Cx [p] < Hx [i]) { Cx [p] = Hx [i] ; }

    // Hx [i] += t
    #define GB_HX_UPDATE(i,t) \
        if (Hx [i] < t) { Hx [i] = t ; }

    // memcpy (&(Cx [p]), &(Hx [i]), len)
    #define GB_CIJ_MEMCPY(p,i,len) \
        memcpy (Cx +(p), Hx +(i), (len) * sizeof(int8_t))

#endif

// 1 if the semiring has a concise bitmap multiply-add
#define GB_HAS_BITMAP_MULTADD \
    1

// concise statement(s) for the bitmap case:
//  if (exists)
//      if (cb == 0)
//          cx = ax * bx
//          cb = 1
//      else
//          cx += ax * bx
#define GB_BITMAP_MULTADD(cb,cx,exists,ax,bx) \
    int8_t t = ((int8_t) (GB_IDIV_SIGNED (ax, bx, 8))) ; if (exists && cx < t) { cx = t ; } ; cb |= exists

// define X for bitmap multiply-add
#define GB_XINIT \
    ;

// load X [1] = bkj for bitmap multiply-add
#define GB_XLOAD(bkj) \
    ;

// disable this semiring and use the generic case if these conditions hold
#define GB_DISABLE \
    (GxB_NO_MAX || GxB_NO_DIV || GxB_NO_INT8 || GxB_NO_MAX_INT8 || GxB_NO_DIV_INT8 || GxB_NO_MAX_DIV_INT8)

#endif

