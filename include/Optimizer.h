//
// Created by hongzhe on 3/7/22.
//

#ifndef MPVI_OPTIMIZER_H
#define MPVI_OPTIMIZER_H

#endif //MPVI_OPTIMIZER_H

#include <gtsam/base/Matrix.h>
#include <iostream>
#include <random>
#include <utility>
#include "SparseInverseMatrix.h"
#include "OptimizerFactorized.h"
#include <boost/scoped_ptr.hpp>

using namespace GaussianSampler;
using namespace std;
using namespace SparseInverse;
typedef Triplet<double> T;

template <typename Function, typename costClass, typename... Args>

// template function and classes to calculate the costs
class VariationalInferenceMPOptimizer{
    using FactorizedOptimizer = VariationalInferenceMPOptimizerFactorized<Function, costClass, Args...>;

public:
    VariationalInferenceMPOptimizer(const int& dimension, const int& sub_dim, const vector<Function>& _vec_function,
                                   const vector<costClass>& _vec_cost_class, const vector<MatrixXd>& _vec_Pks):
                                   dim{dimension},
                                   sub_dim{sub_dim},
                                   num_sub_vars{_vec_Pks.size()},
                                   vec_cost_function_{_vec_function},
                                   vec_cost_class_{_vec_cost_class},
                                   vec_Pks_{_vec_Pks},
                                   mu_{VectorXd::Zero(dimension)},
                                   d_mu_{VectorXd::Zero(dimension)},
                                   precision_{MatrixXd::Identity(dim, dim) * 5.0},
//                                   precision_{MatrixXd::Identity(dim, dim)},
                                   d_precision_(MatrixXd::Identity(dim, dim)),
                                   Vdmu_{VectorXd::Zero(dimension)},
                                   Vddmu_(MatrixXd::Identity(dim, dim)),
                                   inverser_{MatrixXd::Identity(dim, dim)}{

        /// initialize the factors
        for (int i=0; i<num_sub_vars; i++){

            FactorizedOptimizer optimizer_k{sub_dim, vec_cost_function_[i], vec_cost_class_[i]};
            vec_factor_optimizers_.emplace_back(optimizer_k);
        }
    }
protected:
    // optimization variables
    int dim, sub_dim, num_sub_vars;

    VectorXd mu_, Vdmu_, d_mu_;
    MatrixXd precision_, Vddmu_, d_precision_;

    // sampler
    vector<FactorizedOptimizer> vec_factor_optimizers_;

    // cost functional. Input: samples vector; Output: cost
    const vector<Function> vec_cost_function_;
    vector<costClass> vec_cost_class_;
    const vector<MatrixXd> vec_Pks_;

    // Sparse matrix inverse helper
//    sparse_inverser inverser_;
    dense_inverser inverser_;
    double step_size_precision = 0.9;
    double step_size_mu = 0.9;

public:

    void step(){
        cout << "mu_ " << endl << mu_ << endl;
//        cout << "new precision " << endl << precision_ << endl;

        Vdmu_.setZero();
        Vddmu_.setZero();
        d_mu_.setZero();
        d_precision_.setZero();

        MatrixXd Sigma{inverser_.inverse(precision_)};

        for (int k=0; k<num_sub_vars; k++){

            const MatrixXd& Pk = vec_Pks_[k];

            auto &optimizer_k = vec_factor_optimizers_[k];
//            cout << "mu_k" << endl << Pk * mu_ << endl;
            optimizer_k.updateSamplerMean(VectorXd{Pk * mu_});

            optimizer_k.updateSamplerCovarianceMatrix(MatrixXd{Pk * Sigma * Pk.transpose()});

            optimizer_k.update_mu(VectorXd{Pk*mu_});
            optimizer_k.update_precision(MatrixXd{(Pk * Sigma * Pk.transpose()).inverse()});

            optimizer_k.calculate_partial_V();

//            optimizer_k.step();

            Vdmu_ = Vdmu_ + Pk.transpose() * optimizer_k.get_Vdmu();
            Vddmu_ = Vddmu_ + Pk.transpose().eval() * optimizer_k.get_Vddmu() * Pk;

        }

        d_precision_ = -precision_ + Vddmu_;

        precision_ = precision_ + step_size_precision*d_precision_;
//        precision_sparse_ = precision_.sparseView();
//        SparseQR<SpMatrix, Eigen::NaturalOrdering<int>> qr_solver(precision_sparse_);
//        d_mu_ = qr_solver.solve(-Vdmu_);

        d_mu_ = precision_.colPivHouseholderQr().solve(-Vdmu_);


        mu_ = mu_ + step_size_mu * d_mu_;

//        cout << "d_precision_" << endl << d_precision_ << endl;
//        cout << "d_mean" << endl << d_mu_ << endl;
//
//        cout << "precision" << endl << precision_ << endl;
//        cout << "mu" << endl << mu_ << endl;
    }

    void step_closed_form(){

        Vdmu_.setZero();
        Vddmu_.setZero();
        d_mu_.setZero();
        d_precision_.setZero();

        MatrixXd Sigma{inverser_.inverse(precision_)};

        for (int k=0; k<num_sub_vars; k++){

            const MatrixXd& Pk = vec_Pks_[k];

            auto &optimizer_k = vec_factor_optimizers_[k];
            optimizer_k.updateSamplerMean(VectorXd{Pk * mu_});
            optimizer_k.updateSamplerCovarianceMatrix(MatrixXd{Pk * Sigma * Pk.transpose()});

            optimizer_k.update_mu(VectorXd{Pk*mu_});
            optimizer_k.update_precision(MatrixXd{(Pk * Sigma * Pk.transpose()).inverse()});

            // closed form verification for a Gaussian posterior
            auto &cost_class_k = vec_cost_class_[k];
            optimizer_k.calculate_exact_partial_V(cost_class_k.get_mean(), cost_class_k.get_covariance());

            Vdmu_ = Vdmu_ + Pk.transpose() * optimizer_k.get_Vdmu();
            Vddmu_ = Vddmu_ + Pk.transpose().eval() * optimizer_k.get_Vddmu() * Pk;
        }

        d_precision_ = -precision_ + Vddmu_;

        precision_ = precision_ + step_size_precision*d_precision_;
//        precision_sparse_ = precision_.sparseView();
//        SparseQR<SpMatrix, Eigen::NaturalOrdering<int>> qr_solver(precision_sparse_);
//        d_mu_ = qr_solver.solve(-Vdmu_);

        d_mu_ = precision_.colPivHouseholderQr().solve(-Vdmu_);
        mu_ = mu_ + step_size_mu * d_mu_;

        cout << "mu_ " << endl << mu_ << endl;
        cout << "new precision " << endl << precision_ << endl;

    }

    gtsam::Vector get_mean(){
        return mu_;
    }

    gtsam::Matrix get_precision(){
        return precision_;
    }

    gtsam::Matrix get_covariance(){
        return inverser_.inverse();
    }

    void set_step_size(double ss_mean, double ss_precision){
        step_size_mu = ss_mean;
        step_size_precision = ss_precision;
    }

    void set_mu(const VectorXd& mean){
        mu_ = mean;
    }

};
