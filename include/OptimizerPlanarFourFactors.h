//
// Created by hongzhe on 3/7/22.
//

#ifndef MPVI_OPTIMIZERFOURFACTORS_H
#define MPVI_OPTIMIZERFOURFACTORS_H

#include <gtsam/base/Matrix.h>
#include <iostream>
#include <random>
#include <utility>
#include "SparseInverseMatrix.h"
#include "OptimizerFactorizedTwoFactors.h"
#include "OptimizerFactorizedOneFactor.h"
#include <boost/scoped_ptr.hpp>

#endif //MPVI_OPTIMIZER_H

using namespace GaussianSampler;
using namespace std;
using namespace SparseInverse;
typedef Triplet<double> T;

namespace MPVI {
    template<class Function_Interp, class Function_Prior, class Function_Collision, class PriorFactorClass, class CollisionFactorClass, class PriorFactorClassInterp, class CollisionFactorClassInterp, class... Args>
    class VIMPOptimizerFourFactors {
        using FactorizedOptimizerTwoFactors = VIMPOptimizerFactorizedTwoFactors<Function_Interp, PriorFactorClassInterp, CollisionFactorClassInterp, Args...>;
        using FactorizedOptimizerOnePriorFactor = VIMPOptimizerFactorizedOneFactor<Function_Prior, PriorFactorClass, Args...>;
        using FactorizedOptimizerOneCollisionFactor = VIMPOptimizerFactorizedOneFactor<Function_Collision, CollisionFactorClass, Args...>;

    public:
        VIMPOptimizerFourFactors(int dimension,
                                 vector<FactorizedOptimizerTwoFactors> _vec_optimizer_interp,
                                 vector<FactorizedOptimizerOnePriorFactor> _vec_optimizer_one_prior,
                                 vector<FactorizedOptimizerOneCollisionFactor> _vec_optimizer_one_collision):
                dim{dimension},
                mu_{VectorXd::Zero(dimension)},
                d_mu_{VectorXd::Zero(dimension)},
                precision_{MatrixXd::Identity(dim, dim) * 5.0},
                d_precision_{MatrixXd::Identity(dim, dim)},
                Vdmu_{VectorXd::Zero(dimension)},
                Vddmu_{MatrixXd::Identity(dim, dim)},
                inverser_{MatrixXd::Identity(dim, dim)},
                vec_optimizer_interp_{_vec_optimizer_interp},
                vec_optimizer_one_prior_{_vec_optimizer_one_prior},
                vec_optimizer_one_collision_{_vec_optimizer_one_collision},
                num_opti_interp{_vec_optimizer_interp.size()},
                num_opti_prior{_vec_optimizer_one_prior.size()},
                num_opti_collision{_vec_optimizer_one_collision.size()} {}

    protected:
        // optimization variables
        int dim, num_opti_interp, num_opti_prior, num_opti_collision;

        VectorXd mu_, Vdmu_, d_mu_;
        MatrixXd precision_, Vddmu_, d_precision_;

//    sparse_inverser inverser_;
        dense_inverser inverser_;
        double step_size_precision = 0.9;
        double step_size_mu = 0.9;

        vector<FactorizedOptimizerTwoFactors> vec_optimizer_interp_;
        vector<FactorizedOptimizerOnePriorFactor> vec_optimizer_one_prior_;
        vector<FactorizedOptimizerOneCollisionFactor> vec_optimizer_one_collision_;

    public:

        void step() {
            cout << "mu_ " << endl << mu_ << endl;
//        cout << "new precision " << endl << precision_ << endl;

            Vdmu_.setZero();
            Vddmu_.setZero();
            d_mu_.setZero();
            d_precision_.setZero();

            MatrixXd Sigma{inverser_.inverse(precision_)};

            for (int k = 0; k < num_opti_interp; k++) {

                auto &optimizer_k = vec_optimizer_interp_[k];

                optimizer_k.updateSamplerMean(VectorXd{optimizer_k.Pk() * mu_});

                optimizer_k.updateSamplerCovarianceMatrix(
                        MatrixXd{optimizer_k.Pk() * Sigma * optimizer_k.Pk().transpose()});

                optimizer_k.update_mu(VectorXd{optimizer_k.Pk() * mu_});
                optimizer_k.update_precision(
                        MatrixXd{(optimizer_k.Pk() * Sigma * optimizer_k.Pk().transpose()).inverse()});

                optimizer_k.calculate_partial_V();

                Vdmu_ = Vdmu_ + optimizer_k.Pk().transpose() * optimizer_k.get_Vdmu();
                Vddmu_ = Vddmu_ + optimizer_k.Pk().transpose().eval() * optimizer_k.get_Vddmu() * optimizer_k.Pk();

            }

            cout << "finish step 1" << endl;

            for (int k = 0; k < num_opti_prior; k++) {

                auto &optimizer_k = vec_optimizer_one_prior_[k];

                optimizer_k.updateSamplerMean(VectorXd{optimizer_k.Pk() * mu_});

                optimizer_k.updateSamplerCovarianceMatrix(
                        MatrixXd{optimizer_k.Pk() * Sigma * optimizer_k.Pk().transpose()});

                optimizer_k.update_mu(VectorXd{optimizer_k.Pk() * mu_});
                optimizer_k.update_precision(
                        MatrixXd{(optimizer_k.Pk() * Sigma * optimizer_k.Pk().transpose()).inverse()});

                optimizer_k.calculate_partial_V();

                Vdmu_ = Vdmu_ + optimizer_k.Pk().transpose() * optimizer_k.get_Vdmu();
                Vddmu_ = Vddmu_ + optimizer_k.Pk().transpose().eval() * optimizer_k.get_Vddmu() * optimizer_k.Pk();

            }

            cout << "finish step 2" << endl;

            for (int k = 0; k < num_opti_collision; k++) {

                auto &optimizer_k = vec_optimizer_one_collision_[k];

                optimizer_k.updateSamplerMean(VectorXd{optimizer_k.Pk() * mu_});

                optimizer_k.updateSamplerCovarianceMatrix(
                        MatrixXd{optimizer_k.Pk() * Sigma * optimizer_k.Pk().transpose()});

                optimizer_k.update_mu(VectorXd{optimizer_k.Pk() * mu_});
                optimizer_k.update_precision(
                        MatrixXd{(optimizer_k.Pk() * Sigma * optimizer_k.Pk().transpose()).inverse()});

                optimizer_k.calculate_partial_V();

                Vdmu_ = Vdmu_ + optimizer_k.Pk().transpose() * optimizer_k.get_Vdmu();
                Vddmu_ = Vddmu_ + optimizer_k.Pk().transpose().eval() * optimizer_k.get_Vddmu() * optimizer_k.Pk();

            }

            cout << "finish step" << endl;

            d_precision_ = -precision_ + Vddmu_;

            precision_ = precision_ + step_size_precision * d_precision_;

            d_mu_ = precision_.colPivHouseholderQr().solve(-Vdmu_);

            mu_ = mu_ + step_size_mu * d_mu_;

            cout << "d_precision_" << endl << d_precision_ << endl;
            cout << "d_mean" << endl << d_mu_ << endl;

            cout << "precision" << endl << precision_ << endl;
            cout << "mu" << endl << mu_ << endl;
        }

//    void step_closed_form(){
//
//        Vdmu_.setZero();
//        Vddmu_.setZero();
//        d_mu_.setZero();
//        d_precision_.setZero();
//
//        MatrixXd Sigma{inverser_.inverse(precision_)};
//
//        for (int k=0; k<num_sub_vars; k++){
//
//            MatrixXd Pk = vec_Pks_[k];
//
//            auto &optimizer_k = vec_factor_optimizers_[k];
//            optimizer_k.updateSamplerMean(VectorXd{Pk * mu_});
//            optimizer_k.updateSamplerCovarianceMatrix(MatrixXd{Pk * Sigma * Pk.transpose()});
//
//            optimizer_k.update_mu(VectorXd{Pk*mu_});
//            optimizer_k.update_precision(MatrixXd{(Pk * Sigma * Pk.transpose()).inverse()});
//
//            // closed form verification for a Gaussian posterior
//            auto &cost_class_k = vec_collision_factor_class_[k];
//            optimizer_k.calculate_exact_partial_V(cost_class_k.get_mean(), cost_class_k.get_covariance());
//
//            Vdmu_ = Vdmu_ + Pk.transpose() * optimizer_k.get_Vdmu();
//            Vddmu_ = Vddmu_ + Pk.transpose().eval() * optimizer_k.get_Vddmu() * Pk;
//        }
//
//        d_precision_ = -precision_ + Vddmu_;
//
//        precision_ = precision_ + step_size_precision*d_precision_;
////        precision_sparse_ = precision_.sparseView();
////        SparseQR<SpMatrix, Eigen::NaturalOrdering<int>> qr_solver(precision_sparse_);
////        d_mu_ = qr_solver.solve(-Vdmu_);
//
//        d_mu_ = precision_.colPivHouseholderQr().solve(-Vdmu_);
//        mu_ = mu_ + step_size_mu * d_mu_;
//
//        cout << "mu_ " << endl << mu_ << endl;
//        cout << "new precision " << endl << precision_ << endl;
//
//    }

        gtsam::Vector get_mean() {
            return mu_;
        }

        gtsam::Matrix get_precision() {
            return precision_;
        }

        gtsam::Matrix get_covariance() {
            return inverser_.inverse();
        }

        void set_step_size(double ss_mean, double ss_precision) {
            step_size_mu = ss_mean;
            step_size_precision = ss_precision;
        }

        void set_mu(const VectorXd &mean) {
            mu_ = mean;
        }

    };
}
