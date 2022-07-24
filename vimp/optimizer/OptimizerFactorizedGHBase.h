/**
 * @file OptimizerFactorizedGHBase.h
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

#include <vimp/helpers/SparseMatrixHelper.h>
#include <vimp/helpers/GaussHermite.h>


using namespace std;
using namespace Eigen;

IOFormat CleanFmt(4, 0, ", ", "\n");

namespace vimp{
    class VIMPOptimizerFactorizedBase{
    public:
        
        /**
         * @brief Default Constructor
         */
        VIMPOptimizerFactorizedBase(){}

        /**
         * @brief Construct a new VIMPOptimizerFactorizedBase object
         * 
         * @param dimension The dimension of the state
         * @param Pk_ Mapping matrix from marginal to joint
         */
        VIMPOptimizerFactorizedBase(const int& dimension, const MatrixXd& Pk):
                _dim{dimension},
                _mu{VectorXd::Zero(_dim)},
                _covariance{MatrixXd::Identity(_dim, _dim)},
                _precision{MatrixXd::Identity(_dim, _dim)},
                _dprecision{MatrixXd::Zero(_dim, _dim)},
                _Vdmu{VectorXd::Zero(_dim)},
                _Vddmu{MatrixXd::Zero(_dim, _dim)},
                _Pk{Pk}
                {}
    
    
    /// Public members for the inherited classes access
    public:

        /// dimension
        int _dim;

        VectorXd _mu;
        MatrixXd _covariance;
        
        /// Intermediate functions for Gauss-Hermite quadratures, default definition, needed to be overrided by the
        /// derived classes.
        using GHFunction = std::function<MatrixXd(const VectorXd&)>;
        GHFunction _func_phi = GHFunction();
        GHFunction _func_Vmu = GHFunction();
        GHFunction _func_Vmumu = GHFunction();

        /// G-H quadrature class
        GaussHermite<GHFunction> _gauss_hermite = GaussHermite<GHFunction>();

    private:
        /// optimization variables
        MatrixXd _precision, _dprecision;

        /// intermediate variables in optimization steps
        VectorXd _Vdmu;
        MatrixXd _Vddmu;

        /// step sizes
        double _step_size_mu = 0.9;
        double _step_size_Sigma = 0.9;

        /// Mapping matrix to the joint distribution
        MatrixXd _Pk;

    /// public functions
    public:
        /// update the GH approximator
        void updateGH(){
            _gauss_hermite.update_mean(_mu);
            update_covariance();
            _gauss_hermite.update_P(_covariance); }

        /// update the GH approximator
        void updateGH(const VectorXd& x, const MatrixXd& P){
            _gauss_hermite.update_mean(x);
            _gauss_hermite.update_P(P); }


        /**
         * @brief Update the step size
         * 
         * @param ss_mean step size for updating mean
         * @param ss_precision step size for updating covariance
         */
        inline void set_step_size(double ss_mean, double ss_precision){
            _step_size_mu = ss_mean;
            _step_size_Sigma = ss_precision; }


        /**
         * @brief Update mean, covariance, and precision matrix
         * 
         * @param new_mu a given mean
         */
        inline void update_mu(const VectorXd& new_mu){ assert(_mu.size() == new_mu.size()); _mu = new_mu; }


        /**
         * @brief Update precision matrix
         * 
         * @param new_mu a given precision matrix
         */
        inline void update_precision(const MatrixXd& new_precision){ assert(_precision.size() == new_precision.size()); _precision = new_precision; }


        /**
         * @brief Update the marginal mean using JOINT mean and 
         * mapping matrix Pk.
         * 
         * @param joint_mean 
         */
        inline void update_mu_from_joint_mean(const VectorXd& joint_mean){ 
            _mu = _Pk * joint_mean;}
            

        /**
         * @brief Update the marginal precision matrix using joint COVARIANCE matrix and 
         * mapping matrix Pk.
         * 
         * @param joint_covariance a given JOINT covariance matrix
         */
        inline void update_precision_from_joint_covariance(const MatrixXd& joint_covariance){ 
            _covariance = _Pk * joint_covariance * _Pk.transpose();
            _precision = _covariance.inverse();}

        /**
         * @brief Update covariance matrix by inverting precision matrix.
         */
        inline void update_covariance(){ _covariance = _precision.inverse();}
        

        /**
         * @brief Main function calculating phi * (partial V) / (partial mu), and 
         * phi * (partial V^2) / (partial mu * partial mu^T)
         * 
         * @return * void 
         */
        void calculate_partial_V();


        /**
         * @brief Main function calculating phi * (partial V) / (partial mu), and 
         * phi * (partial V^2) / (partial mu * partial mu^T) for Gaussian posterior: closed-form expression
         * 
         * @param mu_t target mean vector
         * @param covariance_t target covariance matrix
         */
        void calculate_exact_partial_V(VectorXd mu_t, MatrixXd covariance_t);


        /**
         * @brief One step in the optimization.
         * @return true: success.
         */
        bool step();

        
        /**
         * @brief Compute the cost function. V(x) = E_q(\phi(x))
         * 
         * @param x the state to compute cost on.
         * @return cost value 
         */
        double cost_value(const VectorXd& x, const MatrixXd& Cov);


        /**
         * @brief Compute the cost function. V(x) = E_q(\phi(x)) using the current values.
         * 
         * @return cost value 
         */
        double cost_value();


        /**
         * @brief Get the marginal intermediate variable (partial V^2 / par mu / par mu)
         * 
         * @return MatrixXd (par V^2 / par mu / par mu)
         */
        inline MatrixXd Vddmu() const { return _Vddmu; }


        /**
         * @brief Get the marginal intermediate variable partial V / dmu
         * 
         * @return VectorXd (par V / par mu)
         */
        inline VectorXd Vdmu() const { return _Vdmu; }


        /**
         * @brief Get the joint intermediate variable (partial V / partial mu).
         * 
         * @return VectorXd Pk.T * (par V / par mu)
         */
        inline VectorXd joint_Vdmu() const { return _Pk.transpose() * _Vdmu; }


        /**
         * @brief Get the joint intermediate variable Vddmu
         * 
         * @return MatrixXd Pk.T * V^2 / dmu /dmu * Pk
         */
        inline MatrixXd joint_Vddmu() const { return _Pk.transpose().eval() * _Vddmu * _Pk; }


        /**
         * @brief Get the mapping matrix Pk
         * 
         * @return MatrixXd Pk
         */
        inline MatrixXd Pk() const { return _Pk; }


        /**
         * @brief Get the mean 
         * 
         * @return VectorXd 
         */
        inline VectorXd mean() const{ return _mu; }


        /**
         * @brief Get the precision matrix
         * 
         * @return MatrixXd 
         */
        inline MatrixXd precision() const{ return _precision; }


        /**
         * @brief Get the joint mean 
         * 
         * @return VectorXd 
         */
        inline VectorXd joint_mean() const{ return _Pk.transpose() * _mu; }


        /**
         * @brief Get the joint precision matrix
         * 
         * @return MatrixXd 
         */
        inline MatrixXd joint_precision() const{ return _Pk.transpose() * _precision * _Pk; }


        /**
         * @brief Get the joint covariance matrix
         * 
         * @return MatrixXd 
         */
        inline MatrixXd joint_covariance() const{ return _Pk.transpose() * covariance() * _Pk; }


        /**
         * @brief Get the covariance matrix
         * 
         * @return MatrixXd 
         */
        inline MatrixXd covariance() const{ return _precision.inverse();}

    };

}
#include <vimp/optimizer/OptimizerFactorizedGHBase-impl.h>