/**
 * @file conv_prior_posvel_col_pR.cpp
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Test the convergence of the algorithm with prior (pos + vel) + collision cost 
 * only on supported states, for a planar robot.
 * @version 0.1
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#define STRING(x) #x
#define XSTRING(x) STRING(x)

#include "helpers/ExperimentRunner.h"
#include "instances/GVIMPPlanarPRSDF.h"
using namespace vimp;

int main(int argc, char* argv[]){
    std::string source_root{XSTRING(SOURCE_ROOT)};
    int nx = 4, nu = 2, num_exp = 4;
    GVIMPParams params;
    // no experiment argument, run the default scripts
    if (argc == 1){
        num_exp = 2;
        std::string config_file{source_root+"/configs/vimp/planar_pR_map1_new.xml"};
        GVIMPRunner<GVIMPPlanarPRSDF> runner(nx, nu, num_exp, config_file);
        runner.read_config(params);
        runner.run();
        return 0;
    }
    // arguments: i_exp, params:(i_exp, eps, eps_sdf, speed, nt, sig0, sigT, eta, stop_err, max_iter, cost_sig)
    else if (argc == 2){
        num_exp = 4;
        std::string config_relative = static_cast<std::string>(argv[1]);
        std::cout << "config_relative " << std::endl << config_relative << std::endl;
        
        std::string config_file{source_root+"/"+config_relative};
        GVIMPRunner<GVIMPPlanarPRSDF> runner(nx, nu, num_exp, config_file);
        runner.read_config(params);
        runner.run();
        return 0;
    }
    else{
        std::runtime_error("Wrong number of arguments!");
    }
    return 0;
}