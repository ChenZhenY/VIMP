#include "../gp/fixed_prior.h"
#include "../gp/minimum_acc_prior.h"
#include <gpmp2/obstacle/ObstaclePlanarSDFFactor.h>
#include <gpmp2/obstacle/ObstacleSDFFactor.h>

#include "../3rd-part/rapidxml-1.13/rapidxml.hpp"
#include "../3rd-part/rapidxml-1.13/rapidxml_utils.hpp"

/**
 * @brief Fixed cost with a covariance
 * @param x input
 * @param fixed_gp 
 * @return double cost
 */
double cost_fixed_gp(const VectorXd& x, const vimp::FixedPriorGP& fixed_gp){
    return fixed_gp.cost(x);
}


/**
 * @brief cost for the linear gaussian process.
 * @param pose : the combined two secutive states. [x1; v1; x2; v2]
 * @param gp_minacc : the linear gp object
 * @return double, the cost (-logporb)
 */
double cost_linear_gp(const VectorXd& pose_cmb, const vimp::MinimumAccGP& gp_minacc){
    int dim = gp_minacc.dim_posvel();
    return gp_minacc.cost(pose_cmb.segment(0, dim), pose_cmb.segment(dim, dim));
}

/**
 * Obstacle factor: planar case
 * */
template <typename ROBOT>
double cost_obstacle_planar(const VectorXd& pose, 
                    const gpmp2::ObstaclePlanarSDFFactor<ROBOT>& obs_factor){
    VectorXd vec_err = obs_factor.evaluateError(pose);

    MatrixXd precision_obs{MatrixXd::Identity(vec_err.rows(), vec_err.rows())};
    precision_obs = precision_obs / obs_factor.get_noiseModel()->sigmas()[0];

    // std::cout << "cost_obstacle_planar " << std::endl;

    return vec_err.transpose().eval() * precision_obs * vec_err;

}


/**
 * Obstacle factor
 * */
template <typename ROBOT>
double cost_obstacle(const VectorXd& pose, 
                    const gpmp2::ObstacleSDFFactor<ROBOT>& obs_factor){
    std::cout << "cost_obstacle_planar " << std::endl;
    VectorXd vec_err = obs_factor.evaluateError(pose);

    // MatrixXd precision_obs;
    MatrixXd precision_obs{MatrixXd::Identity(vec_err.rows(), vec_err.rows())};
    precision_obs = precision_obs / obs_factor.get_noiseModel()->sigmas()[0];

    return vec_err.transpose().eval() * precision_obs * vec_err;

}