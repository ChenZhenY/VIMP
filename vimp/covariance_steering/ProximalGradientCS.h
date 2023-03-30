/**
 * @file ProximalGradientCS.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Proximal gradient algorithm for nonlinear covariance steering. 
 * @version 0.1
 * @date 2023-03-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "../dynamics/NonlinearDynamics.h"
#include "LinearCovarianceSteering.h"
#include "../helpers/eigen_wrapper.h"
#include <memory>
#include <Eigen/QR> 

using namespace Eigen;

namespace vimp{

class ProxGradCovSteer{
public:
    ProxGradCovSteer(){};

    virtual ~ProxGradCovSteer(){}

    ProxGradCovSteer(MatrixXd A0, 
                     VectorXd a0, 
                     MatrixXd B, 
                     double sig,
                     int nt,
                     double eta,
                     double eps,
                     VectorXd z0,
                     MatrixXd Sig0,
                     VectorXd zT,
                     MatrixXd SigT,
                     std::shared_ptr<NonlinearDynamics> pdyn,
                     double Vscale=1.0): 
                     _ei(),
                     _nx(A0.rows()),
                     _nu(B.cols()),
                     _nt(nt),
                     _eta(eta),
                     _Akt(_ei.replicate3d(A0, _nt)),
                     _akt(_ei.replicate3d(a0, _nt)),
                     _Bt(_ei.replicate3d(B, _nt)),
                     _sig(sig),
                     _eps(eps),
                     _deltt(1.0/(nt-1)),
                     _state_cost_scale(Vscale),
                     _Qkt(Matrix3D(_nx, _nx, _nt)),
                     _Qt(Matrix3D(_nx, _nx, _nt)),
                     _rkt(Matrix3D(_nx, 1, _nt)),
                     _hAkt(Matrix3D(_nx, _nx, _nt)),
                     _hakt(Matrix3D(_nx, 1, _nt)),
                     _nTrt(Matrix3D(_nx, 1, _nt)),
                     _pinvBBTt(Matrix3D(_nx, _nx, _nt)),
                     _zkt(_ei.replicate3d(z0, _nt)),
                     _Sigkt(_ei.replicate3d(Sig0, _nt)),
                     _z0(z0),
                     _Sig0(Sig0),
                     _zT(zT),
                     _SigT(SigT),
                     _dynptr{pdyn},
                     _Kt(_nu, _nx, _nt),
                     _dt(_nu, 1, _nt),
                     _linear_cs(_Akt, _Bt, _akt, _nx, _nu, _nt, _eps, _Qkt, _rkt, _z0, _Sig0, _zT, _SigT)
                     {  
                        // Initialize the final time covariance
                        _ei.compress3d(_SigT, _Sigkt, _nt-1); 
                        MatrixXd Bi(_nx, _nu), BiT(_nu, _nx), pinvBBTi(_nx, _nx);
                        for (int i=0; i<_nt; i++){
                            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
                            BiT = Bi.transpose();
                            pinvBBTi = (Bi * BiT).completeOrthogonalDecomposition().pseudoInverse();;
                            _ei.compress3d(pinvBBTi, _pinvBBTt, i);
                        }
                     }

    /**
     * @brief The optimization process, including linearization, 
     * sovling a linear CS, and push forward the mean and covariances.
     * @return std::tuple<MatrixXd, MatrixXd>  representing (Kt, dt)
     */
    std::tuple<MatrixXd, MatrixXd> optimize(double stop_err){
        double err = 1;
        MatrixXd Ak_prev(_nx*_nx, _nt), ak_prev(_nx, _nt);
        Ak_prev = _Akt;
        ak_prev = _akt;
        int i_step = 0;
        while (err > stop_err){
            step(i_step);
            err = (Ak_prev - _Akt).norm() / _Akt.norm() / _nt + (ak_prev - _akt).norm() / _akt.norm() / _nt;
            Ak_prev = _Akt;
            ak_prev = _akt;
            i_step ++;
        }

        return std::make_tuple(_Kt, _dt);
    }

    /**
     * @brief Solving a linear covariance steering at each iteration.
     * @return none, but inside already compute (K, d).
     */
    void step(int indx){
        std::cout << "----- iter " << indx << " -----" << std::endl;
        // propagate the mean and the covariance
        propagate_mean(_Akt, _akt, _Bt);
        linearization();

        MatrixXd Apriort(_nx*_nx, _nt), apriort(_nx, _nt);
        Apriort = _Akt / (1+_eta) + _hAkt * _eta / (1+_eta);
        apriort = _akt / (1+_eta) + _hakt * _eta / (1+_eta);

        // Update Qkt, rkt
        update_Qrk();

        // solve for the linear covariance steering
        _linear_cs.update_params(Apriort, _Bt, apriort, _Qkt, _rkt);
        _linear_cs.solve();

        // retrieve (K, d)
        _Kt = _linear_cs.Kt();
        _dt = _linear_cs.dt();

        Matrix3D fbK(_nx, _nx, _nt), fbd(_nx, 1, _nt);
        MatrixXd Ai(_nx, _nx), ai(_nx, 1), Aprior_i(_nx, _nx), aprior_i(_nx, 1), Ki(_nu, _nx), fbKi(_nx, _nx), di(_nx, 1), fbdi(_nx, 1), Bi(_nx, _nu);
        for (int i=0; i<_nt; i++){
            Aprior_i = _ei.decompress3d(Apriort, _nx, _nx, i);
            aprior_i = _ei.decompress3d(apriort, _nx, 1, i);

            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
            Ki = _ei.decompress3d(_Kt, _nu, _nx, i);
            di = _ei.decompress3d(_dt, _nu, 1, i);

            Ai = Aprior_i + Bi * Ki;
            ai = aprior_i + Bi * di;
            _ei.compress3d(Ai, _Akt, i);
            _ei.compress3d(ai, _akt, i);
        }
    }

    inline Matrix3D zkt(){ return _zkt; }

    inline Matrix3D Sigkt(){ return _Sigkt; }

    inline Matrix3D Akt(){ return _Akt; }

    inline Matrix3D akt(){ return _akt; }

    inline Matrix3D Qkt(){ return _Qkt; }

    inline Matrix3D rkt(){ return _rkt; }

    /**
     * @brief replicating a fixed state cost
     */
    void repliacteQt(MatrixXd Q0){
        _Qt = _ei.replicate3d(Q0, _nt);
    }

    /**
     * @brief Problem with a state cost V(Xt) differs only in the expressions Qk and rk.
     */
    virtual void update_Qrk(){
        MatrixXd Aki(_nx, _nx), aki(_nx, 1), hAi(_nx, _nx), hai(_nx, 1), Bi(_nx, _nu), Qti(_nx, _nx), pinvBBTi(_nx, _nx), Qki(_nx, _nx), nTri(_nx, 1), zi(_nx, 1), rki(_nx, 1);
        MatrixXd temp(_nx, _nx);
        // for each time step
        _Qkt.setZero();
        _rkt.setZero();
        for (int i=0; i<_nt; i++){
            Aki = _ei.decompress3d(_Akt, _nx, _nx, i);
            aki = _ei.decompress3d(_akt, _nx, 1, i);
            hAi = _ei.decompress3d(_hAkt, _nx, _nx, i);
            hai = _ei.decompress3d(_hakt, _nx, 1, i);
            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
            Qti = _ei.decompress3d(_Qt, _nx, _nx, i);
            pinvBBTi = _ei.decompress3d(_pinvBBTt, _nx, _nx, i);
            nTri = _ei.decompress3d(_nTrt, _nx, 1, i);
            zi = _ei.decompress3d(_zkt, _nx, 1, i);
            temp = (Aki - hAi).transpose();
            Qki = Qti * 2 * _state_cost_scale * _eta / (1 + _eta)  + temp * pinvBBTi * (Aki - hAi) * _eta / (1+_eta) / (1+_eta);
            rki = - (Qti * zi) * _state_cost_scale * _eta / (1 + _eta) +  nTri * _eta / (1+_eta) / 2 +  temp * pinvBBTi * (aki - hai) * _eta / (1+_eta) / (1+_eta);

            // update Qkt, rkt
            _ei.compress3d(Qki, _Qkt, i);
            _ei.compress3d(rki, _rkt, i);
        }
        
    }

    /**
     * @brief linearization
     */
    void linearization(){
        std::tuple<Matrix3D, MatrixXd, Matrix3D, Matrix3D> res;
        res = _dynptr->linearize(_zkt, _sig, _Akt, _Sigkt);
        _hAkt = std::get<0>(res);
        _Bt   = std::get<1>(res);
        _hakt = std::get<2>(res);
        _nTrt = std::get<3>(res);
    }

    void propagate_mean(Matrix3D At, Matrix3D at, Matrix3D Bt){
        // The i_th matrices
        Eigen::VectorXd zi(_nx), znew(_nx), ai(_nx);
        Eigen::MatrixXd Ai(_nx, _nx), Bi(_nx, _nu);
        Eigen::MatrixXd Si(_nx, _nx), Snew(_nx, _nx);
        for (int i=0; i<_nt-1; i++){
            zi = _ei.decompress3d(_zkt, _nx, 1, i);
            Ai = _ei.decompress3d(At, _nx, _nx, i);
            ai = _ei.decompress3d(at, _nx, 1, i);
            Bi = _ei.decompress3d(Bt, _nx, _nu, i);
            Si = _ei.decompress3d(_Sigkt, _nx, _nx, i);

            znew = zi + _deltt*(Ai*zi + ai);
            Snew = Si + _deltt*(Ai*Si + Si*Ai.transpose() + _eps*(Bi*Bi.transpose()));

            _ei.compress3d(znew, _zkt, i+1);
            _ei.compress3d(Snew, _Sigkt, i+1);
        }
    }


public:
    EigenWrapper _ei;
    int _nx, _nu, _nt;
    double _eta, _sig, _eps, _deltt;
    double _state_cost_scale;

    // All the variables are time variant (3d matrices)
    // iteration variables
    Matrix3D _Akt, _Bt, _akt, _pinvBBTt;
    Matrix3D _Qkt, _Qt; // Qk is the Q in each iteration, and Qt is the quadratic state cost matrix.
    Matrix3D _rkt;

    // linearizations
    Matrix3D _hAkt, _hakt, _nTrt;

    // boundary conditions
    Matrix3D _Sigkt;
    MatrixXd _Sig0, _SigT;
    VectorXd _z0, _zT;
    Matrix3D _zkt;

    // Final result
    Matrix3D _Kt;
    Matrix3D _dt;

    // Dynamics class
    std::shared_ptr<NonlinearDynamics> _dynptr;
    LinearCovarianceSteering _linear_cs;
    
};
}