/**
 * @file GVIFactorizedBase.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief The base class for marginal optimizer.
 * @version 0.1
 * @date 2022-03-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once


#include <iostream>
#include <random>
#include <utility>
#include <assert.h>
#include <random>

#include "GaussHermite.h"
#include "helpers/sparse_graph.h"
#include "helpers/timer.h"


using namespace std;
using namespace Eigen;

IOFormat CleanFmt(4, 0, ", ", "\n");

namespace vimp{
    class GVIFactorizedBase{
    public:
        virtual ~GVIFactorizedBase(){}
        /**
         * @brief Default Constructor
         */
        GVIFactorizedBase(){}

        /**
         * @brief Construct a new GVIFactorizedBase object
         * 
         * @param dimension The dimension of the state
         */
        GVIFactorizedBase(int dimension, int state_dim, int num_states, int start_index, 
                          double temperature=1.0, double high_temperature=10.0, bool is_linear=false):
                _is_linear{is_linear},
                _dim{dimension},
                _state_dim{state_dim},
                _num_states{num_states},
                _temperature{temperature},
                _high_temperature{high_temperature},
                _mu(_dim),
                _covariance{MatrixXd::Identity(_dim, _dim)},
                _precision{MatrixXd::Identity(_dim, _dim)},
                _dprecision(_dim, _dim),
                _Vdmu(_dim),
                _Vddmu(_dim, _dim),
                _block{state_dim, num_states, start_index, dimension},
                _Pk(dimension, state_dim*num_states)
                {   
                    _joint_size = state_dim * num_states;
                    _Pk.setZero();
                    _Pk.block(0, start_index*state_dim, dimension, dimension) = Eigen::MatrixXd::Identity(dimension, dimension);
                }        
        
    /// Public members for the inherited classes access
    public:

        bool _is_linear;

        /// dimension
        int _dim, _state_dim, _num_states, _joint_size;

        VectorXd _mu;
        
        /// Intermediate functions for Gauss-Hermite quadratures, default definition, needed to be overrided by the
        /// derived classes.
        using GHFunction = std::function<MatrixXd(const VectorXd&)>;
        GHFunction _func_phi;
        GHFunction _func_Vmu;
        GHFunction _func_Vmumu;

        std::shared_ptr<GHFunction> _func_phi_T;
        std::shared_ptr<GHFunction> _func_Vmu_T;
        std::shared_ptr<GHFunction> _func_Vmumu_T;

        // std::shared_ptr<GHFunction> _func_phi_highT;
        // std::shared_ptr<GHFunction> _func_Vmu_highT;
        // std::shared_ptr<GHFunction> _func_Vmumu_highT;

        /// G-H quadrature class
        using GH = GaussHermite<GHFunction> ;
        std::shared_ptr<GH> _gh;

    protected:
        /// intermediate variables in optimization steps
        VectorXd _Vdmu;
        MatrixXd _Vddmu;

        /// optimization variables
        MatrixXd _precision, _dprecision;
        MatrixXd _covariance;

        // Sparse inverser and matrix helpers
        EigenWrapper _ei;

    private:
        /// step sizes
        double _step_size_mu = 0.9;
        double _step_size_Sigma = 0.9;

        double _temperature, _high_temperature;
        
        // sparse mapping to sub variables
        TrajectoryBlock _block;

        MatrixXd _Pk;

    /// public functions
    public:
        // void construct_function_T(){
        //     _func_phi_T = std::make_shared<GHFunction>([this](const VectorXd& x){return (*(this->_func_phi))(x) / this->_temperature;});
        //     _func_Vmu_T = std::make_shared<GHFunction>([this](const VectorXd& x){return (*(this->_func_Vmu))(x) / this->_temperature;});
        //     _func_Vmumu_T = std::make_shared<GHFunction>([this](const VectorXd& x){return (*(this->_func_Vmumu))(x) / this->_temperature;});
        // }

        /// update the GH approximator
        void updateGH(const VectorXd& x, const MatrixXd& P){
            _gh->update_mean(x);
            _gh->update_P(P); }

        /**
         * @brief Update the step size
         */
        inline void set_step_size(double ss_mean, double ss_precision){
            _step_size_mu = ss_mean;
            _step_size_Sigma = ss_precision; }

        /**
         * @brief Update mean
         */
        inline void update_mu(const VectorXd& new_mu){ 
            _mu = new_mu; }

        /**
         * @brief Update covariance matrix
         */
        inline void update_covariance(const MatrixXd& new_cov){ 
            _covariance = new_cov; 
            _precision = _covariance.inverse();
        }

        inline MatrixXd Pk(){
            return _Pk;
        }

        /**
         * @brief Update the marginal mean.
         */
        inline void update_mu_from_joint(const VectorXd & joint_mean) {
            _mu = _block.extract_vector(joint_mean);
        }

        inline VectorXd extract_mu_from_joint(const VectorXd & joint_mean) {
            VectorXd res(_dim);
            res = _block.extract_vector(joint_mean);
            return res;
        }

        inline SpMat extract_cov_from_joint(const SpMat& joint_covariance) {
            return _block.extract(joint_covariance);
             ;
        }

        /**
         * @brief Update the marginal precision matrix.
         */
        inline void update_precision_from_joint(const SpMat& joint_covariance) {
            _covariance = _block.extract(joint_covariance);
            _precision = _covariance.inverse();
        }

        /**
         * @brief Calculating phi * (partial V) / (partial mu), and 
         * phi * (partial V^2) / (partial mu * partial mu^T)
         */
        virtual void calculate_partial_V(){
            // update the mu and sigma inside the gauss-hermite integrator
            updateGH(_mu, _covariance);

            /// Integrate for E_q{_Vdmu} 
            VectorXd Vdmu{VectorXd::Zero(_dim)};

            Vdmu = _gh->Integrate(_func_Vmu);
            Vdmu = _precision * Vdmu;

            /// Integrate for E_q{phi(x)}
            double E_phi = _gh->Integrate(_func_phi)(0, 0);
            
            /// Integrate for partial V^2 / ddmu_ 
            MatrixXd E_xxphi{_gh->Integrate(_func_Vmumu)};

            MatrixXd Vddmu{MatrixXd::Zero(_dim, _dim)};
            Vddmu.triangularView<Upper>() = (_precision * E_xxphi * _precision - _precision * E_phi).triangularView<Upper>();
            Vddmu.triangularView<StrictlyLower>() = Vddmu.triangularView<StrictlyUpper>().transpose();

            // update member variables
            _Vdmu = Vdmu;
            _Vddmu = Vddmu;
        }

        void test_integration(){
            std::cout << "=========== test_integration ===========" << std::endl;
            updateGH(_mu, _covariance);
            double E_phi = _gh->Integrate(_func_phi)(0, 0);
            std::cout << "E_phi " << std::endl << E_phi << std::endl;

            VectorXd Vdmu = _gh->Integrate(_func_Vmu);
            _ei.print_matrix(Vdmu, "E_xphi");

            MatrixXd E_xxphi = _gh->Integrate(_func_Vmumu);
            _ei.print_matrix(E_xxphi, "E_xxphi");
        }

        void calculate_partial_V_GH(){
            // update the mu and sigma inside the gauss-hermite integrator
            updateGH(_mu, _covariance);

            /// Integrate for E_q{_Vdmu} 
            VectorXd Vdmu = _gh->Integrate(_func_Vmu);
            Vdmu = _precision * Vdmu;

            /// Integrate for E_q{phi(x)}
            double E_phi = _gh->Integrate(_func_phi)(0, 0);
            
            /// Integrate for partial V^2 / ddmu_ 
            MatrixXd E_xxphi{_gh->Integrate(_func_Vmumu)};

            MatrixXd Vddmu{MatrixXd::Zero(_dim, _dim)};
            Vddmu.triangularView<Upper>() = (_precision * E_xxphi * _precision - _precision * E_phi).triangularView<Upper>();
            Vddmu.triangularView<StrictlyLower>() = Vddmu.triangularView<StrictlyUpper>().transpose();

            // update member variables
            _Vdmu = Vdmu;
            _Vddmu = Vddmu;
        }

        /**
         * @brief Compute the cost function. V(x) = E_q(\phi(x))
         */
        virtual double fact_cost_value(const VectorXd& x, const MatrixXd& Cov) {
            updateGH(x, Cov);
            return _gh->Integrate(_func_phi)(0, 0);
        }

        /**
         * @brief Compute the cost function. V(x) = E_q(\phi(x)) using the current values.
         */
        virtual double fact_cost_value(){
            return fact_cost_value(_mu, _covariance);
        }

        /**
         * @brief Get the marginal intermediate variable (partial V^2 / par mu / par mu)
         */
        inline MatrixXd Vddmu() const { return _Vddmu; }

        /**
         * @brief Get the marginal intermediate variable partial V / dmu
         */
        inline VectorXd Vdmu() const { return _Vdmu; }

        /**
         * @brief Get the joint intermediate variable (partial V / partial mu).
         */
        inline VectorXd joint_Vdmu_sp() { 
            VectorXd res(_joint_size);
            res.setZero();
            _block.fill_vector(res, _Vdmu);
            // _ei.print_matrix(res, "joint Vdmu");
            // _ei.print_matrix(_Vdmu, "Vdmu");
            // _ei.print_matrix(Pk(), "Pk");
            return res;
        }

        // inline VectorXd joint_Vdmu() { 
        //     return _Vdmu;
        // }

        /**
         * @brief Get the joint Pk.T * V^2 / dmu /dmu * Pk using block insertion
         */
        inline SpMat joint_Vddmu_sp() { 
            SpMat res(_joint_size, _joint_size);
            res.setZero();
            _block.fill(_Vddmu, res);

            // _ei.print_matrix(_Vddmu, "_Vddmu");
            // _ei.print_matrix(Pk(), "Pk");
            // _ei.print_matrix(Pk()*res*Pk().transpose(), "reconstruct Vddmu");
            // std::cout << "diff norm local Vddmu" << std::endl <<
            // (Pk()*res*Pk().transpose() - _Vddmu).norm() << std::endl;

            return res;
        }

        /**
         * @brief Get the mapping matrix Pk
         */
        inline TrajectoryBlock block() const {return _block;}

        /**
         * @brief Get the mean 
         */
        inline VectorXd mean() const{ return _mu; }


        /**
         * @brief Get the precision matrix
         */
        inline MatrixXd precision() const{ 
            assert((_precision - _covariance.inverse()).norm()==0); 
            return _precision; }


        /**
         * @brief Get the covariance matrix
         */
        inline MatrixXd covariance() const{ return _covariance;}

        /********************************************************/
        /// Function interfaces

        /**
         * @brief returns the Phi(x) 
         */
        inline MatrixXd Phi(const VectorXd& x) const{
            return _func_phi(x);
        }

        /**
         * @brief returns the (x-mu)*Phi(x) 
         */
        inline MatrixXd xMuPhi(const VectorXd& x) const{
            return _func_Vmu(x);
        }

        /**
         * @brief returns the (x-mu)*Phi(x) 
         */
        inline MatrixXd xMuxMuTPhi(const VectorXd& x) const{
            return _func_Vmumu(x);
        }

        /**
         * @brief returns the E_q{phi(x)} = E_q{-log(p(x,z))}
         */
        inline double E_Phi() {
            return _gh->Integrate(_func_phi)(0, 0);
        }

        inline MatrixXd E_xMuPhi(){
            return _gh->Integrate(_func_Vmu);
        }

        inline MatrixXd E_xMuxMuTPhi(){
            return _gh->Integrate(_func_Vmumu);
        }

        void set_GH_points(int p){
            _gh->set_polynomial_deg(p);
        }
    };

}