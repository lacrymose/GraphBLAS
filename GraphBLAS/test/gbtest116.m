function gbtest116
%GBTEST116 list all idxunop operators for GrB.apply2

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
% SPDX-License-Identifier: GPL-3.0-or-later

opnames = {
    'tril',
    'triu',
    'diag',
    'offdiag',
    'diagindex',
    'rowindex',
    'rowle',
    'rowgt',
    'colindex',
    'colle',
    'colgt' } ;

for k1 = 1:length(opnames)
    op = opnames {k1} ;
    fprintf ('\n=================================== %s\n', op) ;
    GrB.binopinfo (op) ;
end

fprintf ('gbtest116: all tests passed\n') ;

