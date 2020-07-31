function s = GB_spec_is_positional (op)
%GB_SPEC_IS_POSITIONAL determine if an op is positional

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

if (isstruct (op))
    op = op.opname ;
end
switch (op)
    case { 'firsti' , 'firsti1' , 'firstj' , 'firstj1', ...
           'secondi', 'secondi1', 'secondj', 'secondj1' } ;
        % binary positional op
        s = true ;
    case { 'positioni', 'positioni1', 'positionj', 'positionj1' }
        % unary positional op
        s = true ;
    otherwise
        s = false ;
end
