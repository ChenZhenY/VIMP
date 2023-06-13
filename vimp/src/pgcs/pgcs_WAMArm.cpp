/**
 * @file pgcs_pR_RobotModel.cpp
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief pgcs with plannar obstacles and arm robot, 
 * using Robot Model which has a vector of balls to check collisions.
 * @version 0.1
 * @date 2023-03-31
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "covariance_steering/PGCSLinDynArmSDF.h"
#include "helpers/ExperimentRunner7D.h"

using namespace vimp;

int main(int argc, char* argv[]){
    
    int nx = 14, nu=7;

    int num_exp = 3;
    std::string config_file{"/home/hongzhe/git/VIMP/vimp/configs/pgcs/wam_arm.xml"};
    ExperimentRunner7D<PGCSLinDynArmSDF> runner(num_exp, config_file);
    // no experiment argument, run the default scripts
    if (argc == 1){
        runner.run();
        return 0;
    }
    // arguments: i_exp, params:(i_exp, eps, eps_sdf, speed, nt, sig0, sigT, eta, stop_err, max_iter, cost_sig)
    else if (argc == 12){
        
        int i_exp = std::stoi(argv[1]);
        double eps = std::stof(argv[2]);
        double eps_sdf = std::stof(argv[3]);
        double speed = std::stof(argv[4]);
        int nt = std::stof(argv[5]);
        double sig0 = std::stof(argv[6]);
        double sigT = std::stof(argv[7]);
        double eta = std::stof(argv[8]);
        double stop_err = std::stof(argv[9]);
        int max_iter = std::stof(argv[10]);
        double sig_obs = std::stof(argv[11]);

        ExperimentParams param(nx, nu, eps_sdf, eps, speed, nt, sig0, sigT, eta, stop_err, sig_obs, max_iter);
        return runner.run_one_exp(i_exp, param);
    }
    else{
        std::runtime_error("Wrong number of arguments!");
    }
    

}