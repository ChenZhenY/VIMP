/**
 * @file OptimizerPriorPlanarPointRobot.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief 
 * @version 0.1
 * @date 2022-07-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../include/OptimizerGH.h"
#include "../include/GaussianPriorUnaryTranslation.h"
#include "../include/OptimizerFactorizedGH.h"

using namespace MPVI;
using namespace gpmp2;
using namespace Eigen;

/// Description: The definition of joint optimizer for planar point robot with only prior factors.

/// Function calculating two cost factors: prior and the collision factor. 
using UnaryFactorTranslation2D = UnaryFactorTranslation<gtsam::Vector2>;

/**
 * @brief Declaration of the factorized optimizer.
 * 
 */
using OptimizerFactorPriorPRGH = VIMPOptimizerFactorizedGaussHermite<std::function<double(const VectorXd&, const UnaryFactorTranslation2D&)>,
                                                                    UnaryFactorTranslation2D>;

