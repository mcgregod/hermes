#include "hermes2d.h"

// This test makes sure that example 03-poisson works correctly.
// CAUTION: This test will fail when any changes to the shapeset
// are made, but it is easy to fix (see below).

int P_INIT = 5;                                   // Uniform polynomial degree of mesh elements.
double CONST_F = 2.0;                             // Constant right-hand side.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Weak forms.
#include "forms.cpp"

int main(int argc, char* argv[])
{
  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("domain.mesh", &mesh);

  // Perform initial mesh refinements.
  mesh.refine_element_id(0);

  // Initialize the weak formulation.
  WeakFormPoisson wf(CONST_F);
  
  // Initialize boundary conditions
  DirichletConstantBoundaryCondition bc(Hermes::vector<std::string>("1", "2", "3", "4"), 0.0);
  BoundaryConditions bcs(&bc);

  // Create an H1 space with default shapeset.
  H1Space space(&mesh, &bcs, P_INIT);
  int ndof = space.get_num_dofs();
  info("ndof = %d", ndof);
  
  // Testing n_dof and correctness of solution vector
  // for p_init = 1, 2, ..., 10
  int success = 1;
  for (int p_init = 1; p_init <= 10; p_init++) {

    printf("********* p_init = %d *********\n", p_init);
    space.set_uniform_order(p_init);

    // Initialize the FE problem.
    bool is_linear = true;
    DiscreteProblem dp(&wf, &space, is_linear);

    // Set up the solver, matrix, and rhs according to the solver selection.
    SparseMatrix* matrix = create_matrix(matrix_solver);
    Vector* rhs = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

    // Initialize the solution.
    Solution sln;

    // Assemble the stiffness matrix and right-hand side vector.
    info("Assembling the stiffness matrix and right-hand side vector.");
    bool rhsonly = false;
    dp.assemble(matrix, rhs, rhsonly);

    // Solve the linear system and if successful, obtain the solution.
    info("Solving the matrix problem.");
    if(solver->solve())
      Solution::vector_to_solution(solver->get_solution(), &space, &sln);
    else
      error ("Matrix solver failed.\n");

    int ndof = Space::get_num_dofs(&space);
    printf("ndof = %d\n", ndof);
    double sum = 0;
    for (int i=0; i < ndof; i++) sum += solver->get_solution()[i];
    printf("coefficient sum = %g\n", sum);

    // Actual test. The values of 'sum' depend on the
    // current shapeset. If you change the shapeset,
    // you need to correct these numbers.
    if (p_init == 1 && fabs(sum - 0.1875) > 1e-3) success = 0;
    if (p_init == 2 && fabs(sum + 0.927932) > 1e-3) success = 0;
    if (p_init == 3 && fabs(sum + 0.65191) > 1e-3) success = 0;
    if (p_init == 4 && fabs(sum + 0.939909) > 1e-3) success = 0;
    if (p_init == 5 && fabs(sum + 0.63356) > 1e-3) success = 0;
    if (p_init == 6 && fabs(sum + 0.905309) > 1e-3) success = 0;
    if (p_init == 7 && fabs(sum + 0.61996) > 1e-3) success = 0;
    if (p_init == 8 && fabs(sum + 0.909494) > 1e-3) success = 0;
    if (p_init == 9 && fabs(sum + 0.610543) > 1e-3) success = 0;
    if (p_init == 10 && fabs(sum + 0.902731) > 1e-3) success = 0;
  }

  if (success == 1) {
    printf("Success!\n");
    return ERR_SUCCESS;
  }
  else {
    printf("Failure!\n");
    return ERR_FAILURE;
  }
}

