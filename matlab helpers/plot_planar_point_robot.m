%% Plot the result of planar point robot with a simple sdf;
% Author: Hongzhe Yu
% Date: 07/27/22

clear all
close all
clc

%% ******************* Read datas ******************
means = csvread("../vimp/data/2d_pR/mean.csv");
covs = csvread("../vimp/data/2d_pR/cov.csv");
costs = csvread("../vimp/data/2d_pR/cost.csv");
sdfmap = csvread("../vimp/data/2d_pR/map_ground_truth.csv");
addpath("error_ellipse");

[niters, ttl_dim] = size(means)
dim_theta = 4;
nsteps = 10;
step_size = floor(niters / nsteps);
n_states = floor(ttl_dim / dim_theta);

% =========================== load the means and covs on the 2*2 level
% containers for all the steps data
vec_means = cell(niters, 1);
vec_covs = cell(niters, 1);

for i_iter = 0: nsteps-1
        % each step updates
        i = i_iter * step_size;
        i_mean = means(i+1, 1:end);
        i_cov = covs(i*ttl_dim+1 : (i+1)*ttl_dim, 1:ttl_dim);
        i_vec_means_2d = cell(n_states, 1);
        i_vec_covs_2d = cell(n_states, 1);
        for j = 0:n_states-1
            i_vec_means_2d{j+1} = i_mean(j*dim_theta+1 : j*dim_theta+2);
            i_vec_covs_2d{j+1} = i_cov(j*dim_theta +1 : j*dim_theta+2,  j*dim_theta+1 : j*dim_theta+2);
        end
        vec_means{i_iter+1} = i_vec_means_2d;
        vec_covs{i_iter+1} = i_vec_covs_2d;
end

%% plot sdf and means and covs
figure
title("SDF 2D Point Robot")
% -------- plot the means -------- 
colors = [255, 0, 0];

for i_iter = 1: nsteps
    subplot(2, floor(nsteps/2), i_iter)
    hold on 
    % -------- plot the sdf mesh contour -------- 
    nmesh = size(sdfmap, 1);
    x_mesh = linspace(0, nmesh, nmesh);
    y_mesh = linspace(0, nmesh, nmesh);
    [X,Y] = meshgrid(x_mesh, y_mesh);
    contourf(X,Y,sdfmap, 1)

    i_vec_means_2d = vec_means{i_iter};
    i_vec_covs_2d = vec_covs{i_iter};
    for j = 1:n_states
        % means
        scatter(i_vec_means_2d{j}(1), i_vec_means_2d{j}(2), 40, 'r*');
        % covariance
        error_ellipse(i_vec_covs_2d{j}, i_vec_means_2d{j});
    end
end

figure 
grid on 
hold on
plot(costs, 'LineWidth', 2.2);
xlabel('iterations')
ylabel('cost')