clear all
close all
clc
addpath('/usr/local/gtsam_toolbox')
import gtsam.*
import gpmp2.*

%% read map
sdfmap = csvread("../../RAL-examples/2d_pR/map2/map_multiobs_map2.csv");
addpath("../")

x0 = 500;
y0 = 500;
width = 1290.427199;
height = 800;
figure
set(gcf,'position',[x0,y0,width,height])

tiledlayout(2, 2, 'TileSpacing', 'tight', 'Padding', 'tight')

for i = 1:4 % 4 experiments
%     x0 = 500;
%     y0 = 500;
%     width = 1290.427199;
%     height = 800;
%     figure
%     set(gcf,'position',[x0,y0,width,height])
%     tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'tight')

    nexttile
    hold on
    prefix = ["map2/case"+num2str(i)+"/"];
    % % --- read means and covariances ---
    means = csvread([prefix + "zk_sdf.csv"]);
    covs = csvread([prefix + "Sk_sdf.csv"]);

    % --- read baselines ---
    means_prm = csvread([prefix + "prm_5000.csv"]);
    means_rrt = csvread([prefix + "RRTstar_3000.csv"]);
    
    addpath("../error_ellipse");
    addpath("../../../matlab_helpers/");
    
    plot_2d_result(sdfmap, means, covs);

    % --- plot baselines ---
    [~, nt_prm] = size(means_prm);
    [~, nt_rrt] = size(means_rrt);
    
    for i_pt = 1:nt_prm-1
        plot([means_prm(1, i_pt), means_prm(1, i_pt+1)], ...
            [means_prm(2, i_pt), means_prm(2, i_pt+1)], 'LineWidth', 3.0, 'Color', 'b');
    end
    
    hold on

    for i_pt = 1:nt_rrt-1
        plot([means_rrt(1, i_pt), means_rrt(1, i_pt+1)], ...
            [means_rrt(2, i_pt), means_rrt(2, i_pt+1)], 'LineWidth', 3.0, 'Color', 'g');
    end
    
%     if i==3
        xlim([-20, 25]);
        ylim([-15, 22]);
%     end
%     axis off ; 

end
