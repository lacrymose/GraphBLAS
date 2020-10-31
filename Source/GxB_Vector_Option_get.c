//------------------------------------------------------------------------------
// GxB_Vector_Option_get: get an option in a vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

#include "GB.h"

GrB_Info GxB_Vector_Option_get      // gets the current option of a vector
(
    GrB_Vector v,                   // vector to query
    GxB_Option_Field field,         // option to query
    ...                             // return value of the vector option
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Vector_Option_get (v, field, &value)") ;
    GB_RETURN_IF_NULL_OR_FAULTY (v) ;
    ASSERT_VECTOR_OK (v, "v to get option", GB0) ;

    //--------------------------------------------------------------------------
    // get the option
    //--------------------------------------------------------------------------

    va_list ap ;

    switch (field)
    {

        case GxB_SPARSITY_CONTROL : 
GB_GOTCHA ;

            {
                va_start (ap, field) ;
                int *sparsity = va_arg (ap, int *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (sparsity) ;
                (*sparsity) = v->sparsity ;
            }
            break ;

        case GxB_SPARSITY_STATUS : 

            {
GB_GOTCHA ;
                va_start (ap, field) ;
                int *sparsity = va_arg (ap, int *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (sparsity) ;
                (*sparsity) = GB_sparsity ((GrB_Matrix) v) ;
            }
            break ;

        case GxB_FORMAT : 
GB_GOTCHA ;

            {
                // a GrB_Vector is always stored by-column
                va_start (ap, field) ;
                GxB_Format_Value *format = va_arg (ap, GxB_Format_Value *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (format) ;
                (*format) = GxB_BY_COL ;
            }
            break ;

        case GxB_IS_HYPER : // deprecated; use GxB_SPARSITY_STATUS instead
GB_GOTCHA ;
            {
                // a GrB_Vector is never hypersparse
                va_start (ap, field) ;
                bool *v_is_hyper = va_arg (ap, bool *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (v_is_hyper) ;
                (*v_is_hyper) = false ;
            }
            break ;

        default : 
GB_GOTCHA ;

            return (GrB_INVALID_VALUE) ;

    }
    return (GrB_SUCCESS) ;
}

