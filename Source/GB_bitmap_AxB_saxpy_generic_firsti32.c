//------------------------------------------------------------------------------
// GB_bitmap_AxB_saxpy_generic_firsti32.c: C=A*B, C bitmap/full, FIRSTI mult
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// C is bitmap/full
// multiply op is GxB_FIRSTI_INT32 or GxB_FIRSTI1_INT32

#define GB_AXB_SAXPY_GENERIC_METHOD GB_bitmap_AxB_saxpy_generic_firsti32 
#define C_IS_SPARSE_OR_HYPERSPARSE  0
#define OP_IS_POSITIONAL            1
#define FLIPXY                      0
#define OP_IS_INT64                 0
#define OP_IS_FIRSTI                1
#define OP_IS_FIRSTJ                0
#define OP_IS_FIRST                 0
#define OP_IS_SECOND                0

#include "GB_AxB_saxpy_generic_method.c"

