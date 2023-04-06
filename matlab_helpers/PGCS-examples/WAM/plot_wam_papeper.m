clear all
close all
clc

%% ******************* Read datas ******************
addpath('/usr/local/gtsam_toolbox')
addpath ('/home/hongzhe/git/VIMP/matlab_helpers/experiments/WAM/utils')
import gtsam.*
import gpmp2.*

% import utils.*

replan = true;

prefix = "case1";
means = csvread([prefix+"/zk_sdf.csv"]);
covs = csvread([prefix+"/Sk_sdf.csv"]);

[nx, nt] = size(means);

addpath("../../error_ellipse");

%% ******************* Define map dataset ******************
dataset = generate3Ddataset('WAMDeskDataset');
origin = [dataset.origin_x, dataset.origin_y, dataset.origin_z];
origin_point3 = Point3(origin');
cell_size = dataset.cell_size;

disp('calculating signed distance field ...');
field = signedDistanceField3D(dataset.map, dataset.cell_size);
disp('calculating signed distance field done');

%% ******************* WAM Arm and start and end conf ******************
arm = generateArm('WAMArm');

start_conf = [-0.8,-1.70,1.64,1.29,1.1,-0.106,2.2]';
end_conf = [-0.0,0.94,0,1.6,0,-0.919,1.55]';
start_vel = zeros(7,1);
end_vel = zeros(7,1);

%% ================= plot the final iteration, only mean value ===================
x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'none')
nexttile
t.FontSize = 14;
hold on 
view(3)
plotMap3D(dataset.corner_idx, origin, cell_size);

for j = 1:nt
    % gradual changing colors
    alpha = (j / nt)^(1.15);
    color = [0, 0, 1, alpha];
    % means
    plotArm3D(arm.fk_model(), means(:, j), color, 4, true);
end
plotArm3D(arm.fk_model(), start_conf, 'r', 4, true);
plotArm3D(arm.fk_model(), end_conf, 'g', 4, true);
plotArm3D(arm.fk_model(), end_conf, 'g', 4, true);

xlim([-1, 1.4])
ylim([-1, 1.0])
zlim([-0.8, 0.9])

axis off;

% %% ==== plot sampled covariance for the supported states seperately ==== 
% x0 = 500;
% y0 = 500;
% width = 600;
% height = 350;
% figure
% set(gcf,'position',[x0,y0,width,height])
% 
% tiledlayout(3, 5, 'TileSpacing', 'tight', 'Padding', 'tight')
% 
% n_samples = 50;
% for j = 1:1:n_states
%     nexttile
%     t = title(['Support State ',num2str(j)]);
%     t.FontSize = 16;
%     hold on 
%     view(3)
%     % plot map
%     plotMap3D(dataset.corner_idx, origin, cell_size);
% 
%     % ------------------- sampling ------------------- 
%     % gradual changing colors
%     color = [0, 0, 1, 0.9];
%     color_sample = [0.0, 0.0, 0.7, 0.04];
% 
%     i_vec_means = vec_means{nsteps};
%     i_vec_covs = vec_covs{nsteps};
%     % mu j
%     mean_j = i_vec_means{j}';
%     % cov j
%     cov_j = i_vec_covs{j};
%     rng('default')  % For reproducibility
%     samples = mvnrnd(mean_j, cov_j, n_samples);
%     plotArm3D(arm.fk_model(), mean_j, color, 4, true);
%     for k = 1: size(samples, 1)
%         k_sample = samples(k, 1:end)';
%         plotArm3D(arm.fk_model(), k_sample, color_sample, 3, false);
%     end
% 
% plotArm3D(arm.fk_model(), start_conf, 'r', 4, true);
% plotArm3D(arm.fk_model(), end_conf, 'g', 4, true);
% 
% end
