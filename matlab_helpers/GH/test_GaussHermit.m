clc
%% Verification code for one dimensional case
% generating the ground truth for the tests in vimp c++ code.
% Author: Hongzhe Yu
% Date: 07/26/2022

mu_p = 20;
sig_p_sq = 9;
P = 9;
p = 6;

f = 400;
b = 0.1;
sig_r_sq = 0.09;
y = f*b/mu_p + 0.05;

syms x real
gx1 = x;
gx2 = x^2;
gx3 = x^3;
gx4 = x^4;

mu = mu_p;

GaussHermitOneDim(gx1, p, mu, P);
GaussHermitOneDim(gx2, p, mu, P);
GaussHermitOneDim(gx3, p, mu, P);
GaussHermitOneDim(gx4, p, mu, P);

phi = (x-mu_p)^2/sig_p_sq/2 + (y-f*b/x)^2/sig_r_sq/2;
phi_func = matlabFunction(phi);

xmu_phi = (x-mu).*phi;
xmu_phi_func = matlabFunction(xmu_phi);

xmumuT_phi = (x-mu)*(x-mu).*phi;
xmumuT_phi_func = matlabFunction(xmumuT_phi);


%% function valuesRows
disp('phi_func value at mu_p')
feval(phi_func, mu_p)

disp('xmu_phi_func value at mu_p')
feval(xmu_phi_func, mu_p)

disp('xmumuT_phi_func value at mu_p')
feval(xmumuT_phi_func, mu_p)

%% ===== Gauss Hermite =====
% Weights
p = 10;
[roots, W] = getWeight(p)

%% Integrations
p = 6;
mu = mu_p;
P = 9; %covariance, =sig^2
disp('E_Phi')
Int1 = GaussHermitOneDim(phi, p, mu, P)
disp('E_xmu_phi')
Int2 = GaussHermitOneDim(xmu_phi, p, mu, P)
disp('E_xmumuT_phi')
Int3 = GaussHermitOneDim(xmumuT_phi, p, mu, P)

%% Cost
% cost value should be E_{phi(x)} + ln(det(sig^{-2}))/2
cost_value =  GaussHermitOneDim(phi, p, mu, P) + log(1/P)/2

%% Test GaussHermit for a quadratic function
% ============== two dimension input x = [x_1; x_2], one dimensional output ============== 
% Cost fn
x = sym('x', [2,1]);
prec_cost = 10000.*eye(2);
phi = transpose(x) * prec_cost * x
phi_func = matlabFunction(phi);

% Inputs
m = ones(2,1);
prec = [1, -0.74; -0.74, 1.0];
cov = inv(prec);

p_GH = 10;
Int1 = GaussianHermiteN(2, phi, p_GH, m, cov)

% ============== two dimension input x = [x_1; x_2], two dimensional output ============== 
x = sym('x', [2,1])
phi_2 = [3*x(1)*x(1); 2*x(1)*x(2)];
phi_func = matlabFunction(phi_2);

Int2 = GaussianHermiteN(2, phi_2, p_GH, m, cov)
