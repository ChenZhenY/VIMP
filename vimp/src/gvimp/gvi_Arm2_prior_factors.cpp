/**
 * A script to compute the prior factors for a 2-link arm trajectory for computing the time.
 * Hongzhe YU
 * Date: 08/25/2023
*/

#include "helpers/EigenWrapper.h"
#include "helpers/ExperimentParams.h"
#include "instances/FactorizedGVI.h"
#include <gpmp2/obstacle/ObstacleSDFFactor.h>
#include <gtsam/inference/Symbol.h>
#include "robots/PlanarArm2SDF_pgcs.h"

using namespace vimp;

int main(){

    int i_exp = 2;
    double total_time = 6.0;
    int nt = 50;
    double coeff_Qc = 1.0;
    double sig_obs = 0.05;
    double eps_sdf = 0.1;
    double radius = 0.01;
    double step_size = 0.75;
    double init_precision_factor = 10000.0;
    double boundary_penalties = 10000.0;
    double temperature = 0.0001;
    double high_temperature = 0.05;
    int low_temp_iterations = 30;
    double stop_err = 1e-5;
    int num_iter = 40;
    int max_n_backtracking = 10;
    std::string map_name{"2darm_map1"};
    // std::string sdf_file{"/home/hzyu/git/VIMP/vimp/maps/WAM/WAMDeskDataset.bin"};

    int nx = 4, nu = 2;

    GVIMPParams params(nx, nu, total_time, nt, coeff_Qc, sig_obs, 
                        eps_sdf, radius, step_size, num_iter, init_precision_factor, 
                        boundary_penalties, temperature, high_temperature, low_temp_iterations, 
                        stop_err, max_n_backtracking, map_name);


    PlanarArm2SDFExample _robot_sdf(params.eps_sdf(), params.radius(), params.map_name(), params.sdf_file());

    /// parameters
    int n_states = params.nt();
    int N = n_states - 1;
    const int dim_conf = _robot_sdf.ndof() * _robot_sdf.nlinks();
    // state: theta = [conf, vel_conf]
    const int dim_state = 2 * dim_conf; 
    /// joint dimension
    const int ndim = dim_state * n_states;

    VectorXd start_theta{ params.m0() };
    VectorXd goal_theta{ params.mT() };

    MatrixXd Qc{MatrixXd::Identity(dim_conf, dim_conf)*params.coeff_Qc()};
    MatrixXd K0_fixed{MatrixXd::Identity(dim_state, dim_state)/params.boundary_penalties()};

    /// Vector of base factored optimizers
    vector<std::shared_ptr<GVIFactorizedBase>> vec_factors;

    auto robot_model = _robot_sdf.RobotModel();
    auto sdf = _robot_sdf.sdf();
    // double sig_obs = params.sig_obs(), eps_sdf = params.eps_sdf();
    // double temperature = params.temperature(), high_temperature = params.high_temperature();

    /// initial values
    VectorXd joint_init_theta{VectorXd::Zero(ndim)};
    VectorXd avg_vel{(goal_theta.segment(0, dim_conf) - start_theta.segment(0, dim_conf)) / params.total_time()};

    /// prior 
    double delt_t = params.total_time() / N;

    for (int i = 0; i < n_states; i++) {

        // initial state
        VectorXd theta_i{start_theta + double(i) * (goal_theta - start_theta) / N};

        // initial velocity: must have initial velocity for the fitst state??
        theta_i.segment(dim_conf, dim_conf) = avg_vel;
        joint_init_theta.segment(i*dim_state, dim_state) = std::move(theta_i);   

        MinimumAccGP lin_gp{Qc, i, delt_t, start_theta};

        // fixed start and goal priors
        // Factor Order: [fixed_gp_0, lin_gp_1, obs_1, ..., lin_gp_(N-1), obs_(N-1), lin_gp_(N), fixed_gp_(N)] 
        if (i==0 || i==n_states-1){

            // lin GP factor for the first and the last support state
            if (i == n_states-1){
                // std::shared_ptr<LinearGpPrior> p_lin_gp{}; 
                vec_factors.emplace_back(new LinearGpPrior{2*dim_state, 
                                                            dim_state, 
                                                            cost_linear_gp, 
                                                            lin_gp, 
                                                            n_states, 
                                                            i-1, 
                                                            params.temperature(), 
                                                            params.high_temperature()});
            }

            // Fixed gp factor
            FixedPriorGP fixed_gp{K0_fixed, MatrixXd{theta_i}};
            vec_factors.emplace_back(new FixedGpPrior{dim_state, 
                                                        dim_state, 
                                                        cost_fixed_gp, 
                                                        fixed_gp, 
                                                        n_states, 
                                                        i,
                                                        params.temperature(), 
                                                        params.high_temperature()});

        }else{
            // linear gp factors
            vec_factors.emplace_back(new LinearGpPrior{2*dim_state, 
                                                        dim_state, 
                                                        cost_linear_gp, 
                                                        lin_gp, 
                                                        n_states, 
                                                        i-1, 
                                                        params.temperature(), 
                                                        params.high_temperature()});
  
        }
    }

    /// The joint optimizer
    GVIGH<GVIFactorizedBase> optimizer{vec_factors, 
                                        dim_state, 
                                        n_states, 
                                        params.max_iter(), 
                                        params.temperature(), 
                                        params.high_temperature()};

    optimizer.set_max_iter_backtrack(params.max_n_backtrack());
    optimizer.set_niter_low_temperature(params.max_iter_lowtemp());
    optimizer.set_stop_err(params.stop_err());

    optimizer.update_file_names(params.saving_prefix());
    optimizer.set_mu(joint_init_theta);

    optimizer.initilize_precision_matrix(params.initial_precision_factor());

    optimizer.set_GH_degree(3);
    optimizer.set_step_size_base(params.step_size()); // a local optima

    VectorXd factor_cost_vector = optimizer.factor_cost_vector();

    // EigenWrapper ei;
    // ei.print_matrix(factor_cost_vector, "factor_cost_vector");

    // optimizer.optimize(verbose);

    // _last_iteration_mean_precision = std::make_tuple(optimizer.mean(), optimizer.precision());

    // return _last_iteration_mean_precision;

}

