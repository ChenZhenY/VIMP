/**
 * @file LinearCovarianceSteering.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Linear covariance steering problem. 
 * See Chen, Yongxin, Tryphon T. Georgiou, and Michele Pavon. 
 * "Optimal steering of a linear stochastic system to a final probability distribution, Part I." 
 * IEEE Transactions on Automatic Control 61.5 (2015): 1158-1169. 
 * @version 0.1
 * @date 2023-03-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <Eigen/Dense>
#include "../helpers/eigen_wrapper.h"
using namespace Eigen;

namespace vimp{
class LinearCovarianceSteering{

public:
    LinearCovarianceSteering(){}

/**
 * @brief Construct a new Linear Covariance Steering object
 * All time varying matrices should be in the shape (m*n, nt)
 */
    LinearCovarianceSteering(const Matrix3D& At, 
                             const Matrix3D& Bt,
                             const Matrix3D& at, 
                             const int nx,
                             const int nu,
                             const int nt,
                             const double epsilon,
                             const Matrix3D& Qt,
                             const Matrix3D& rt,
                             const VectorXd& m0,
                             const MatrixXd& Sig0,
                             const VectorXd& m1,
                             const MatrixXd& Sig1):
                             _At(At),
                             _Bt(Bt),
                             _at(at),
                             _rt(rt),
                             _Qt(Qt),
                             _nx(nx),
                             _nu(nu),
                             _nt(nt),
                             _Phi(MatrixXd::Identity(2*_nx, 2*_nx)),
                             _Phi11(MatrixXd::Zero(_nx, _nx)),
                             _Phi12(MatrixXd::Zero(_nx, _nx)),
                             _Mt(Matrix3D(2*_nx, 2*_nx, _nt)),
                             _Pit(Matrix3D(_nx, _nx, _nt)),
                             _m0(m0),
                             _m1(m1),
                             _Sig0(Sig0),
                             _Sig1(Sig1),
                             _eps(epsilon),
                             _delta_t(1.0/(nt-1)),
                             _Kt(Matrix3D(_nu, _nx, _nt)),
                             _dt(Matrix3D(_nu, 1, _nt)){
        compute_M_Phi();
    }

    void update_params(const MatrixXd& At, const MatrixXd& Bt, const MatrixXd& at, 
                        const MatrixXd& Qt, const MatrixXd& rt){
        _At = At;
        _Bt = Bt;
        _at = at;
        _Qt = Qt;
        _rt = rt;
        compute_M_Phi();
    }

    void compute_M_Phi(){
        MatrixXd Ai(_nx, _nx), Bi(_nx, _nu), Qi(_nx, _nx), Mi(2*_nx, 2*_nx);
        for (int i=0; i< _nt; i++){
            Ai = _ei.decompress3d(_At, _nx, _nx, i);
            Qi = _ei.decompress3d(_Qt, _nx, _nx, i);
            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
            Mi.block(0, 0, _nx, _nx) = Ai;
            Mi.block(0, _nx, _nx, _nx) = -Bi*Bi.transpose();
            Mi.block(_nx, 0, _nx, _nx) = -Qi;
            Mi.block(_nx, _nx, _nx, _nx) = -Ai.transpose();
            _ei.compress3d(Mi, _Mt, i);
        }
        _Phi = MatrixXd::Identity(2*_nx, 2*_nx);
        for (int i=0; i<_nt-1; i++){
            MatrixXd Mi = _ei.decompress3d(_Mt, 2*_nx, 2*_nx, i);
            _Phi = _Phi + Mi*_Phi*_delta_t;
        }

        _Phi11 = _Phi.block(0, 0, _nx, _nx);
        _Phi12 = _Phi.block(0, _nx, _nx, _nx);
    }

    Matrix3D At(){ return _At; }

    MatrixXd At(int i){
        MatrixXd Ai;
        _ei.decompress3d(_At, Ai, _nx, _nx, i);
        return Ai;
    }

    inline Matrix3D at(){ return _at; }

    inline MatrixXd at(int i){ return _at.col(i); }

    inline Matrix3D Bt(){ return _Bt; }

    inline MatrixXd Bt(int i){
        MatrixXd Bi;
        _ei.decompress3d(_Bt, Bi, _nx, _nu, i);
        return Bi;
    }

    inline Matrix3D Qt(){ return _Qt; }

    inline MatrixXd Qt(int i){ return _ei.decompress3d(_Qt, _nx, _nx, i); }

    inline Matrix3D rt(){ return _rt; }

    inline MatrixXd rt(int i){ return _rt.col(i); }

    inline Matrix3D Kt(){ return _Kt; }

    inline MatrixXd Kt(int i){ return _ei.decompress3d(_Kt, _nu, _nx, i); }

    inline Matrix3D dt(){ return _dt; }

    inline Matrix3D Mt(){ return _Mt; }

    MatrixXd Mt(int i){
        MatrixXd Mi{MatrixXd::Zero(2*_nx, 2*_nx)};
        _ei.decompress3d(_Mt, Mi, 2*_nx, 2*_nx, i);
        return Mi;
    }

    inline MatrixXd Phi(){ return _Phi; }

    inline MatrixXd Phi11(){ return _Phi11; }

    inline MatrixXd Phi12(){ return _Phi12; }

    inline Matrix3D Pit(){ return _Pit; }

    inline MatrixXd Pit(int i){ return _ei.decompress3d(_Pit, _nx, _nx, i); }

    void solve(){
        VectorXd s{VectorXd::Zero(2*_nx)};
        MatrixXd a_r{MatrixXd::Zero(2*_nx, _nt)};
        a_r << _at, 
              -_rt;
        
        MatrixXd Mi(2*_nx, 2*_nx);
        for (int i=0;i<_nt-1;i++){
            Mi = _ei.decompress3d(_Mt, 2*_nx, 2*_nx, i);
            s = s + (Mi*s + a_r.col(i))*_delta_t;
        }

        VectorXd rhs{_m1 - _Phi11*_m0-s.block(0,0,_nx,1)};
        VectorXd Lambda_0 = _Phi12.colPivHouseholderQr().solve(rhs);
        MatrixXd Xt(2*_nx, _nt);
        VectorXd X0(2*_nx);
        X0 << _m0, Lambda_0;

        Xt.col(0) = X0;
        for (int i=0; i<_nt-1; i++){
            Mi = _ei.decompress3d(_Mt, 2*_nx, 2*_nx, i); 
            Xt.col(i+1) = Xt.col(i) + _delta_t*(Mi*Xt.col(i) + a_r.col(i));
        }
        MatrixXd xt{Xt.block(0,0,_nx,_nt)};
        MatrixXd lbdt{Xt.block(_nx, 0, _nx, _nt)};
        MatrixXd v(_nu, _nt);
        MatrixXd Bi(_nx, _nu), BiT(_nu, _nx);
        
        for (int i=0; i<_nt; i++){
            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
            BiT = Bi.transpose();
            v.col(i) = -BiT*lbdt.col(i);
        }
        MatrixXd Sig0_inv_sqrt = _ei.psd_invsqrtm(_Sig0);
        
        MatrixXd Sig0_sqrt = _ei.psd_sqrtm(_Sig0);
        MatrixXd temp = _eps*_eps*MatrixXd::Identity(_nx, _nx)/4 + 
                        Sig0_sqrt*_Phi12.inverse()*_Sig1*_Phi12.transpose().inverse()*Sig0_sqrt;

        MatrixXd Pi_0 = _eps * _Sig0.inverse() / 2.0 - _Phi12.inverse()*_Phi11 - 
                        Sig0_inv_sqrt *_ei.psd_sqrtm(temp)*Sig0_inv_sqrt;
        
        MatrixXd Pi_0_T = Pi_0.transpose();
        Pi_0 = (Pi_0 + Pi_0_T)/2;
        _ei.compress3d(Pi_0, _Pit, 0);
        MatrixXd Ai(_nx, _nx), Qi(_nx, _nx), l_Pi(_nx, _nx), Pinew(_nx, _nx);
        for (int i=0; i<_nt-1; i++){
            l_Pi = _ei.decompress3d(_Pit, _nx, _nx, i);
            Qi = _ei.decompress3d(_Qt, _nx, _nx, i);
            Ai = _ei.decompress3d(_At, _nx, _nx, i);
            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
            BiT = Bi.transpose();
            Pinew = l_Pi - _delta_t*(Ai.transpose()*l_Pi+l_Pi*Ai-l_Pi*Bi*BiT*l_Pi+Qi);
            _ei.compress3d(Pinew, _Pit, i+1);
        }

        MatrixXd Ki(_nu, _nx);
        for (int i=0; i<_nt; i++){
            Bi = _ei.decompress3d(_Bt, _nx, _nu, i);
            BiT = Bi.transpose();
            l_Pi = _ei.decompress3d(_Pit, _nx, _nx, i);
            Ki = - BiT * l_Pi;
            _ei.compress3d(Ki, _Kt, i);
            _dt.col(i) = v.col(i) + BiT * l_Pi * xt.col(i);
        }
        
    }


private:
    Matrix3D _At, _Bt, _at;
    Matrix3D _Qt, _rt;

    int _nx, _nu, _nt;

    MatrixXd _Phi, _Phi11, _Phi12;

    Matrix3D _Mt, _Pit;
    VectorXd _m0, _m1;
    MatrixXd _Sig0, _Sig1;

    double _eps, _delta_t;

    // feedback gains
    Matrix3D _Kt;
    Matrix3D _dt;

    // helper
    vimp::EigenWrapper _ei;

};
}