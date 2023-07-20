clear all
close all
clc

% =================== problem settings =================== 
nx = 6;
nu = 2;

% time scaling
sig = 2.0;

%%
m0 = [4; 4; 0.0;0; 0; 0];
Sig0  =  0.001 .* eye(6);

m1 = [2; 2; 0; 0; 0; 0];
Sig1 = 0.001 .* eye(6);

[A1, B1, a1] = linearization(m0);

% pinvBBT = [   0         0         0         0         0         0;
%                          0         0         0         0         0         0;
%                          0         0         0         0         0         0;
%                          0         0         0         0         0         0;
%                          0         0         0         0    0.1181   0;
%                          0         0         0         0         0    1.173512e-04];

pinvBBT = [   0         0         0         0         0         0;
                         0         0         0         0         0         0;
                         0         0         0         0         0         0;
                         0         0         0         0         0         0;
                         0         0         0         0        0.01    0;
                         0         0         0         0         0        0.01];

eta = 1e-6;
epsilon = 1e-2;
stop_err = 1e-5;

% state cost
Q1 = zeros(nx, nx);
% Q1(6,6) = 0.1;
% Q1(3,3) = 1;

nt = 500;

% 3D matrices
A  = A1(:)*ones(1,nt);
As  = reshape(A,[nx,nx,nt]);
B  = B1(:)*ones(1,nt);
B  = reshape(B,[nx,nu,nt]);
r  = zeros(nx,nt);
as = a1(:)*ones(1,nt);
Q  = Q1(:)*ones(1,nt);
Q  = reshape(Q,[nx,nx,nt]);

% profile on
% tic_solving = tic;

[K,d, As, B, as,zk,Sk] = optimize(nt, As, B, pinvBBT, as, Q, epsilon, m0, Sig0, m1, Sig1, eta, sig, stop_err);

disp("Are all the covariances PSD?")
is_all_psd(Sk)

% solving_time = toc(tic_solving)
% profile viewer

%% recover control
[Skstar, zkstar]     = Szk(As,B,as,epsilon,m0,Sig0,sig);
[hAstar,hastar,nTr] = linearAa_pquad(Skstar,zkstar,As);

% linearization_error(hAstar, hastar, B, zkstar, nt, sig);
Qstar = Q;
rstar= zeros(nx, nt);
for i = 1:nt
    rstar(:, i) = (-2*Q(:,:,i)*zkstar(:, i) + nTr(:, i))/2;
end

[Ks,ds] = linearCov(hAstar,hastar,B,epsilon,Qstar,rstar,m0,Sig0,m1,Sig1,sig);

%% plot a prior trejectories and samples
% ========================= uncontrolled dynamics ========================= 
dt = sig/(nt-1);
t = linspace(0,sig,nt);
n_samples = 20;

x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'none', 'Padding', 'none')

nexttile
% t.FontSize = 14;
% grid on
hold on
% --------------------- plot samples --------------------- 
for i_sample=1:n_samples
    xs = zeros(6, nt);
    xs(:,1) = m0 + Sig0^(1/2)* randn(6, 1);
    xt = xs(:,1);
    for i=1:nt-1
        xt = xt + (hAstar(:,:,i)*xt + hastar(i)) .* dt + sqrt(epsilon*dt).*B(:,:,i) * randn(2,1);
        xs(:,i+1) = xt;
    end
    plot3(t, xs(1, :), xs(2, :), 'LineWidth', 1);
end

% --------------------- plot mean --------------------- 
mus = zeros(6, nt);
mus(:,1) = m0;
xt = m0;
for i=1:nt-1
    xt = xt + (hAstar(:,:,i)*xt + hastar(i)) .* dt;
    mus(:,i+1) = xt;
end
plot3(t, mus(1, :), mus(2, :), 'r', 'LineWidth', 7.5);

% --------------------- plot covariance --------------------- 
Sigk = Sig0;
Sigs = zeros(6,6,nt);
Sigs(:,:,1) = Sig0;
for i=1:nt-1
    Sigk = Sigk + (hAstar(:,:,i)*Sigk + Sigk*hAstar(:,:,i)' + epsilon*B(:,:,i)*B(:,:,i)').*dt;
    Sigs(:,:,i+1) = Sigk;
end
plot_marginal(m0, mus, Sig0(1:2,1:2), Sigs(1:2,1:2,:), nt, sig, 100);

xlabel('Time $t$','Interpreter','latex', 'FontSize',18),ylabel('Position $x$','Interpreter','latex', 'FontSize',18);
zlabel('Position $y$','Interpreter','latex', 'FontSize',18);

% ========================= plot the controlled linear system ========================= 

x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'none', 'Padding', 'none')

nexttile
% t.FontSize = 14;
% grid on
hold on
% --------------------- plot samples --------------------- 
for i_sample=1:n_samples
    xs = zeros(6, nt);
    xs(:,1) = m0 + Sig0^(1/2)* randn(6, 1);
    xt = xs(:,1);
    for i=1:nt-1
        Acl = hAstar(:,:,i) + B(:,:,i)*Ks(:,:,i);
        acl = hastar(i) + B(:,:,i)*ds(:,i);
        xt = xt + (Acl*xt + acl) .* dt +  +  sqrt(epsilon*dt).*B(:,:,i) * randn(2,1);
        xs(:,i+1) = xt;
    end
    plot3(t, xs(1, :), xs(2, :), 'LineWidth', 1);
end

% --------------------- plot mean --------------------- 
mus = zeros(6, nt);
mus(:,1) = m0;
mu = m0;
for i=1:nt-1
    Acl = hAstar(:,:,i) + B(:,:,i)*Ks(:,:,i);
    acl = hastar(i) + B(:,:,i)*ds(:,i);
    mu = mu + (Acl*mu + acl) .* dt;
    mus(:,i+1) = mu;
end
plot3(t, mus(1, :), mus(2, :), 'r', 'LineWidth', 7.5);

% --------------------- plot covariance --------------------- 
Sigk = Sig0;
Sigs = zeros(6,6,nt);
Sigs(:,:,1) = Sig0;
for i=1:nt-1
    Acl = hAstar(:,:,i) + B(:,:,i)*Ks(:,:,i);
    acl = hastar(i) + B(:,:,i)*ds(:,i);
    Sigk = Sigk + (Acl*Sigk + Sigk*Acl' + epsilon*B(:,:,i)*B(:,:,i)').*dt;
    Sigs(:,:,i+1) = Sigk;
%     error_ellipse_easy(Sk, mu(:,i+1),[],t);
end
plot_marginal(m0, mus, Sig0(1:2,1:2), Sigs(1:2,1:2,:), nt, sig, 100);

xlabel('Time $t$','Interpreter','latex', 'FontSize',18),ylabel('Position $x$','Interpreter','latex', 'FontSize',18);
zlabel('Position $y$','Interpreter','latex', 'FontSize',18);

%% plotting
% =================== ellipsoids =================== 
x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'none', 'Padding', 'none')
nexttile
hold on 

% ---------------------------- positions ---------------------------- 
step_size = 100;
Sig0_xy = Sig0(1:2, 1:2, 1:end);
Sk_xy = Sk(1:2, 1:2, 1:end);
zk_2d = zk(1:2, :);
plot_marginal(m0, zk_2d, Sig0_xy, Sk_xy, nt, sig, step_size);
set(gca,'fontsize',16);
xlabel('Time $t$','Interpreter','latex'),ylabel('Position $x$','Interpreter','latex');
zlabel('Position $y$','Interpreter','latex');


% ---------------------------- sampled trajectories ----------------------------
n2=6;

t  = linspace(0,sig,nt);
cstring='grbcmk';
for j=1:n2
    init = randn(6,1);
    init = Sig0^(1/2)*init+m0;
    [~,x,u] = planar_quadrotor(Ks,ds,epsilon,init,sig);

    plot3(t,x(1,:),x(2,:),cstring(mod(j,6)+1),'LineWidth',2);
end

% plot the mean quadrotor trajectory
x0 = 50;
y0 = 50;
width = 400;
height = 350;
t  = linspace(0,sig,nt);
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'none')
nexttile
hold on 
step_size = 100;

for ti=1:30:nt
    mean_x = zk(1, ti);
    mean_y = zk(2, ti);
    angle_phi = zk(3, ti) / pi * 180;
    draw_planarquad([mean_x,mean_y], 0.25, 0.05, angle_phi, [0.1,0.1,0.1]);
end

%
% ---------------------------- velocities ---------------------------- 
x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'tight', 'Padding', 'none')
nexttile
hold on 

n1=nt;
n2=6;
n=2;
x0=zeros(n,n2);
x1=zeros(n,n2);

Sig0_vxy = Sig0(3:4, 3:4, 1:end);
Sk_vxy = Sk(3:4, 3:4, 1:end);
vzk_2d = zk(3:4, :);
v0 = m0(4:5);
plot_marginal(v0, vzk_2d, Sig0_vxy, Sk_vxy, nt, sig, step_size);
set(gca,'fontsize',16);
xlabel('Time $t$','Interpreter','latex'),ylabel('Velocity $v_x$','Interpreter','latex');
zlabel('Velocity $v_y$','Interpreter','latex');
% xlim([0,5])

% ---------------------------- sampled trajectories ----------------------------
cstring='grbcmk';
for j=1:n2
    init = randn(nx,1);
    init = Sig0^(1/2)*init+m0;
    [~,x,u] = planar_quadrotor(Ks,ds,epsilon,init,sig);
    x0(:,j)=x(1:2,1);
    x1(:,j)=x(1:2,n1);
    plot3(t,x(3,:),x(4,:),cstring(mod(j,6)+1),'LineWidth',2);
end

% =================== trajectories and control inputs =================== 
n1=nt;
n2=6;
n=2;
x0=zeros(n,n2);
x1=zeros(n,n2);

x0 = 50;
y0 = 50;
width = 400;
height = 350;
figure
set(gcf,'position',[x0,y0,width,height])
tiledlayout(1, 1, 'TileSpacing', 'none', 'Padding', 'none')
hold on
cstring='grbcmk';
for j=1:n2
    init = randn(nx,1);
    init = Sig0^(1/2)*init+m0;
    [~,x,u] = planar_quadrotor(Ks,ds,epsilon,init,sig);
    plot(t,u(1,:),cstring(mod(j,6)+1),'LineWidth',2);
end
hold off;

xlabel('Time $t$','Interpreter','latex'),ylabel('Control input $u$','Interpreter','latex');
set(gca,'fontsize',16);
