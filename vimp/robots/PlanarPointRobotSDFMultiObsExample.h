/**
 * @file PlanarPointRobotSDFMultiObsExample.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief An example experiment settings of a planar point robot in multi obstacle env. 
 * Imported from the tested code in gpmp2.
 * @version 0.1
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#define STRING(x) #x
#define XSTRING(x) STRING(x)

#include <gpmp2/kinematics/PointRobotModel.h>
#include <gtsam/inference/Symbol.h>
#include <gpmp2/obstacle/PlanarSDF.h>
#include "helpers/MatrixIO.h"

using namespace std;
using namespace Eigen;
using namespace vimp;
using namespace gtsam;

namespace vimp{


class PlanarPointRobotSDFMultiObsExample{
    public:
        PlanarPointRobotSDFMultiObsExample(const string& map_name="map0"){
            std::string map_file;
            std::string field_file;
            /// map and sdf
            std::string source_root{XSTRING(SOURCE_ROOT)};
            if (strcmp(map_name.data(), "map0") == 0){
                std::string map_file{source_root+"/maps/2dpR/map3/map_multiobs_map3.csv"};
                std::string field_file{source_root+"/maps/2dpR/map3/field_multiobs_map3.csv"};
            }
            else if(strcmp(map_name.data(), "map1") == 0){
                std::string map_file{source_root+"/maps/2dpR/map1/map_multiobs_map3.csv"};
                std::string field_file{source_root+"/maps/2dpR/map1/field_multiobs_map3.csv"};
            }
            else if(strcmp(map_name.data(), "map2") == 0){
                std::string map_file{source_root+"/maps/2dpR/map2/map_multiobs_map3.csv"};
                std::string field_file{source_root+"/maps/2dpR/map2/field_multiobs_map3.csv"};
            }
            else if(strcmp(map_name.data(), "map3") == 0){
                std::string map_file{source_root+"/maps/2dpR/map3/map_multiobs_map3.csv"};
                std::string field_file{source_root+"/maps/2dpR/map3/field_multiobs_map3.csv"};
            }
            else{
                std::runtime_error("No such map for 2d point robot sdf!");
            }

            MatrixXd map_ground_truth = _matrix_io.load_csv(map_file);
            _field = _matrix_io.load_csv(field_file);

            // layout of SDF: Bottom-left is (0,0), length is +/- cell_size per grid.
            Point2 origin(-20, -10);
            double cell_size = 0.1;

            _sdf = gpmp2::PlanarSDF(origin, cell_size, _field);

            /// Robot model
            gpmp2::PointRobot pR(_ndof, _nlinks);
            double r = 1.5;
            gpmp2::BodySphereVector body_spheres;
            body_spheres.push_back(gpmp2::BodySphere(0, r, Point3(0.0, 0.0, 0.0)));
            _pR_model = gpmp2::PointRobotModel(pR, body_spheres);

        }

        public:
            gpmp2::PointRobotModel _pR_model = gpmp2::PointRobotModel();
            gpmp2::PlanarSDF _sdf = gpmp2::PlanarSDF();
            MatrixXd _field;
            MatrixIO _matrix_io = MatrixIO();

            /// 2D point robot
            int _ndof = 2;
            int _nlinks = 1;

        public:
            gpmp2::PointRobotModel pRmodel() const { return _pR_model; }
            gpmp2::PlanarSDF sdf() const { return _sdf; }
            int ndof() const {return _ndof;}
            int nlinks() const {return _nlinks;}
            MatrixXd field() const {return _field;}
        
};
}