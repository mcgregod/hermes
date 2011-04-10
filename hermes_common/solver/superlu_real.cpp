// This file is part of Hermes3D
//
// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Email: hpfem-group@unr.edu, home page: http://hpfem.org/.
//
// Hermes3D is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// Hermes3D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes3D; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include"superlu.h"
#include <slu_ddefs.h>

template <>
void  SuperLUSolver<double>::solver_driver (superlu_options_t *options, SuperMatrix *A, int *perm_c, int *perm_r, int *etree, char *equed, double *R, 
                                          double *C, SuperMatrix *L, SuperMatrix *U, void *work, int lwork, SuperMatrix *B, SuperMatrix *X, 
                                          double *recip_pivot_growth, double *rcond, double *ferr, double *berr, mem_usage_t *mem_usage, SuperLUStat_t *stat, 
                                          int *info){
  dgssvx(options, A, perm_c, perm_r, etree, equed, R, C, L, U, work, lwork, B, X, recip_pivot_growth, rcond, ferr, berr, mem_usage, stat, info);
}

template <>
void SuperLUSolver<double>::create_csc_matrix (SuperMatrix *A, int m, int n, int nnz, typename SuperLuType<double>::scalar *nzval, 
                        int *rowind, int *colptr, Stype_t stype, Dtype_t dtype, Mtype_t mtype){
  dCreate_CompCol_Matrix (A, m, n, nnz, nzval, rowind, colptr, stype, dtype, mtype);
}

