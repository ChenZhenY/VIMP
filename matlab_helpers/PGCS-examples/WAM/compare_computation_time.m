clear all
close all
clc

%% Compute the computation time and compare between gpmp2, gvimp, and pgcsmp.
% =========================================================================
%                                  PGCS-MP
% =========================================================================
% ------------- parameters -------------
eps = 0.001;
eps_map = 0.18;
radius = 0.0;
total_time = 9.0;
nt = 50;
sig0 = 0.001;
sigT = 0.001;
step_size = 1e-4;
stop_err = 1e-5;
max_iter = 50;
map_name = 'map_bookshelf';
sig_obs = 8e2;
backtrack_ratio = 0.5;
max_n_backtracking = 8;
sdf_file = '/home/hzyu/git/VIMP/vimp/maps/WAM/WAMDeskDataset.bin';

executable = '/home/hzyu/git/VIMP/vimp/build/src/pgcs/pgcs_WAMArm';

%% ========================= 1st experiment =========================
i_exp = 1;
args = [num2str(i_exp), ' ', num2str(eps), ' ', num2str(eps_map), ' ',... 
    num2str(radius), ' ', num2str(total_time), ' ', num2str(nt), ' ', num2str(sig0), ...
    ' ', num2str(sigT), ' ', num2str(step_size), ' ', num2str(stop_err), ' ', num2str(max_iter), ' ', ...
    num2str(sig_obs), ' ', num2str(backtrack_ratio), ' ', num2str(max_n_backtracking), ' ', map_name, ' ', sdf_file];

% ================== optimize and count time ======================
profile on
for i = 1:50
    execute_one_exp(executable, args);
end
profile viewer
% =================================================================

%% ========================= 2nd experiment =========================
i_exp = 2;
args = [num2str(i_exp), ' ', num2str(eps), ' ', num2str(eps_map), ' ',... 
    num2str(radius), ' ', num2str(total_time), ' ', num2str(nt), ' ', num2str(sig0), ...
    ' ', num2str(sigT), ' ', num2str(step_size), ' ', num2str(stop_err), ' ', num2str(max_iter), ' ', ...
    num2str(sig_obs), ' ', num2str(backtrack_ratio), ' ', num2str(max_n_backtracking), ' ', map_name, ' ', sdf_file];

% ================== optimize and count time ======================
profile on
for i = 1:50
    execute_one_exp(executable, args);
end
profile viewer
% =================================================================

%% ========================================================================
%                                  GVI-MP
% =========================================================================
% ================= 1st experiment =================



%% ========================================================================
%                                  GPMP2
% =========================================================================
clear all 
close all
clc

start_conf = [-0.8,-1.70,1.64,1.29,1.1,-0.106,2.2]';
end_conf = [-0.0,0.94,0,1.6,0,-0.919,1.55]';

sdf_file = '/home/hzyu/git/VIMP/vimp/maps/WAM/WAMDeskDataset.bin';

% ================== optimize and count time ======================
profile on
for i=1:50
    execute_one_exp_gpmp2(sdf_file, start_conf, end_conf);
end
profile viewer
% =================================================================

%% ================= 2nd experiment =================
clear all 
close all
clc

start_conf = [-0.9,-1.70,1.34,1.19,0.8,-0.126,2.5]';
end_conf = [-0.7,1.35,1.2,1.0,-0.7,-0.1,1.2]';

sdf_file = '/home/hzyu/git/VIMP/vimp/maps/WAM/WAMDeskDataset.bin';

profile on
for i=1:50
    execute_one_exp_gpmp2(sdf_file, start_conf, end_conf);
end
profile viewer
