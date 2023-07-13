clear all
close all
clc

%% ******************* Read datas ******************
addpath('/home/hyu419/.local/gtsam_toolbox')
import gtsam.*
import gpmp2.*

addpath("../../error_ellipse");

map = 1;
exp = 2;

prefix = "map1";
prefix_gpmp2 = "map1";
switch map
    case 1
        prefix = "map1";
        sdfmap = csvread("map1/map.csv");
        switch exp
            case 1
                prefix = "map1/case1";
                prefix_gpmp2 = "map1/case1/gpmp2";
                % boundary conditions
                start_conf = [0, 0]';
                start_vel = [0, 0]';
                end_conf = [pi/2, 0]';
                end_vel = [0, 0]';
            case 2
                prefix = "map1/case2";
                prefix_gpmp2 = "map1/case2/gpmp2";
                % boundary conditions
                start_conf = [-pi/2, 0]';
                start_vel = [0.1, 0]';
                end_conf = [pi/2, 0]';
                end_vel = [0, 0]';
        end

    case 2
        sdfmap = csvread("map2/map.csv");
        prefix_gpmp2 = "map2";
        switch exp
            case 1
                prefix = "map2/case1";
                % boundary conditions
                start_conf = [0, 0]';
                start_vel = [0, 0]';
                end_conf = [pi/2, 0]';
                end_vel = [0, 0]';
            case 2
                prefix = "map2/case2";
                % boundary conditions
                start_conf = [-pi/2, 0]';
                start_vel = [0.1, 0]';
                end_conf = [pi/2, 0]';
                end_vel = [0, 0]';
        end
end

% read pgcs results
[prefix+"/zk_sdf.csv"]
means = csvread([prefix+"/zk_sdf.csv"]);
covs = csvread([prefix+"/Sk_sdf.csv"]);

% read gpmp2 results
means_gpmp2 = csvread([prefix_gpmp2+"/zt_gpmp2.csv"]);

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


%% ================= plot the final iteration ===================
x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure(1)
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 2, 'TileSpacing', 'none', 'Padding', 'none')
nexttile
% t=title("2-link arm");
t.FontSize = 16;

hold on 
plotEvidenceMap2D_arm(sdfmap, origin_x, origin_y, cell_size);

for j = 1:2:nt
    % gradual changing colors
    alpha = (j / nt)^(1.15);
    color = [0, 0, 1, alpha];
    % means
    plotPlanarArm1(arm.fk_model(), means(1:2,j), color, 4, true);
end
plotPlanarArm1(arm.fk_model(), start_conf, 'r', 4, true);
plotPlanarArm1(arm.fk_model(), end_conf, 'g', 4, true);
axis off;
hold off

% ------ configuration space trajectory ------
nexttile
hold on 

% read gpmp2 results
means_gpmp2 = csvread([prefix_gpmp2+"/zt_gpmp2.csv"]);
nt_gpmp2 = size(means_gpmp2, 2);

% plot gpmp2 results
for i = 1:1:nt_gpmp2
    scatter(means_gpmp2(1, i), means_gpmp2(2, i), 20, 'b', 'fill');
end

% plot pgcs results
nt = size(means, 2);
for i=1:nt
    scatter(means(1, i), means(2, i), 20, 'k', 'fill');
    error_ellipse(covs(1:2,1:2,i), means(1:2, i));
end

% plot start and goal conf
scatter(start_conf(1), start_conf(2), 20, 'r', 'fill');
scatter(end_conf(1), end_conf(2), 20, 'g', 'fill');

t.FontSize = 16;
axis off
hold off


% %% ==== animated motion plan ==== 
% x0 = 50;
% y0 = 50;
% width = 400;
% height = 350;
% figure(2)
% set(figure(2),'position',[x0,y0,width,height])
% tiledlayout(1, 2, 'TileSpacing', 'none', 'Padding', 'none')
% nexttile
% t=title("Animation");
% t.FontSize = 16;
% 
% % ---------- plot the mean positions ---------- 
% for j = 1:nt
%     hold on 
%     figure(2)
%     plotEvidenceMap2D_arm(sdfmap, origin_x, origin_y, cell_size);
%     plotPlanarArm1(arm.fk_model(), start_conf, 'r', 2, true);
%     plotPlanarArm1(arm.fk_model(), end_conf, 'g', 2, true);
%     % gradual changing colors
% %     alpha = (j / nt)^(1.15);
%     color = [0, 0, 1, 1];
%     % means
%     plotPlanarArm1(arm.fk_model(), means(1:2,j), color, 2, true);
%     hold off
%     pause(0.01)
% end

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
