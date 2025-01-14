clear all
close all
clc

precision = read_data("precision_16.csv");
precision = precision(1:end, 1:end);
cov = read_data("cov_16.csv");
cov = cov(1:end, 1:end);
spm = sparse(precision);
K = size(precision, 1);

figure()
title('precision')
spy(precision)

%% inversion without considering the sparsity pattern
% ---------------- LDL factorization ----------------
% [L, D, P] = ldl(precision);
% ---------------- residual P'*precision*P - L*D*LT ------------------
% precision_permute = P'*precision*P;
% norm(precision_permute - L*D*L', 'fro')

% [L,D] = ldl_golub(precision);

figure()
title('L')
spy(L)

% ---------------- main algorithm ----------------
inv_full = zeros(K, K);
for k=K : -1 : 1
    for j=K : -1 : k
        cur_val = 0;
        if (j==k)
            cur_val = 1/D(j,k);
        end
        for l=k+1:K
            if(l>j)
                cur_val = cur_val - inv_full(l, j)*L(l,k);
            else
                cur_val = cur_val - inv_full(j, l)*L(l,k);
            end
        end
        inv_full(j,k) = cur_val;
    end
end

% ------- inv_sp is inv(P'*precision*P) -------
inv_full = inv_full + tril(inv_full, -1)';
% inv_full = P*inv_full*P';
figure()
spy(inv_full)

inv_true = inv(precision);
assert(norm(cov-inv_true, 'fro') < 1e-10);
norm(inv_full-inv_true, 'fro')
assert(norm(inv_full-inv_true, 'fro') < 1e-10);

%% compare time of ldlt
% disp("matlab built-in ldl")
% tic
% [L,D] = ldl(precision);
% toc
% disp("golub ldl")
% tic
% [L1,D1] = ldl_golub(spm);
% toc
disp("golub ldl")
tic
[L,D] = ldl_golub(precision);
toc
norm(precision-L*D*L', 'fro')
assert(norm(precision-L*D*L', 'fro')<1e-10)
%% inverser considering the sparsity pattern
disp("full inversion time")
tic
inv_full = inv(precision);
toc

disp("sparse inversion time")
tic
inv_sp = sparse_inv(precision);
toc
%%
inv_true = inv(precision);
assert(norm(cov-inv_true, 'fro') < 1e-10);
norm(inv_sp-inv_true, 'fro')
mask = zeros(K,K);
mask(inv_sp ~= 0) = 1;
inv_true_masked = inv_true.*mask;
norm(inv_sp-inv_true_masked, 'fro')
assert(norm(inv_sp-inv_true_masked, 'fro') < 1e-10);

% EXTERNAL ldl
% [L,D] = ldl_golub(precision);
% assert(norm(precision- L*D*L', 'fro')<1e-10);

%% compare inversion from matlab and cpp
clear all
clc
precision = read_data("precision_10.csv");
precision = precision(1:end, 1:end);
Lcpp = read_data("L_cpp.csv");
Lcpp = Lcpp(1:end, 1:end);
Dcpp = read_data("D_cpp.csv");
Dcpp = Dcpp(1:end, 1:end);

% %
% [L, D] = ldl_golub(precision);

K = size(precision, 1);
% ---------------- LDL factorization ----------------
Lsp = sparse(Lcpp);
[row, col, ~] = find(Lsp);
nnz = size(row, 1);

% --------------- main algorithm -----------------
inv_sp = zeros(K, K);
for index=nnz:-1:1
    j = row(index);
    k = col(index);
    cur_val = 0;
    if (j==k)
        cur_val = 1/Dcpp(j,j);
    end

    start_index = index;
    for s_index=index:-1:1
        if (col(s_index) < k)
            start_index = s_index+1;
            break;
        end
        if s_index == 1
            start_index = 2;
        end
    end
    for l_indx=start_index : nnz
        l = row(l_indx);
        if (col(l_indx)>k)
            break;
        end
        % ------------ inv_sp(j,l) * L(l,k) ------------
        if(l>j)
            cur_val = cur_val - inv_sp(l,j)*Lsp(l,k);
        else
            cur_val = cur_val - inv_sp(j,l)*Lsp(l,k);
        end
    end
    inv_sp(j,k) = cur_val;
end

inv_sp = inv_sp + tril(inv_sp, -1)';

% --------------- verification --------------- 
inv_true = inv(precision);
mask = zeros(K,K);
mask(inv_sp ~= 0) = 1;
inv_true_masked = inv_true.*mask;
diff_matlab = inv_sp - inv_true_masked;
disp("diff_matlab: ");
norm(diff_matlab, 'fro')
% verification cpp results
inv_cpp = read_data("inv_computed.csv");
inv_cpp = inv_cpp(1:end, 1:end);
diff_cpp = inv_cpp - inv_true_masked;
disp("diff_cpp: ");
norm(diff_cpp, 'fro')
assert(norm(diff_cpp, 'fro')<1e-10);


%%
covariance
covariancefullsp