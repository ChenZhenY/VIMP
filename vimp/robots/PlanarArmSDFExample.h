/**
 * @file PlanarArmSDFExample.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief A commonly used planar arm model with sdf. 
 * Imported from the tested code in gpmp2.
 * @version 0.1
 * @date 2022-08-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <gpmp2/kinematics/ArmModel.h>
#include <gpmp2/obstacle/PlanarSDF.h>

using namespace gpmp2;
using namespace Eigen;

namespace vimp{

    class PlanarArmSDFExample{
        public:
            PlanarArmSDFExample(){
                
                _field = _matrix_io.load_csv("/home/hongzhe/git/VIMP/vimp/data/2d_Arm/field.csv");

                Point2 origin(-1, -1);
                double cell_size = 0.01;

                _sdf = PlanarSDF(origin, cell_size, _field);

                // 2 link simple example
                Pose3 arm_base(Rot3(), Point3(0.05, 0.15, 0.05));
                Vector2d a(0.1, 0.2), alpha(0, 0), d(0, 0);
                Arm abs_arm(2, a, alpha, d, arm_base);

                // body info, three spheres
                BodySphereVector body_spheres;
                const double r = 0.05;
                body_spheres.push_back(BodySphere(0, r, Point3(-0.1, 0, 0)));
                body_spheres.push_back(BodySphere(0, r, Point3(0, 0, 0)));
                body_spheres.push_back(BodySphere(1, r, Point3(-0.1, 0, 0)));
                body_spheres.push_back(BodySphere(1, r, Point3(0, 0, 0)));
                _arm = ArmModel{abs_arm, body_spheres};

            }

            ArmModel arm_model(){
                return _arm;
            }

            PlanarSDF sdf() const { return _sdf; }
            int ndof() const {return _ndof;}
            int nlinks() const {return _nlinks;}
            MatrixXd field() const {return _field;}

        private:
            ArmModel _arm;
            PlanarSDF _sdf;
            MatrixXd _field;
            MatrixIO _matrix_io;

            /// Arm robot
            int _ndof = 1;
            int _nlinks = 2;
    };

}// namespace