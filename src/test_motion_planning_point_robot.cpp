//
// Created by Hongzhe Yu on 3/20/22.
//

#include "../include/OptimizerPlanarPointRobot.h"
#include <gtsam/base/numericalDerivative.h>
#include <gtsam/inference/Symbol.h>
#include "../include/matplotlibcpp.h"
#include <gpmp2/obstacle/ObstaclePlanarSDFFactorGPPointRobot.h>
#include <gpmp2/obstacle/ObstaclePlanarSDFFactorPointRobot.h>
#include "../include/GaussianPriorLinearPlus.h"
#include "../include/GaussianPriorPose2Plus.h"

using namespace gtsam;
using namespace std;
using namespace GaussianSampler;
using namespace gpmp2;
using namespace Eigen;
namespace plt = matplotlibcpp;

/**
 * The definition of error funciton h(x) using the cost class 'factor'.
 * Form: f(Args ..., cost class).
 * **/
inline double errorWrapperInterp(const gtsam::Vector& theta,
        const GaussianPriorLinearPlus& prior_factor,
        const ObstaclePlanarSDFFactorGPPointRobot& collision_factor_interp) {
    VectorXd conf1{theta.segment<2>(0)};
    VectorXd vel1{theta.segment<2>(2)};
    VectorXd conf2{theta.segment<2>(4)};
    VectorXd vel2{theta.segment<2>(6)};

    // collision cost on the support states
    VectorXd vec_err = collision_factor_interp.evaluateError(conf1, vel1, conf2, vel2);

    MatrixXd precision_obs{MatrixXd::Identity(vec_err.rows(), vec_err.rows())};
    double sig_obs = 1.0;
    precision_obs = precision_obs / sig_obs;

    double collision_cost = vec_err.transpose() * precision_obs * vec_err;

    VectorXd vec_prior_err = prior_factor.evaluateError(conf1, vel1, conf2, vel2);
    MatrixXd Qi = calcQ(prior_factor.get_Qc(), prior_factor.get_delta_t());

    double prior_err = vec_prior_err.transpose() * Qi.inverse() * vec_prior_err;

    return collision_cost + prior_err;
}

inline double errorWrapperSinglePrior(const gtsam::Vector& theta, const GaussianPriorPose2Plus & prior_factor) {
    VectorXd conf1{theta.segment<2>(0)};
    VectorXd vel1{theta.segment<2>(2)};
    VectorXd conf2{theta.segment<2>(4)};
    VectorXd vel2{theta.segment<2>(6)};

    VectorXd vec_prior_err = prior_factor.evaluateError(conf1, vel1, conf2, vel2);
    MatrixXd Qi = calcQ(prior_factor.get_Qc(), prior_factor.get_delta_t());

    double prior_err = vec_prior_err.transpose() * Qi.inverse() * vec_prior_err;

    return prior_err;
}

inline double errorWrapperSingleCollision(const gtsam::Vector& theta, const ObstaclePlanarSDFFactorPointRobot& obstacle_factor) {

    VectorXd vec_err = obstacle_factor.evaluateError(theta);

    MatrixXd precision_obs{MatrixXd::Identity(vec_err.rows(), vec_err.rows())};
    double sig_obs = 1.0;
    precision_obs = precision_obs / sig_obs;

    double collision_cost = vec_err.transpose() * precision_obs * vec_err;

    return collision_cost;
}

// convert sdf vector to hinge loss err vector
inline gtsam::Vector convertSDFtoErr(const gtsam::Vector& sdf, double eps) {
    gtsam::Vector err_ori = 0.0 - sdf.array() + eps;
    return (err_ori.array() > 0.0).select(err_ori, gtsam::Vector::Zero(err_ori.rows()));  // (R < s ? P : Q)
}

/**
 * Plot the iteration results
 * input: results: (num_iteration, num_states)
 * output: figure
**/
void plot_result(const MatrixXd& results, int N, int dim_theta){
    // plotting in the 2D case
    plt::figure();
    MatrixXd x(results.rows(), N), y(results.rows(), N);
    vector<double> vec_x, vec_y;
    vec_x.resize(x.cols());
    vec_y.resize(x.cols());

    for (int i=0; i<N; i++){
        x.col(i) = results.col(i*dim_theta);
        y.col(i) = results.col(i*dim_theta+1);
    }
    for (int k=x.rows()-1; k<x.rows(); k++){
        VectorXd::Map(&vec_x[0], x.cols()) = VectorXd{x.row(k)};
        VectorXd::Map(&vec_y[0], y.cols()) = VectorXd{y.row(k)};
        plt::plot(vec_x, vec_y, "--");
    }

    // plot the start and goal positions
//    VectorXd::Map(&vec_x[0], x.cols()) = VectorXd{x.row(-1)};
//    VectorXd::Map(&vec_y[0], y.cols()) = VectorXd{y.row(-1)};
//    vector<double> x_start{vec_x[0]}, y_start{vec_y[0]};
//    plt::plot(x_start, y_start, "r*");

    cout << "x" << endl << x << endl;
    cout << "y" << endl << y << endl;

    plt::grid(true);
    plt::show();
}

void test_point_robot(){
    /* ************************************************************************** */
// signed distance field data
    gtsam::Matrix map_ground_truth = (gtsam::Matrix(7, 7) <<
            0,     0,     0,     0,     0,     0,     0,
            0,     0,     0,     0,     0,     0,     0,
            0,     0,     1,     1,     1,     0,     0,
            0,     0,     1,     1,     1,     0,     0,
            0,     0,     1,     1,     1,     0,     0,
            0,     0,     0,     0,     0,     0,     0,
            0,     0,     0,     0,     0,     0,     0).finished();
    gtsam::Matrix field = (gtsam::Matrix(7, 7) <<
            2.8284,    2.2361,    2.0000,    2.0000,    2.0000,    2.2361,    2.8284,
            2.2361,    1.4142,    1.0000,    1.0000,    1.0000,    1.4142,    2.2361,
            2.0000,    1.0000,   -1.0000,   -1.0000,   -1.0000,    1.0000,    2.0000,
            2.0000,    1.0000,   -1.0000,   -2.0000,   -1.0000,    1.0000,    2.0000,
            2.0000,    1.0000,   -1.0000,   -1.0000,   -1.0000,    1.0000,    2.0000,
            2.2361,    1.4142,    1.0000,    1.0000,    1.0000,    1.4142,    2.2361,
            2.8284,    2.2361,    2.0000,    2.0000,    2.0000,    2.2361,    2.8284).finished();
// bottom-left is (0,0), length is +/- 1 per point.
    Point2 origin(0, 0);
    double cell_size = 1.0;

    PlanarSDF sdf = PlanarSDF(origin, cell_size, field);

    // 2D point robot
    double total_time = 10.0;
    int num_support_states = 5, num_interp = 2, N=num_support_states+1;

    const int ndof = 2, nlinks = 1, nspheres = 1;
    const int dim_theta = 2 * ndof * nlinks;
    const int ndim = dim_theta * N;

    double delta_t = total_time / num_support_states, tau = delta_t / num_interp;
    double obs_eps = 0.2, obs_sigma = 1.0;

    PointRobot pR(ndof, nlinks);

    double r = 0.5;

    BodySphereVector body_spheres;
    body_spheres.push_back(BodySphere(0, r, Point3(0.0, 0.0, 0.0)));

    PointRobotModel pRModel(pR, body_spheres);

    // just check cost of two link joint
    gtsam::Matrix H1_act, H2_act, H3_act, H4_act;

    // origin zero and stationary case
    // initialize the mean by linear interpolation
    double start_x = 0.0, start_y = 0.0, goal_x = 5.5, goal_y = 0.0;
    VectorXd start_conf(dim_theta);
    start_conf << start_x, start_y, 0, 0;
    VectorXd goal_conf(dim_theta);
    goal_conf << goal_x, goal_y, 0, 0;

    VectorXd init_mean{VectorXd::Zero(ndim)};
    for (int j=0; j<N; j++){
        init_mean.segment<dim_theta>(j*dim_theta) = start_conf + double(j)*(goal_conf - start_conf)/num_support_states;
    }

    vector<std::function<double(const gtsam::Vector&, const GaussianPriorLinearPlus&, const ObstaclePlanarSDFFactorPointRobot&, const ObstaclePlanarSDFFactorGPPointRobot&)>> vec_cost_functions;
    vector<ObstaclePlanarSDFFactorGPPointRobot> vec_collision_classes;
    vector<GaussianPriorLinearPlus> vec_prior_classes;
    vector<gtsam::Matrix> vec_Pks;

    using OptimizerOnePrior = VariationalInferenceMPOptimizerFactorizedOneFactor<std::function<double(const gtsam::Vector&, const GaussianProcessPriorPose2&)>,
            const GaussianProcessPriorPose2&,
            const gtsam::Vector&>;
    using OptimizerOneCollision = VariationalInferenceMPOptimizerFactorizedOneFactor<std::function<double(const gtsam::Vector&, const ObstaclePlanarSDFFactorPointRobot&)>,
            const ObstaclePlanarSDFFactorPointRobot&,
            const gtsam::Vector&>;
    using OptimizerInterp = VariationalInferenceMPOptimizerFactorizedTwoFactors<std::function<double(const gtsam::Vector&, const GaussianPriorLinearPlus&, const ObstaclePlanarSDFFactorGPPointRobot&)>,
            const GaussianPriorLinearPlus&,
            const ObstaclePlanarSDFFactorGPPointRobot&,
            const gtsam::Vector&>;

    vector<OptimizerOnePrior> vec_optimizer_one_priors;
    vector<OptimizerOneCollision> vec_optimizer_one_collision;
    vector<OptimizerInterp> vec_optimizer_interp;

    SharedNoiseModel Qc_model = noiseModel::Isotropic::Sigma(2, 1.0);
    cout << "Qc" << endl << getQc(Qc_model) << endl;

    SharedNoiseModel K_0 = noiseModel::Isotropic::Sigma(2, 1.0);
    SharedNoiseModel K_N = noiseModel::Isotropic::Sigma(2, 1.0);

    for (int i=0; i<N-1; i++){

        if (i==0 or i==N-1){
            // prior at start and end pose
            GaussianPriorPose2Plus factor_prior_single{symbol((unsigned char) 'x', i),
                                                       symbol((unsigned char) 'v', i),
                                                       symbol((unsigned char) 'x', i+1),
                                                       symbol((unsigned char) 'v', i+1),
                                                       delta_t,
                                                       Qc_model};
            vec_optimizer_one_priors.emplace_back(OptimizerOnePrior{})

        }

        // collision cost classes
        ObstaclePlanarSDFFactorGPPointRobot factor_collision_interp{symbol((unsigned char) 'x', i),
                                                                          symbol((unsigned char) 'v', i),
                                                                          symbol((unsigned char) 'x', i+1),
                                                                          symbol((unsigned char) 'v', i+1),
                                                                          pRModel,
                                                                          sdf,
                                                                          obs_sigma,
                                                                          obs_eps,
                                                                          Qc_model,
                                                                          delta_t,
                                                                          tau};
        // prior cost classes
        GaussianPriorLinearPlus factor_prior_interp{symbol((unsigned char) 'x', i),
                                                                  symbol((unsigned char) 'v', i),
                                                                  symbol((unsigned char) 'x', i+1),
                                                                  symbol((unsigned char) 'v', i+1),
                                                                  delta_t,
                                                                  Qc_model};

        // single cost factor


        vec_cost_functions.emplace_back(errorWrapperInterp);
        // Pk matrices
        MatrixXd Pk{MatrixXd::Zero(2*dim_theta, ndim)};
        Pk.block<dim_theta, dim_theta>(0, i*dim_theta) = MatrixXd::Identity(dim_theta, dim_theta);
        Pk.block<dim_theta, dim_theta>(dim_theta, (i+1)*dim_theta) = MatrixXd::Identity(dim_theta, dim_theta);
        vec_Pks.emplace_back(Pk);
//        cout << "Pk" << endl << Pk << endl;
    }

    // declare the optimizer
    // template <typename Function, typename PriorFactorClass, typename CollisionFactorClass, typename... Args>
    VariationalInferenceMPOptimizerTwoFactors<std::function<double(const gtsam::Vector&, const GaussianPriorLinearPlus&, const ObstaclePlanarSDFFactorGPPointRobot&)>,
            GaussianPriorLinearPlus,
            ObstaclePlanarSDFFactorGPPointRobot, gtsam::Vector> optimizer(ndim, 2*dim_theta, vec_cost_functions, vec_prior_classes, vec_collision_classes, vec_Pks);
    optimizer.set_mu(init_mean);

    const int num_iter = 8;
    double step_size = 0.9;

    MatrixXd results{MatrixXd::Zero(num_iter, ndim)};

    for (int i=0; i<num_iter; i++) {
        step_size = step_size / pow((i + 1), 1 / 3);
        optimizer.set_step_size(step_size, step_size);
        VectorXd mean_iter{optimizer.get_mean()};
        cout << "==== iteration " << i << " ====" << endl;
        for (int j=0; j<N; j++){
            cout <<"position mean" << endl << mean_iter.segment<2>(j*dim_theta) << endl;
        }
        results.row(i) = optimizer.get_mean().transpose();
        optimizer.step();
    }

    plot_result(results, N, dim_theta);
}


int main(){
    test_point_robot();
    return 0;
}

