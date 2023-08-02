#include "instances/PlanarFactorPR.h"
#include "robots/PlanarPointRobotSDFMultiObsExample.h"
#include <gtest/gtest.h>

using namespace gpmp2;
using namespace Eigen;
using namespace vimp;
using namespace std;


int dim_state = 4, num_states = 1; 

/// Obs factor
double cost_sigma = 0.5, epsilon = 4.0;

// An example pr and sdf
vimp::PlanarPointRobotSDFMultiObsExample planar_pr_sdf;
gpmp2::PointRobotModel pRModel = std::move(planar_pr_sdf.pRmodel());
gpmp2::PlanarSDF sdf = std::move(planar_pr_sdf.sdf());

// collision factor
gpmp2::ObstaclePlanarSDFFactorPointRobot collision_k{gtsam::symbol('x', 0), pRModel, sdf, cost_sigma, epsilon};

TEST(ColCost, sdf_map){

    // Test point
    VectorXd conf_test(2);
    conf_test.setZero();
    conf_test << 11.3321059864421, 9.16117246531728;

    // signed distance ground truth
    double signed_distance = planar_pr_sdf.sdf().getSignedDistance(conf_test);
    double signed_distance_gt = 4.5321;
    ASSERT_LE(abs(signed_distance - signed_distance_gt), 1e-5);

    // error vector ground truth
    VectorXd vec_err = collision_k.evaluateError(conf_test);  
    VectorXd err_vec_gt(1);
    err_vec_gt(0) = 0.9679;
    ASSERT_LE((err_vec_gt - vec_err).norm(), 1e-5);

}


TEST(ColCost, change_covariance){
    
    /// Vector of base factored optimizers
    vector<std::shared_ptr<GVIFactorizedBase>> vec_factor_opts;

    
    MatrixXd Pk_col{MatrixXd::Zero(2, 4)};
    Pk_col.block(0, 0, 2, 2) = std::move(MatrixXd::Identity(2, 2));

    /// Factored optimizer
    std::shared_ptr<PlanarSDFFactorPR> p_obs{new PlanarSDFFactorPR{2, dim_state, cost_sdf_pR, collision_k, num_states, 0, 1.0, 10.0}};
    vec_factor_opts.emplace_back(p_obs);

    /// The joint optimizer
    GVIGH<GVIFactorizedBase> optimizer{vec_factor_opts, dim_state, num_states};

    /// Set initial value to the linear interpolation
    VectorXd joint_init_theta(4);
    joint_init_theta.setZero();
    joint_init_theta << 11.3321059864421, 9.16117246531728, 0, 0;

    optimizer.set_mu(joint_init_theta);

    optimizer.set_GH_degree(10);

    // *********** In the case of a very small covariance, the expected value should equal the mean value.
    MatrixXd precision = MatrixXd::Identity(4, 4) * 100000.0;
    SpMat precision_sp = precision.sparseView();

    optimizer.set_precision(precision_sp);

    double cost = optimizer.cost_value_no_entropy();
    cout << "cost_w_o_entropy " << endl << cost << endl;

    VectorXd vec_err(1);
    vec_err = collision_k.evaluateError(joint_init_theta.segment(0, 2));  

    double temperature = 10.0;
    double  cost_expected = vec_err(0) * vec_err(0) / cost_sigma / temperature;
    ASSERT_LE(abs(cost - cost_expected), 1e-5);


    // *********** In the case of a normal covariance, compared with the ground truth from matlab.
    precision = MatrixXd::Identity(4, 4);
    precision_sp = precision.sparseView();

    optimizer.set_precision(precision_sp);

    double cost_new = optimizer.cost_value_no_entropy();
    cout << "cost_new " << endl << cost_new << endl;

    cost_expected = 0.387363776150491;

    ASSERT_LE(abs(cost_new - cost_expected), 1e-5);

}
