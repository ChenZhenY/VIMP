/**
 * @file run_experiment_template.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief The template of running a motion planning experiment.
 * @version 0.1
 * @date 2023-04-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "experimentParam.h"
#include "../3rd-part/rapidxml-1.13/rapidxml.hpp"
#include "../3rd-part/rapidxml-1.13/rapidxml_utils.hpp"

using namespace Eigen;

namespace vimp{

template <typename PGCSOptimizer>
class ExperimentRunner{
public:
    // ExperimentRunner(){}
    virtual ~ExperimentRunner(){}

    ExperimentRunner(int nx, int nu, int num_exp, const std::string & config): 
                    _nx(nx),
                    _nu(nu),
                    _params(nx, nu),
                    _num_exp(num_exp),
                    _config_file{config}{
                        read_config_file();
                    }

    void read_config_file(){
        rapidxml::file<> xmlFile(_config_file.data()); // Default template is char
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlFile.data());

        // Common parameters
        std::string CommonNodeName = "Commons";
        char * c_commons = CommonNodeName.data();
        rapidxml::xml_node<>* CommonNode = doc.first_node(c_commons);
        rapidxml::xml_node<>* commonParams = CommonNode->first_node("parameters");

        double eps = atoi(commonParams->first_node("eps")->value());
        double eps_sdf = atof(commonParams->first_node("eps_sdf")->value());
        double speed = atof(commonParams->first_node("speed")->value());
        _nt = atoi(commonParams->first_node("nt")->value());

        double sig0 = atof(commonParams->first_node("sig0")->value());
        double sigT = atof(commonParams->first_node("sigT")->value());

        double eta = atof(commonParams->first_node("eta")->value());
        double stop_err = atof(commonParams->first_node("stop_err")->value());
        int max_iterations = atoi(commonParams->first_node("max_iter")->value());
        double sig_obs = atof(commonParams->first_node("cost_sigma")->value());
        // std::string sdf_file = static_cast<std::string>(commonParams->first_node("sdf_file")->value());

        _params = ExperimentParams(_nx, _nu, eps_sdf, eps, speed, _nt, sig0, sigT, eta, stop_err, sig_obs, max_iterations);

    }

    virtual void read_boundary_conditions(const rapidxml::xml_node<>* paramNode){
        double start_x = atof(paramNode->first_node("start_pos")->first_node("x")->value());
        double start_y = atof(paramNode->first_node("start_pos")->first_node("y")->value());

        double start_vx = atof(paramNode->first_node("start_pos")->first_node("vx")->value());
        double start_vy = atof(paramNode->first_node("start_pos")->first_node("vy")->value());

        double goal_x = atof(paramNode->first_node("goal_pos")->first_node("x")->value());
        double goal_y = atof(paramNode->first_node("goal_pos")->first_node("y")->value());

        double goal_vx = atof(paramNode->first_node("goal_pos")->first_node("vx")->value());
        double goal_vy = atof(paramNode->first_node("goal_pos")->first_node("vy")->value());
        
        VectorXd m0(_nx), mT(_nx); 
        m0 << start_x, start_y, start_vx, start_vy;
        mT << goal_x, goal_y, goal_vx, goal_vy;

        _params.set_m0(m0);
        _params.set_mT(mT);
       
    }

    void run(){
        rapidxml::file<> xmlFile(_config_file.data()); // Default template is char
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlFile.data());

        for (int i=1; i<_num_exp+1; i++){
            std::string ExpNodeName = "Experiment" + std::to_string(i);
            char * c_expname = ExpNodeName.data();
            rapidxml::xml_node<>* ExpNode = doc.first_node(c_expname);
            rapidxml::xml_node<>* paramNode = ExpNode->first_node("parameters");            
            
            std::cout << "=== debug 0 ===" << std::endl;
            this->read_boundary_conditions(paramNode);
            MatrixXd A0(_nx, _nx), B0(_nx, _nu), a0(_nx, 1);
            A0.setZero(); B0.setZero(); a0.setZero();
            std::cout << "=== debug 0 ===" << std::endl;
            std::shared_ptr<ConstantVelDynamics> pdyn{new ConstantVelDynamics(_nx, _nu, _nt)};
            A0 = pdyn->A0() * _params.sig();
            B0 = pdyn->B0() * _params.sig();
            a0 = pdyn->a0() * _params.sig();
            std::cout << "=== debug 0 ===" << std::endl;
            PGCSOptimizer pgcs_lin_sdf(A0, a0, B0, pdyn, _params);
            std::cout << "=== debug 0 ===" << std::endl;
            std::tuple<MatrixXd, MatrixXd> res_Kd;
            res_Kd = pgcs_lin_sdf.optimize();
            std::cout << "=== debug 0 ===" << std::endl;
            MatrixXd Kt(_nx*_nx, _nt), dt(_nx, _nt);
            Kt = std::get<0>(res_Kd);
            dt = std::get<1>(res_Kd);
            MatrixXd zk_star(_nx, _nt), Sk_star(_nx*_nx, _nt);
            zk_star = pgcs_lin_sdf.zkt();
            Sk_star = pgcs_lin_sdf.Sigkt();
            std::cout << "=== debug 0 ===" << std::endl;
            std::string saving_prefix = static_cast<std::string>(paramNode->first_node("saving_prefix")->value());
            m_io.saveData(saving_prefix + std::string{"zk_sdf.csv"}, zk_star);
            m_io.saveData(saving_prefix + std::string{"Sk_sdf.csv"}, Sk_star);

            m_io.saveData(saving_prefix + std::string{"Kt_sdf.csv"}, Kt);
            m_io.saveData(saving_prefix + std::string{"dt_sdf.csv"}, dt);

        }
    }


public:
    
    MatrixIO m_io;
    ExperimentParams _params;
    std::string _config_file;

    int _num_exp, _nx, _nu, _nt;

};


}