using namespace Eigen;
using namespace std;
#include <stdexcept>

namespace vimp{
    
    /**
     * @brief optimize with backtracking
     */
    template <typename FactorizedOptimizer>
    void VIMPOptimizerGH<FactorizedOptimizer>::optimize(){
        double new_cost = 0.0;
        double cost_iter =  0.0;
        for (int i_iter = 0; i_iter < _niters; i_iter++) {
            cout << "========= iteration " << i_iter << " ========= "<< endl;
            // ============= Collect results ============= 
            VectorXd fact_costs_iter = factor_costs();
            cost_iter = cost_value();
            cout << "cost: " << cost_iter << endl;
            _res_recorder.update_data(_mu, _covariance, _precision, cost_iter, fact_costs_iter);

            // one step
            _Vdmu.setZero();
            _Vddmu.setZero();

            _timer.start();
            for (auto& opt_k : _vec_factors){
                // if (opt_k->_is_linear){
                //     opt_k->calculate_partial_V();
                //     // opt_k->calculate_partial_V_GH();
                // }
                opt_k->calculate_partial_V();
                _Vdmu = _Vdmu + opt_k->joint_Vdmu_sp();
                _Vddmu = _Vddmu + opt_k->joint_Vddmu_sp();
            }
            _Vdmu = _Vdmu / _temperature;
            _Vddmu = _Vddmu / _temperature;

            _timer.end("Vdmu and Vddmu time: ");
            
            SpMat dprecision = -_precision + _Vddmu;
            VectorXd dmu = _eigen_wrapper.solve_cgd_sp(_Vddmu, -_Vdmu);

            int cnt = 1;
            const int MAX_ITER = 10;
            double step_size;

            SpMat new_precision;
            VectorXd new_mu;

            while (true){
                
                step_size = pow(_step_size_base, cnt);

                new_precision = _precision + step_size * dprecision;
                new_mu  = _mu + step_size * dmu;
                
                new_cost = cost_value(new_mu, new_precision);
                if (new_cost < cost_iter){ break; }

                if (cnt > MAX_ITER){
                    // throw std::runtime_error(std::string("Too many iterations in the backtracking ... Dead"));
                    cout << "Too many iterations in the backtracking ... Dead" << endl;
                    break;
                }
                cnt += 1;
            }
            cout << "step size " << endl << step_size << endl;

            /// update the variables
            set_mu(new_mu);
            set_precision(new_precision);

            /// small cost decrease, stop iterations. 
            double STOP_SIGN = 1e-5;
            if (cost_iter - new_cost < STOP_SIGN){
                cout << "--- Cost Decrease less than threshold ---" << endl << cost_iter - new_cost << endl;
                cout << "--- number of backtrackings ---" << endl << cnt << endl;
                save_data();
                /// see a purturbed cost
                cout << "=== final cost ===" << endl << cost_iter << endl;
                return ;
            }
        }

        save_data();

        /// see a purturbed cost
        cout << "=== final cost ===" << endl << cost_iter << endl;
    } 


    template <typename FactorizedOptimizer>
    inline void VIMPOptimizerGH<FactorizedOptimizer>::set_precision(const SpMat& new_precision){
        _precision = new_precision;
        // sparse inverse
        inverse_inplace();

        for (auto & factor : _vec_factors){
            factor->update_precision_from_joint(_covariance);
        }
    }

    /**
     * @brief Compute the costs of all factors for a given mean and cov.
     */
    template <typename FactorizedOptimizer>
    VectorXd VIMPOptimizerGH<FactorizedOptimizer>::factor_costs(const VectorXd& x, const SpMat& Precision) const{
        VectorXd fac_costs(_nfactors);
        fac_costs.setZero();
        int cnt = 0;
        SpMat Cov = inverse(Precision);
        for (auto& opt_k : _vec_factors){
            VectorXd x_k = opt_k->extract_mu_from_joint(x);

            MatrixXd Cov_k = opt_k->extract_cov_from_joint(Cov);
            fac_costs(cnt) = opt_k->fact_cost_value(x_k, Cov_k) / _temperature;
            cnt += 1;
        }
        return fac_costs;
    }


    /**
     * @brief Compute the costs of all factors, using current values.
     */
    template <typename FactorizedOptimizer>
    VectorXd VIMPOptimizerGH<FactorizedOptimizer>::factor_costs() const{
        VectorXd fac_costs(_nfactors);
        fac_costs.setZero();
        int cnt = 0;
        
        Timer timer;
        for (auto& opt_k : _vec_factors){
            fac_costs(cnt) = opt_k->fact_cost_value();
            cnt += 1;
        }
        return fac_costs  / _temperature;
    }

    /**
     * @brief Compute the total cost function value given a state.
     */
    template <typename FactorizedOptimizer>
    double VIMPOptimizerGH<FactorizedOptimizer>::cost_value(const VectorXd& x, SpMat& Precision){
        
        SpMat Cov = inverse(Precision);

        double value = 0.0;
        for (auto& opt_k : _vec_factors){
            VectorXd x_k = opt_k->extract_mu_from_joint(x);
            MatrixXd Cov_k = opt_k->extract_cov_from_joint(Cov);

            value += opt_k->fact_cost_value(x_k, Cov_k) / _temperature;
        }
        SparseLDLT ldlt(Precision);
        double det = ldlt.determinant();
        if (det < 0){ std::runtime_error(std::string("Infinity log determinant precision matrix ...")); }
        value += log(det) / 2;
        return value;
    }

    /**
     * @brief Compute the total cost function value given a state, using current values.
     */
    template <typename FactorizedOptimizer>
    double VIMPOptimizerGH<FactorizedOptimizer>::cost_value() const{
        double det = _ldlt.determinant();
        if (0 >  det){
            std::__throw_out_of_range("precision matrix not psd ...");
        }
        return cost_value_no_entropy() + log(det) / 2;
    }

    /**
     * @brief given a state, compute the total cost function value without the entropy term, using current values.
     */
    template <typename FactorizedOptimizer>
    double VIMPOptimizerGH<FactorizedOptimizer>::cost_value_no_entropy() const{
        double value = 0.0;
        for (auto& opt_k : _vec_factors){
            value += opt_k->fact_cost_value();
        }
        return value  / _temperature;
    }

}
