/**
 * @file pgcs_ArmPlanarSDF.cpp
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief pgcs with plannar obstacles and arm robot, 
 * using Robot Model which has a vector of balls to check collisions.
 * @version 0.1
 * @date 2023-03-31
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#define STRING(x) #x
#define XSTRING(x) STRING(x)

#include "covariance_steering/PGCSLinDynArmPlanarSDF.h"
#include "helpers/ExperimentRunner.h"

using namespace Eigen;
using namespace vimp;

int main(int argc, char* argv[]){
    int nx=4, nu=2, num_exp=2;
    std::string source_root{XSTRING(SOURCE_ROOT)};
    std::string config_file{source_root+"/configs/pgcs/planar_2link_arm_map2.xml"};
    PGCSRunner<PGCSLinArmPlanarSDF> runner(nx, nu, num_exp, config_file);

    // no experiment argument, run the default scripts
    if (argc == 1){
        runner.run();
        return 0;
    }
    // arguments: i_exp, params:(i_exp, eps, eps_sdf, speed, nt, sig0, sigT, eta, stop_err, max_iter, cost_sig)
    else if (argc == 15){

        int i_exp = std::stoi(argv[1]);
        double eps = std::stof(argv[2]);
        double eps_sdf = std::stof(argv[3]);
        double radius = std::stof(argv[4]);
        double speed = std::stof(argv[8]);
        int nt = std::stof(argv[9]);
        double sig0 = std::stof(argv[10]);
        double sigT = std::stof(argv[11]);
        double eta = std::stof(argv[12]);
        double stop_err = std::stof(argv[13]);
        int max_iter = std::stof(argv[14]);
        double sig_obs = std::stof(argv[15]);
        double backtrack_ratio = std::stof(argv[16]);
        int backtrack_iterations = std::stoi(argv[17]);

        PGCSParams params(nx, nu, eps_sdf, radius, eps, speed, nt, 
                          sig0, sigT, eta, stop_err, sig_obs, 
                          max_iter, backtrack_ratio, backtrack_iterations);
        runner.run_one_exp(i_exp, params);
        return 0;
    }
    else{
        std::runtime_error("Wrong number of arguments!");
    }

}