/**
 * @file PGCSLinDynArmModelPlanarSDF.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Proximal gradient algorithm for nonlinear covariance steering with plannar obstacles, arm robot model. 
 * @version 0.1
 * @date 2023-03-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "pgcsmp/ProximalGradientCSLinearDyn.h"
#include "robots/PlanarArm2SDF_pgcs.h"
#include "pgcsmp/PGCSLinDynRobotSDF.h"

using namespace Eigen;

namespace vimp{

using PGCSLinArmPlanarSDF = PGCSLinDynRobotSDF<PlanarArm2SDFExample>;

}