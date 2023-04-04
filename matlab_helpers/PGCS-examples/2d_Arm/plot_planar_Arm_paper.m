clear all
close all
clc

%% ******************* Read datas ******************
addpath('/usr/local/gtsam_toolbox')
import gtsam.*
import gpmp2.*

map = 2;
exp = 1;

prefix = "map1";
switch map
    case 1
        prefix = "map1";
    case 2
        sdfmap = csvread("map2/map.csv");
        switch exp
            case 1
                prefix = "map2/case1";
        end
end

means = csvread([prefix+"/zk_sdf.csv"]);
covs = csvread([prefix+"/Sk_sdf.csv"]);

addpath("../../error_ellipse");

% ----- parameters -----
[ndim, nt] = size(means);
dim_theta = 4;
covs = reshape(covs, dim_theta, dim_theta, nt);
% niters
nsteps = 10;
step_size = floor(nt / nsteps);

%  ------- arm --------
arm = generateArm('SimpleTwoLinksArm');

%  ------- sdf --------
cell_size = 0.01;
origin_x = -1;
origin_y = -1;
origin_point2 = Point2(origin_x, origin_y);
field = signedDistanceField2D(sdfmap, cell_size);
% save field
sdf = PlanarSDF(origin_point2, cell_size, field);
% boundary conditions
start_conf = [0, 0]';
start_vel = [0, 0]';
end_conf = [pi/2, 0]';
end_vel = [0, 0]';

%% ================= plot the final iteration ===================


x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'none')
nexttile
t=title("2-link arm");
t.FontSize = 16;

hold on 
plotEvidenceMap2D_arm(sdfmap, origin_x, origin_y, cell_size);
for j = 1:nt
    % gradual changing colors
    alpha = (j / nt)^(1.15);
    color = [0, 0, 1, alpha];
    % means
    plotPlanarArm1(arm.fk_model(), means(1:2,j), color, 2, true);
end
plotPlanarArm1(arm.fk_model(), start_conf, 'r', 2, true);
plotPlanarArm1(arm.fk_model(), end_conf, 'g', 2, true);
hold off

%% Plot the animated motion plan
x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'none')
nexttile
t=title("Animation");
t.FontSize = 16;

for j = 1:nt
    hold on 
    plotEvidenceMap2D_arm(sdfmap, origin_x, origin_y, cell_size);
    plotPlanarArm1(arm.fk_model(), start_conf, 'r', 2, true);
    plotPlanarArm1(arm.fk_model(), end_conf, 'g', 2, true);
    % gradual changing colors
%     alpha = (j / nt)^(1.15);
    color = [0, 0, 1, 1];
    % means
    plotPlanarArm1(arm.fk_model(), means(1:2,j), color, 2, true);
    hold off
    pause(0.01)
end
% %% ==== plot sampled covariance for the states ==== 
% x0 = 500;
% y0 = 500;
% width = 600;
% height = 350;
% figure
% set(gcf,'position',[x0,y0,width,height])
% 
% tiledlayout(2, 3, 'TileSpacing', 'tight', 'Padding', 'tight')
% 
% n_samples = 50;
% for j = 1:3:ndim
%     j
%     nexttile
%     t = title(['Support State ',num2str(j)]);
%     t.FontSize = 16;
%     hold on 
%     i_vec_means_2d = vec_means{nsteps};
%     i_covs_2d = vec_covs{nsteps};
%     plotEvidenceMap2D_arm(sdfmap, origin_x, origin_y, cell_size);
% 
%     % gradual changing colors
% %     alpha = (j / n_states)^(1.15);
%     color = [0, 0, 1, 0.9];
%     color_sample = [0.0, 0.0, 0.7, 0.02];
%     % mu j
%     mean_j = i_vec_means_2d{j}';
%     % cov j
%     cov_j = i_covs_2d{j};
%     % sampling 
%     rng('default')  % For reproducibility
%     samples = mvnrnd(mean_j, cov_j, n_samples);
%     plotPlanarArm1(arm.fk_model(), i_vec_means_2d{j}', color, 4, true);
%     for k = 1: size(samples, 1)
%         k_sample = samples(k, 1:end)';
%         plotPlanarArm1(arm.fk_model(), k_sample, color_sample, 3, false);
%     end
%     % means
% %     plotPlanarArm1(arm.fk_model(), , color, 2);
% plotPlanarArm1(arm.fk_model(), start_conf, 'r', 3, true);
% plotPlanarArm1(arm.fk_model(), end_conf, 'g', 3, true);
% xlim([-1 1.5])
% ylim([-0.5 1.5])
% end
% 
% % final step
% j = 15;
% nexttile
% t = title(['Support State ',num2str(j)]);
% t.FontSize = 16;
% hold on 
% i_vec_means_2d = vec_means{nsteps};
% i_covs_2d = vec_covs{nsteps};
% plotEvidenceMap2D_arm(sdfmap, origin_x, origin_y, cell_size);
% 
% % gradual changing colors
% %     alpha = (j / n_states)^(1.15);
% color = [0, 0, 1, 0.9];
% color_sample = [0.0, 0.0, 0.7, 0.02];
% % mu j
% mean_j = i_vec_means_2d{j}';
% % cov j
% cov_j = i_covs_2d{j};
% % sampling 
% rng('default')  % For reproducibility
% samples = mvnrnd(mean_j, cov_j, n_samples);
% plotPlanarArm1(arm.fk_model(), i_vec_means_2d{j}', color, 4, true);
% for k = 1: size(samples, 1)
%     k_sample = samples(k, 1:end)';
%     plotPlanarArm1(arm.fk_model(), k_sample, color_sample, 3, false);
% end
% % means
% %     plotPlanarArm1(arm.fk_model(), , color, 2);
% plotPlanarArm1(arm.fk_model(), start_conf, 'r', 3, true);
% plotPlanarArm1(arm.fk_model(), end_conf, 'g', 3, true);
% xlim([-1 1.5])
% ylim([-0.5 1.5])
% 
% hold off

%% create map and save
% dataset = generate2Ddataset('OneObstacleDataset');
% rows = dataset.rows;
% cols = dataset.cols;
% cell_size = dataset.cell_size;
% origin_point2 = Point2(dataset.origin_x, dataset.origin_y);
% csvwrite( '../vimp/data/2d_Arm/map.csv', dataset.map);
