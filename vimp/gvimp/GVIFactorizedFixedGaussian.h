/**
 * @file GVIFactorizedFixedGaussian.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Factorized optimization steps for fixed linear gaussian factors 
 * -log(p(x|z)) = ||X - \mu_t||_{\Sigma_t^{-1}},
 *  which has closed-form expression in computing gradients wrt variables.
 * The difference from the linear dynamics prior is that for this factor, 
 * temperature and high temperature are both 1.0;
 * @version 0.1
 * @date 2023-08-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "GVIFactorizedBase.h"
#include "gp/linear_factor.h"

namespace vimp{
    template <typename LinearFactor>
    class GVIFactorizedFixedGaussian : public GVIFactorizedBase{
        using Base = GVIFactorizedBase;
        using CostFunction = std::function<double(const VectorXd&, const LinearFactor&)>;
    public:
        GVIFactorizedFixedGaussian(const int& dimension,
                                    int dim_state,
                                    const CostFunction& function, 
                                    const LinearFactor& linear_factor,
                                    int num_states,
                                    int start_indx):
            Base(dimension, dim_state, num_states, start_indx, 1.0, 1.0, true),
            _linear_factor{linear_factor}
            {
                Base::_func_phi = [this, function, linear_factor](const VectorXd& x){return MatrixXd::Constant(1, 1, function(x, linear_factor));};
                Base::_func_Vmu = [this, function, linear_factor](const VectorXd& x){return (x-Base::_mu) * function(x, linear_factor);};
                Base::_func_Vmumu = [this, function, linear_factor](const VectorXd& x){return MatrixXd{(x-Base::_mu) * (x-Base::_mu).transpose() * function(x, linear_factor)};};
                
                Base::_func_phi_highT = [this](const VectorXd& x){return Base::_func_phi(x);};
                Base::_func_Vmu_highT = [this](const VectorXd& x){return Base::_func_Vmu(x);};
                Base::_func_Vmumu_highT = [this](const VectorXd& x){return Base::_func_Vmumu(x);};
                

                using GH = GaussHermite<GHFunction>;
                Base::_gh = std::make_shared<GH>(GH{6, dimension, Base::_mu, Base::_covariance, Base::_func_phi});

                _target_mean = linear_factor.get_mu();
                _target_precision = linear_factor.get_precision();
                _Lambda = linear_factor.get_Lambda();
                _Psi = linear_factor.get_Psi();
                _const_multiplier = linear_factor.get_C();
            }

    protected:
        LinearFactor _linear_factor;

        MatrixXd _target_mean, _target_precision, _Lambda, _Psi;

        double _const_multiplier = 1.0;

    public:
        /*Calculating phi * (partial V) / (partial mu), and 
         * phi * (partial V^2) / (partial mu * partial mu^T) for Gaussian posterior: closed-form expression:
         * (partial V) / (partial mu) = Sigma_t{-1} * (mu_k - mu_t)
         * (partial V^2) / (partial mu)(partial mu^T): higher order moments of a Gaussian.
        */
        // void calculate_partial_V() override{
        //     // helper vectors
        //     MatrixXd tmp{MatrixXd::Zero(_dim, _dim)};

        //     // partial V / partial mu           
        //     _Vdmu = _const_multiplier * (2 * _Lambda.transpose() * _target_precision * (_Lambda*_mu - _Psi*_target_mean));
            
        //     MatrixXd AT_precision_A = _Lambda.transpose() * _target_precision * _Lambda;

        //     // partial V^2 / partial mu*mu^T
        //     // update tmp matrix
        //     for (int i=0; i<(_dim); i++){
        //         for (int j=0; j<(_dim); j++) {
        //             for (int k=0; k<(_dim); k++){
        //                 for (int l=0; l<(_dim); l++){
        //                     tmp(i, j) += (_covariance(i, j) * (_covariance(k, l)) + _covariance(i,k) * (_covariance(j,l)) + _covariance(i,l)*_covariance(j,k))*AT_precision_A(k,l);
        //                 }
        //             }
        //         }
        //     }

        //     _Vddmu = _const_multiplier * (_precision * tmp * _precision - _precision * (AT_precision_A*_covariance).trace());
        // }

        // double fact_cost_value() {
        //     return fact_cost_value(_mu, _covariance);
        // }

        // double fact_cost_value(const VectorXd& x, const MatrixXd& Cov) override {
        //     return _const_multiplier * ((_Lambda.transpose()*_target_precision*_Lambda * Cov).trace() + 
        //             (_Lambda*x - _Psi*_target_mean).transpose() * _target_precision * (_Lambda*x - _Psi*_target_mean));
        // }

    };
}