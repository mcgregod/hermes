#define HERMES_REPORT_WARN
#define HERMES_REPORT_INFO
#define HERMES_REPORT_VERBOSE
#define HERMES_REPORT_FILE "application.log"
#include "hermes2d.h"

using namespace RefinementSelectors;

// This test makes sure that example 11-adapt-system works correctly.

const int P_INIT_U = 2;                           // Initial polynomial degree for u.
const int P_INIT_V = 1;                           // Initial polynomial degree for v.
const int INIT_REF_BDY = 5;                       // Number of initial boundary refinements
const bool MULTI = true;                          // MULTI = true  ... use multi-mesh,
                                                  // MULTI = false ... use single-mesh.
                                                  // Note: In the single mesh option, the meshes are
                                                  // forced to be geometrically the same but the
                                                  // polynomial degrees can still vary.
const double THRESHOLD = 0.3;                     // This is a quantitative parameter of the adapt(...) function and
                                                  // it has different meanings for various adaptive strategies (see below).
const int STRATEGY = 1;                           // Adaptive strategy:
                                                  // STRATEGY = 0 ... refine elements until sqrt(THRESHOLD) times total
                                                  //   error is processed. If more elements have similar errors, refine
                                                  //   all to keep the mesh symmetric.
                                                  // STRATEGY = 1 ... refine all elements whose error is larger
                                                  //   than THRESHOLD times maximum element error.
                                                  // STRATEGY = 2 ... refine all elements whose error is larger
                                                  //   than THRESHOLD.
                                                  // More adaptive strategies can be created in adapt_ortho_h1.cpp.
const CandList CAND_LIST = H2D_HP_ANISO;          // Predefined list of element refinement candidates. Possible values are
                                                  // H2D_P_ISO, H2D_P_ANISO, H2D_H_ISO, H2D_H_ANISO, H2D_HP_ISO,
                                                  // H2D_HP_ANISO_H, H2D_HP_ANISO_P, H2D_HP_ANISO.
                                                  // See the User Documentation for details.
const int MESH_REGULARITY = -1;                   // Maximum allowed level of hanging nodes:
                                                  // MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
                                                  // MESH_REGULARITY = 1 ... at most one-level hanging nodes,
                                                  // MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
                                                  // Note that regular meshes are not supported, this is due to
                                                  // their notoriously bad performance.
const double CONV_EXP = 1;                        // Default value is 1.0. This parameter influences the selection of
                                                  // cancidates in hp-adaptivity. See get_optimal_refinement() for details.
const double ERR_STOP = 0.1;                      // Stopping criterion for adaptivity (rel. error tolerance between the
                                                  // fine mesh and coarse mesh solution in percent).
const int NDOF_STOP = 60000;                      // Adaptivity process stops when the number of degrees of freedom grows over
                                                  // this limit. This is mainly to prevent h-adaptivity to go on forever.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Problem parameters.
const double D_u = 1;
const double D_v = 1;
const double SIGMA = 1;
const double LAMBDA = 1;
const double KAPPA = 1;
const double K = 100;

// Boundary markers.
const std::string OUTER_BDY = "1";

// Weak forms.
#include "forms.cpp"

int main(int argc, char* argv[])
{
  // Load the mesh.
  Mesh u_mesh, v_mesh;
  H2DReader mloader;
  mloader.load("square.mesh", &u_mesh);
  if (MULTI == false) u_mesh.refine_towards_boundary(OUTER_BDY, INIT_REF_BDY);

  // Create initial mesh (master mesh).
  v_mesh.copy(&u_mesh);

  // Initial mesh refinements in the v_mesh towards the boundary.
  if (MULTI == true) v_mesh.refine_towards_boundary(OUTER_BDY, INIT_REF_BDY);

  // Set exact solutions.
  ExactSolutionFitzHughNagumo1 exact_u(&u_mesh, SIGMA, D_u);
  ExactSolutionFitzHughNagumo2 exact_v(&v_mesh, K, D_v);

  // Initialize the weak formulation.
  WeakFormFitzHughNagumo wf(&exact_u, &exact_v);
  
  // Initialize boundary conditions
  DirichletConstantBoundaryCondition bc_u(OUTER_BDY, 0.0);
  BoundaryConditions bcs_u(&bc_u);
  DirichletConstantBoundaryCondition bc_v(OUTER_BDY, 0.0);
  BoundaryConditions bcs_v(&bc_v);

  // Create H1 spaces with default shapeset for both displacement components.
  H1Space u_space(&u_mesh, &bcs_u, P_INIT_U);
  H1Space v_space(MULTI ? &v_mesh : &u_mesh, &bcs_v, P_INIT_V);

  // Initialize coarse and reference mesh solutions.
  Solution u_sln, v_sln, u_ref_sln, v_ref_sln;

  // Initialize refinement selector.
  H1ProjBasedSelector selector(CAND_LIST, CONV_EXP, H2DRS_DEFAULT_ORDER);

  // DOF and CPU convergence graphs.
  SimpleGraph graph_dof_est, graph_cpu_est, 
              graph_dof_exact, graph_cpu_exact;

  // Time measurement.
  TimePeriod cpu_time;
  cpu_time.tick();

  // Adaptivity loop:
  int as = 1; 
  bool done = false;
  do
  {
    info("---- Adaptivity step %d:", as);

    // Construct globally refined reference mesh and setup reference space.
    Hermes::vector<Space *>* ref_spaces = Space::construct_refined_spaces(Hermes::vector<Space *>(&u_space, &v_space));

    // Assemble the reference problem.
    info("Solving on reference mesh.");
    bool is_linear = true;
    DiscreteProblem* dp = new DiscreteProblem(&wf, *ref_spaces, is_linear);
    SparseMatrix* matrix = create_matrix(matrix_solver);
    Vector* rhs = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);
    dp->assemble(matrix, rhs);

    // Time measurement.
    cpu_time.tick();
    
    // Solve the linear system of the reference problem. If successful, obtain the solutions.
    if(solver->solve()) Solution::vector_to_solutions(solver->get_solution(), *ref_spaces, 
                                            Hermes::vector<Solution *>(&u_ref_sln, &v_ref_sln));
    else error ("Matrix solver failed.\n");
  
    // Time measurement.
    cpu_time.tick();

    // Project the fine mesh solution onto the coarse mesh.
    info("Projecting reference solution on coarse mesh.");
    OGProjection::project_global(Hermes::vector<Space *>(&u_space, &v_space), Hermes::vector<Solution *>(&u_ref_sln, &v_ref_sln), 
                   Hermes::vector<Solution *>(&u_sln, &v_sln), matrix_solver); 
   
    // Calculate element errors.
    info("Calculating error estimate and exact error."); 
    Adapt* adaptivity = new Adapt(Hermes::vector<Space *>(&u_space, &v_space));
    
    // Calculate error estimate for each solution component and the total error estimate.
    Hermes::vector<double> err_est_rel;
    double err_est_rel_total = adaptivity->calc_err_est(Hermes::vector<Solution *>(&u_sln, &v_sln), 
                               Hermes::vector<Solution *>(&u_ref_sln, &v_ref_sln), &err_est_rel) * 100;

    // Calculate exact error for each solution component and the total exact error.
    Hermes::vector<double> err_exact_rel;
    bool solutions_for_adapt = false;
    double err_exact_rel_total = adaptivity->calc_err_exact(Hermes::vector<Solution *>(&u_sln, &v_sln), 
							    Hermes::vector<Solution *>(&exact_u, &exact_v), 
                                                            &err_exact_rel, solutions_for_adapt) * 100;

    // Time measurement.
    cpu_time.tick();

    // Report results.
    info("ndof_coarse[0]: %d, ndof_fine[0]: %d",
         u_space.Space::get_num_dofs(), (*ref_spaces)[0]->Space::get_num_dofs());
    info("err_est_rel[0]: %g%%, err_exact_rel[0]: %g%%", err_est_rel[0]*100, err_exact_rel[0]*100);
    info("ndof_coarse[1]: %d, ndof_fine[1]: %d",
         v_space.Space::get_num_dofs(), (*ref_spaces)[1]->Space::get_num_dofs());
    info("err_est_rel[1]: %g%%, err_exact_rel[1]: %g%%", err_est_rel[1]*100, err_exact_rel[1]*100);
    info("ndof_coarse_total: %d, ndof_fine_total: %d",
         Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)), Space::get_num_dofs(*ref_spaces));
    info("err_est_rel_total: %g%%, err_est_exact_total: %g%%", err_est_rel_total, err_exact_rel_total);

    // Add entry to DOF and CPU convergence graphs.
    graph_dof_est.add_values(Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)), err_est_rel_total);
    graph_dof_est.save("conv_dof_est.dat");
    graph_cpu_est.add_values(cpu_time.accumulated(), err_est_rel_total);
    graph_cpu_est.save("conv_cpu_est.dat");
    graph_dof_exact.add_values(Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)), err_exact_rel_total);
    graph_dof_exact.save("conv_dof_exact.dat");
    graph_cpu_exact.add_values(cpu_time.accumulated(), err_exact_rel_total);
    graph_cpu_exact.save("conv_cpu_exact.dat");

    // If err_est too large, adapt the mesh.
    if (err_est_rel_total < ERR_STOP) 
      done = true;
    else 
    {
      info("Adapting coarse mesh.");
      done = adaptivity->adapt(Hermes::vector<RefinementSelectors::Selector *>(&selector, &selector), 
                               THRESHOLD, STRATEGY, MESH_REGULARITY);
    }
    if (Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)) >= NDOF_STOP) done = true;

    // Clean up.
    delete solver;
    delete matrix;
    delete rhs;
    delete adaptivity;
    if(done == false)
      for(unsigned int i = 0; i < ref_spaces->size(); i++)
        delete (*ref_spaces)[i]->get_mesh();
    delete ref_spaces;
    delete dp;
    
    // Increase counter.
    as++;
  }
  while (done == false);

  verbose("Total running time: %g s", cpu_time.accumulated());

  int ndof = Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space));

  printf("ndof allowed = %d\n", 580);
  printf("ndof actual = %d\n", ndof);
  if (ndof < 580) {      // ndofs was 574 at the time this test was created
    printf("Success!\n");
    return ERR_SUCCESS;
  }
  else {
    printf("Failure!\n");
    return ERR_FAILURE;
  }
}

