// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Distributed under the terms of the BSD license (see the LICENSE
// file for the exact terms).
// Email: hermes1d@googlegroups.com, home page: http://hpfem.org/

#ifndef _TRANSFORMS_H_
#define _TRANSFORMS_H_

#include "../../hermes_common/common.h"
#include "lobatto.h"
#include "legendre.h"
#include "space.h"
#include "../../hermes_common/matrix.h"
#include "iterator.h"

void transform_element_refined_forward(int sln, int comp, Element *e, Element *e_ref_left, 
				       Element *e_ref_right);
void transform_element_refined_forward(int comp, Element *e, Element *e_ref_left,  // default for sln=0
				       Element *e_ref_right);
void transform_element_unrefined_forward(int sln, int comp, Element *e, Element *e_ref);
void transform_element_unrefined_forward(int comp, Element *e, Element *e_ref);  // default for sln=0
void transfer_solution_forward(int sln, Space *space, Space *space_ref);
void transfer_solution_forward(Space *space, Space *space_ref);

#endif
