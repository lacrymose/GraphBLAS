//------------------------------------------------------------------------------
// GB_index.h: definitions for index lists and types of assignments
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

#ifndef GB_INDEX_H
#define GB_INDEX_H

//------------------------------------------------------------------------------
// kind of index list, Ikind and Jkind, and assign variations
//------------------------------------------------------------------------------

#define GB_ALL 0
#define GB_RANGE 1
#define GB_STRIDE 2
#define GB_LIST 4

#define GB_ASSIGN 0
#define GB_SUBASSIGN 1
#define GB_ROW_ASSIGN 2
#define GB_COL_ASSIGN 3

#endif
