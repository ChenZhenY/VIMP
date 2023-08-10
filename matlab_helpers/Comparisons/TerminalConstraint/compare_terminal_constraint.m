%% This is a script to compare the terminal 
%% constraint for the GVI-MP and PGCS-MP algorithm under
%% different experiment parameters and conditions.
% Author: Hongzhe Yu
% Date: 07/06/2023

vimp_build_prefix = '/home/hzyu/git/VIMP/vimp/build/';
GVIMP_script = 'gvi_PointRobot';
PGCSMP_script = 'pgcs_PlanarPRModel';

% --------------- GVIMP hyperparameters -----------------





% --------------- PGCSMP hyperparameters ----------------
i_exp = 1;
eps = 0.01;
eps_map = 0.6;
total_time = 0.23;
nt = 50;
sig0 = 0.001;
sigT = 0.001;
eta = 1e-1;
stop_err = 1e-5;
max_iter = 50;
cost_sigma = 1.5e5;
saving_prefix='/home/hzyu/git/VIMP/matlab_helpers/Comparisons';

args = [num2str(i_exp), ' ', num2str(eps), ' ', num2str(eps_map), ' ', num2str(total_time), ' ', num2str(nt), ' ', num2str(sig0), ' ', num2str(sigT), ' ', ...
            num2str(eta), ' ', num2str(stop_err), ' ', num2str(max_iter), ' ', num2str(cost_sigma), ' ', saving_prefix];

command = ['/home/hzyu/git/VIMP/vimp/build/src/pgcs/pgcs_PlanarPRModel', ' ', args];
num_iter = system(command);

