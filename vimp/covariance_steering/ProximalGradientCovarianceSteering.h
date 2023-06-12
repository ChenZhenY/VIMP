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

#pragma once

#include "../helpers/ExperimentParams.h"
#include "LinearCovarianceSteering.h"
#include <memory>
#include <Eigen/QR>
#include "DataBuffer.h"

using namespace Eigen;

namespace vimp{
    class ProxGradCovSteer
    {
    public:
        ProxGradCovSteer(){};

        virtual ~ProxGradCovSteer() {}

        ProxGradCovSteer(const MatrixXd &A0,
                         const VectorXd &a0,
                         const MatrixXd &B,
                         const ExperimentParams& params):
                         _ei(),
                        _nx(params.nx()),
                        _nu(params.nu()),
                        _nt(params.nt()),
                        _eta(params.eta()),
                        _Akt(_ei.replicate3d(A0, _nt)),
                        _akt(_ei.replicate3d(a0, _nt)),
                        _Bt(_ei.replicate3d(B, _nt)),
                        _sig(params.sig()),
                        _eps(params.eps()),
                        _deltt(params.sig() / (_nt - 1)),
                        _z0(params.m0()),
                        _Sig0(params.Sig0()),
                        _zT(params.mT()),
                        _SigT(params.SigT()),
                        _max_iter(params.max_iter()),
                        _stop_err(params.stop_err()),
                        _Qkt(Matrix3D(_nx, _nx, _nt)),
                        _Qt(Matrix3D(_nx, _nx, _nt)),
                        _rkt(Matrix3D(_nx, 1, _nt)),
                        _hAkt(Matrix3D(_nx, _nx, _nt)),
                        _hakt(Matrix3D(_nx, 1, _nt)),
                        _nTrt(Matrix3D(_nx, 1, _nt)),
                        _pinvBBTt(Matrix3D(_nx, _nx, _nt)),
                        _zkt(_ei.replicate3d(_z0, _nt)),
                        _Sigkt(_ei.replicate3d(params.Sig0(), _nt)),
                        _Kt(_nu, _nx, _nt),
                        _dt(_nu, 1, _nt),
                        _linear_cs(_Akt, _Bt, _akt, _nx, _nu, _sig, _nt, _eps, _Qkt, _rkt, _z0, params.Sig0(), _zT, params.SigT()),
                        _recorder(_Akt, _Bt, _akt, _Qkt, _rkt, _Kt, _dt, _zkt, _Sigkt)
                        {
                            // Initialize the final time covariance
                            _ei.compress3d(_SigT, _Sigkt, _nt - 1);

                            // Linear interpolation for the mean zk
                            // initialize_zk();

                            // compute pinvBBT
                            MatrixXd Bi(_nx, _nu), BiT(_nu, _nx), pinvBBTi(_nx, _nx);
                            for (int i = 0; i < _nt; i++)
                            {
                                Bi = Bt_i(i);
                                BiT = Bi.transpose();
                                pinvBBTi = (Bi * BiT).completeOrthogonalDecomposition().pseudoInverse();
                                _ei.compress3d(pinvBBTi, _pinvBBTt, i);
                            }
                        }

        ProxGradCovSteer(const MatrixXd &A0,
                         const VectorXd &a0,
                         const MatrixXd &B,
                         double sig,
                         int nt,
                         double eta,
                         double eps,
                         const VectorXd &z0,
                         const MatrixXd &Sig0,
                         const VectorXd &zT,
                         const MatrixXd &SigT,
                         double stop_err,
                         int max_iteration = 30) : 
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
                        _deltt(sig / (nt - 1)),
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
                        _Kt(_nu, _nx, _nt),
                        _dt(_nu, 1, _nt),
                        _max_iter(max_iteration),
                        _stop_err(stop_err),
                        _linear_cs(_Akt, _Bt, _akt, _nx, _nu, _sig, _nt, _eps, _Qkt, _rkt, _z0, _Sig0, _zT, _SigT),
                        _recorder(_Akt, _Bt, _akt, _Qkt, _rkt, _Kt, _dt, _zkt, _Sigkt)
        {
            // Initialize the final time covariance
            _ei.compress3d(_SigT, _Sigkt, _nt - 1);

            // Linear interpolation for the mean zk
            // initialize_zk();

            // compute pinvBBT
            MatrixXd Bi(_nx, _nu), BiT(_nu, _nx), pinvBBTi(_nx, _nx);
            for (int i = 0; i < _nt; i++)
            {
                Bi = Bt_i(i);
                BiT = Bi.transpose();
                pinvBBTi = (Bi * BiT).completeOrthogonalDecomposition().pseudoInverse();
                _ei.compress3d(pinvBBTi, _pinvBBTt, i);
            }
        }

        /**
         * @brief The optimization process, including linearization,
         * sovling a linear CS, and push forward the mean and covariances.
         * @return std::tuple<MatrixXd, MatrixXd>  representing (Kt, dt)
         */
        virtual std::tuple<Matrix3D, Matrix3D, int> optimize()
        {
            double err = 1;
            MatrixXd Ak_prev(_nx * _nx, _nt), ak_prev(_nx, _nt);
            Ak_prev = _Akt;
            ak_prev = _akt;
            int i_step = 1;
            while ((err > _stop_err) && (i_step <= _max_iter))
            {
                step(i_step);
                err = (Ak_prev - _Akt).norm() / _Akt.norm() / _nt + (ak_prev - _akt).norm() / _akt.norm() / _nt;
                Ak_prev = _Akt;
                ak_prev = _akt;
                i_step++;
                _recorder.add_iteration(_Akt, _Bt, _akt, _Qkt, _rkt, _Kt, _dt, _zkt, _Sigkt);
            }

            return std::make_tuple(_Kt, _dt, i_step);
        }

        virtual void step(int indx) = 0;

        /**
         * @brief Problem with a state cost V(Xt) differs only in the expressions Qk and rk.
         */
        virtual void update_Qrk()
        {
            MatrixXd Aki(_nx, _nx), aki(_nx, 1), hAi(_nx, _nx), hai(_nx, 1), Bi(_nx, _nu), Qti(_nx, _nx), pinvBBTi(_nx, _nx), Qki(_nx, _nx), nTri(_nx, 1), zi(_nx, 1), rki(_nx, 1);
            MatrixXd temp(_nx, _nx);
            // for each time step
            _Qkt.setZero();
            _rkt.setZero();

            for (int i = 0; i < _nt; i++)
            {
                Aki = Akt_i(i);
                aki = akt_i(i);
                hAi = hAkt_i(i);
                hai = hakt_i(i);
                Bi = Bt_i(i);
                Qti = Qt_i(i);
                pinvBBTi = pinvBBTt_i(i);
                nTri = nTrt_i(i);
                zi = zkt_i(i);
                temp = (Aki - hAi).transpose();
                Qki = Qti * 2 * _eta / (1 + _eta) + temp * pinvBBTi * (Aki - hAi) * _eta / (1 + _eta) / (1 + _eta);
                rki = -(Qti * zi) * _eta / (1 + _eta) + nTri * _eta / (1 + _eta) / 2 + temp * pinvBBTi * (aki - hai) * _eta / (1 + _eta) / (1 + _eta);

                // update Qkt, rkt
                _ei.compress3d(Qki, _Qkt, i);
                _ei.compress3d(rki, _rkt, i);
            }
        }

        void solve_linearCS(const MatrixXd &A, const MatrixXd &B, const MatrixXd &a, const MatrixXd &Q, const MatrixXd &r)
        {
            // solve for the linear covariance steering
            _linear_cs.update_params(A, B, a, Q, r);
            _linear_cs.solve();

            // retrieve (K, d)
            _Kt = _linear_cs.Kt();
            _dt = _linear_cs.dt();

            Matrix3D fbK(_nx, _nx, _nt), fbd(_nx, 1, _nt);
            MatrixXd Ai(_nx, _nx), ai(_nx, 1), Aprior_i(_nx, _nx), aprior_i(_nx, 1), Ki(_nu, _nx), fbKi(_nx, _nx), di(_nx, 1), fbdi(_nx, 1), Bi(_nx, _nu);
            for (int i = 0; i < _nt; i++)
            {
                Aprior_i = _ei.decomp3d(A, _nx, _nx, i);
                aprior_i = _ei.decomp3d(a, _nx, 1, i);

                Bi = Bt_i(i);
                Ki = Kt_i(i);
                di = dt_i(i);

                Ai = Aprior_i + Bi * Ki;
                ai = aprior_i + Bi * di;

                _ei.compress3d(Ai, _Akt, i);
                _ei.compress3d(ai, _akt, i);
            }
        }

        void propagate_mean()
        {   
            // The i_th matrices
            Eigen::VectorXd zi(_nx), znew(_nx), ai(_nx);
            Eigen::MatrixXd Ai(_nx, _nx), Bi(_nx, _nu), AiT(_nx, _nx), BiT(_nu, _nx);
            Eigen::MatrixXd Si(_nx, _nx), Snew(_nx, _nx);
            for (int i = 0; i < _nt - 1; i++)
            {
                zi = zkt_i(i);
                Ai = Akt_i(i);
                ai = akt_i(i);
                Bi = Bt_i(i);
                Si = Sigkt_i(i);

                AiT = Ai.transpose();
                BiT = Bi.transpose();

                znew = zi + _deltt * (Ai * zi + ai);
                Snew = Si + _deltt * (Ai * Si + Si * AiT + _eps * (Bi * BiT));

                _ei.compress3d(znew, _zkt, i + 1);
                _ei.compress3d(Snew, _Sigkt, i + 1);
            }
        }


        inline Matrix3D zkt() { return _zkt; }

        inline Matrix3D Sigkt() { return _Sigkt; }

        inline Matrix3D Akt() { return _Akt; }

        inline Matrix3D akt() { return _akt; }

        inline Matrix3D Qkt() { return _Qkt; }

        inline Matrix3D rkt() { return _rkt; }

        /**
         * @brief get the matrices at specific time point i.
         */

        inline MatrixXd Akt_i(int i){ return _ei.decomp3d(_Akt, _nx, _nx, i);}

        inline MatrixXd akt_i(int i){ return _ei.decomp3d(_akt, _nx, 1, i);}

        inline MatrixXd hAkt_i(int i){ return _ei.decomp3d(_hAkt, _nx, _nx, i);}

        inline MatrixXd hakt_i(int i){ return _ei.decomp3d(_hakt, _nx, 1, i);}

        inline MatrixXd Bt_i(int i){ return _ei.decomp3d(_Bt, _nx, _nu, i);}

        inline MatrixXd Qt_i(int i){ return _ei.decomp3d(_Qt, _nx, _nx, i);}

        inline MatrixXd Qkt_i(int i){ return _ei.decomp3d(_Qkt, _nx, _nx, i);}

        inline MatrixXd pinvBBTt_i(int i){ return _ei.decomp3d(_pinvBBTt, _nx, _nx, i);}

        inline MatrixXd nTrt_i(int i){ return _ei.decomp3d(_nTrt, _nx, 1, i);}
        
        inline MatrixXd zkt_i(int i){ return _ei.decomp3d(_zkt, _nx, 1, i);}

        inline MatrixXd Kt_i(int i){ return _ei.decomp3d(_Kt, _nu, _nx, i);}

        inline MatrixXd dt_i(int i){ return _ei.decomp3d(_dt, _nu, 1, i);}

        inline MatrixXd Sigkt_i(int i){ return _ei.decomp3d(_Sigkt, _nx, _nx, i);}

        /**
         * @brief replicating a fixed state cost
         */
        void repliacteQt(MatrixXd Q0) { _Qt = _ei.replicate3d(Q0, _nt); }

        void initialize_zk() { _zkt = _ei.linspace(_z0, _zT, _nt); }

        void set_max_iter(int max_iter) { _max_iter = max_iter; }

    protected:
        EigenWrapper _ei;
        int _nx, _nu, _nt;
        double _eta, _sig, _eps, _deltt, _stop_err;
        int _max_iter;

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

        // Data buffer
        DataBuffer _buffer;

        // Data recorder for iteration plot
        DataRecorder _recorder;

        // Dynamics class
        LinearCovarianceSteering _linear_cs;
    };
}