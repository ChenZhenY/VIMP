clear all
close all
clc
addpath('/usr/local/gtsam_toolbox')
import gtsam.*
import gpmp2.*

addpath("../")
addpath("../error_ellipse");
addpath("../../../matlab_helpers/");

%% read map
sdfmap = csvread("../../RAL-examples/2d_pR/map2/map_multiobs_map2.csv");

for i = 1:1 % 4 experiments
%     x0 = 500;
%     y0 = 500;
%     width = 1290.427199;
%     height = 800;
%     figure
%     set(gcf,'position',[x0,y0,width,height])
%     tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'tight')
    
    prefix = ["map2/case"+num2str(i)+"/"];

    % read last iteration to decide the time discretizations
    means = csvread([prefix + "zk_sdf.csv"]);
    nt = size(means, 2);
    
    % read history to decide how many iterations it has
    m_iters = csvread([prefix + "zk_history.csv"]);
    cov_iters = csvread([prefix + "Sk_history.csv"]);
    
    n_iters = size(m_iters, 2) / nt;
    
    % plotting
    x0 = 500;
    y0 = 500;
    width = 1290.427199;
    height = 800;
    figure
    set(gcf,'position',[x0,y0,width,height])
    
    tiledlayout(1, n_iters, 'TileSpacing', 'tight', 'Padding', 'tight')
    
    for i_iter = 1: n_iters-1
        nexttile
        hold on
        prefix = ["map2/case"+num2str(i)+"/"];
        % % --- read means and covariances ---
        means = m_iters(:, i_iter*nt: (i_iter+1)*nt);
        covs = cov_iters(:, i_iter*nt: (i_iter+1)*nt);
        plot_2d_result(sdfmap, means, covs);
    end
        xlim([-20, 25]);
        ylim([-15, 22]);

end

%% plot costs
% plotting
x0 = 500;
y0 = 500;
width = 1290.427199;
height = 800;
figure
set(gcf,'position',[x0,y0,width,height])

tiledlayout(2, 2, 'TileSpacing', 'tight', 'Padding', 'tight')
    
for i=1:4
    prefix = ["map2/case"+num2str(i)+"/"];
    costs_i = csvread([prefix + "costs.csv"]);
    n_steps = size(costs_i, 2);
    control_cost = costs_i(1, 1:end);
    collision_cost = costs_i(2, 1:end);
    total_cost = costs_i(3, 1:end);
    nexttile
    hold on
    plot(linspace(1, n_steps, n_steps), control_cost, 'LineWidth', 2.5);
    plot(linspace(1, n_steps, n_steps), collision_cost, 'LineWidth', 2.5);
    plot(linspace(1, n_steps, n_steps), total_cost, 'LineWidth', 2.5);
    legend("Control Costs", "Collision Costs", "Total Costs");
end




