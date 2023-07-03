/**
 * @file GVIMPPlanarPRSDF.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief The optimizer for planar robots at the joint level.
 * @version 0.1
 * @date 2023-06-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "GVIMPPlanarRobotSDF.h"
#include "../robots/PlanarPointRobotSDF_pgcs.h"

namespace vimp{
    using GVIMPPlanarPRSDF = GVIMPPlanarRobotSDF<gpmp2::PointRobotModel, PlanarPRSDFExample>;

}