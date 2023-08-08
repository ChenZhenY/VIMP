using namespace Eigen;
using namespace std;
#include <stdexcept>

#define STRING(x) #x
#define XSTRING(x) STRING(x)

namespace vimp
{
    
    /**
     * @brief One step of optimization.
     */
    template <typename Factor>
    std::tuple<VectorXd, SpMat> GVIGH<Factor>::compute_gradients(){
        _Vdmu.setZero();
        _Vddmu.setZero();

        for (auto &opt_k : _vec_factors)
        {
            opt_k->calculate_partial_V();
            _Vdmu = _Vdmu + opt_k->joint_Vdmu_sp();
            _Vddmu = _Vddmu + opt_k->joint_Vddmu_sp();
        }

        SpMat dprecision = _Vddmu - _precision;
        VectorXd dmu = _ei.solve_cgd_sp(_Vddmu, -_Vdmu);

        return std::make_tuple(dmu, dprecision);
    }

    /**
     * @brief optimize with backtracking
     */
    template <typename Factor>
    void GVIGH<Factor>::optimize()
    {
        double new_cost = 0.0;

        for (int i_iter = 0; i_iter < _niters; i_iter++)
        {
            cout << "========= iteration " << i_iter << " ========= " << endl;

            // ============= Cost at current iteration =============
            double cost_iter = cost_value(_mu, _precision);
            cout << "--- cost_iter ---" << endl << cost_iter << endl;

            // ============= Collect factor costs =============
            VectorXd fact_costs_iter = factor_cost_vector();

            _res_recorder.update_data(_mu, _covariance, _precision, cost_iter, fact_costs_iter);

            // gradients
            std::tuple<VectorXd, SpMat> dmudprecision = this->compute_gradients();


            VectorXd dmu = std::get<0>(dmudprecision);
            SpMat dprecision = std::get<1>(dmudprecision);

            _ei.print_matrix(dmu, "dmu");
            _ei.print_matrix(dprecision, "dprecision");
            
            int cnt = 0;
            int B = 1;
            double step_size = 0.0;

            SpMat new_precision; 
            VectorXd new_mu; 

            double new_cost = 1e9;

            // backtracking 
            while (new_cost > cost_iter)
            {
                new_mu.setZero(); new_precision.setZero();
                step_size = pow(_step_size_base, B);
                new_mu = _mu + step_size * dmu;
                new_precision = _precision + step_size * dprecision;
                new_cost = cost_value(new_mu, new_precision);

                // std::cout << "new_cost backtrack" << std::endl << new_cost << std::endl;

                cnt += 1;

                if (cnt > _niters_backtrack)
                {
                    // throw std::runtime_error(std::string("Too many iterations in the backtracking ... Dead"));
                    cout << "Too many iterations in the backtracking ... Dead" << endl;
                    break;
                }

                B += 1;
            }

            /// update mean and covariance
            set_mu(new_mu);
            set_precision(new_precision);

            cost_iter = new_cost;

            B = 1;

        }

        save_data();

        /// see a purturbed cost
        // cout << "=== final cost ===" << endl
        //      << cost_iter << endl;
    }


    template <typename Factor>
    inline void GVIGH<Factor>::set_precision(const SpMat &new_precision)
    {
        _precision = new_precision;
        // sparse inverse
        inverse_inplace();

        for (auto &factor : _vec_factors)
        {
            factor->update_precision_from_joint(_covariance);
        }
    }

    /**
     * @brief Compute the costs of all factors for a given mean and cov.
     */
    template <typename Factor>
    VectorXd GVIGH<Factor>::factor_cost_vector(const VectorXd& joint_mean, SpMat& joint_precision)
    {
        VectorXd fac_costs(_nfactors);
        fac_costs.setZero();
        int cnt = 0;
        SpMat joint_cov = inverse(joint_precision);
        for (auto &opt_k : _vec_factors)
        {
            fac_costs(cnt) = opt_k->fact_cost_value(joint_mean, joint_cov); // / _temperature;
            cnt += 1;
        }
        return fac_costs;
    }

    /**
     * @brief Compute the costs of all factors, using current values.
     */
    template <typename Factor>
    VectorXd GVIGH<Factor>::factor_cost_vector()
    {   
        return factor_cost_vector(_mu, _precision);
    }

    /**
     * @brief Compute the total cost function value given a state.
     */
    template <typename Factor>
    double GVIGH<Factor>::cost_value(const VectorXd &mean, SpMat &Precision)
    {

        SpMat Cov = inverse(Precision);

        MatrixXd cov_sp{Cov};
        MatrixXd precision_full{Precision};
        MatrixXd cov_full = precision_full.inverse();

        double value = 0.0;
        for (auto &opt_k : _vec_factors)
        {
            value += opt_k->fact_cost_value(mean, Cov); // / _temperature;
        }

        SparseLDLT ldlt(Precision);
        double det = ldlt.determinant();

        if (det < 0)
        {
            std::runtime_error("Infinity log determinant precision matrix ...");
        }
        return value + log(det) / 2;
    }

    /**
     * @brief Compute the total cost function value given a state, using current values.
     */
    template <typename Factor>
    double GVIGH<Factor>::cost_value()
    {
        return cost_value(_mu, _precision);
    }

    /**
     * @brief given a state, compute the total cost function value without the entropy term, using current values.
     */
    template <typename Factor>
    double GVIGH<Factor>::cost_value_no_entropy()
    {
        
        SpMat Cov = inverse(_precision);
        
        double value = 0.0;
        for (auto &opt_k : _vec_factors)
        {
            value += opt_k->fact_cost_value(_mu, Cov);
        }
        return value; // / _temperature;
    }

}
