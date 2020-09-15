#include "sparse_gp_dtc.h"
#include "test_sparse_gp.h"
#include <chrono>

TEST_F(SparseTest, DTC_Prediction) {
  double sigma_e = 1;
  double sigma_f = 2;
  double sigma_s = 3;

  SparseGP_DTC sparse_gp = SparseGP_DTC(kernels, sigma_e, sigma_f, sigma_s);

  LocalEnvironment env1 = test_struc.local_environments[0];
  LocalEnvironment env2 = test_struc.local_environments[1];
  std::vector<LocalEnvironment> sparse_envs{env1, env2};
  sparse_gp.add_sparse_environments(sparse_envs);
  sparse_gp.add_training_structure(test_struc);

  EXPECT_EQ(sparse_gp.Sigma.rows(), 0);
  EXPECT_EQ(sparse_gp.Kuu_inverse.rows(), 0);

  sparse_gp.update_matrices();

  EXPECT_EQ(sparse_gp.sparse_environments.size(), sparse_gp.Sigma.rows());
  EXPECT_EQ(sparse_gp.sparse_environments.size(), sparse_gp.Kuu_inverse.rows());

  sparse_gp.predict_on_structure(test_struc);

  // Check that mean contributions sum to the total.
  int mean_size = test_struc.mean_efs.size();
  EXPECT_EQ(mean_size, 1 + test_struc.noa * 3 + 6);
  Eigen::VectorXd mean_sum = Eigen::VectorXd::Zero(mean_size);
  for (int i = 0; i < sparse_gp.kernels.size(); i++) {
    mean_sum += test_struc.mean_contributions[i];
  }

  double threshold = 1e-8;
  for (int i = 0; i < mean_size; i++) {
    EXPECT_NEAR(mean_sum[i], test_struc.mean_efs[i], threshold);
  }

  // Check that Kuu and Kuf kernels sum to the total.
  int n_sparse = sparse_gp.Kuu.rows();

  Eigen::MatrixXd Kuu_sum = Eigen::MatrixXd::Zero(n_sparse, n_sparse);
  Eigen::MatrixXd Kuf_sum = Eigen::MatrixXd::Zero(sparse_gp.Kuf_struc.rows(),
                                                  sparse_gp.Kuf_struc.cols());
  for (int i = 0; i < kernels.size(); i++) {
    Kuu_sum += sparse_gp.Kuu_kernels[i];
    Kuf_sum.block(0, 0, n_sparse, sparse_gp.n_energy_labels) +=
        sparse_gp.Kuf_struc_energy[i];
    Kuf_sum.block(0, sparse_gp.n_energy_labels, n_sparse,
                  sparse_gp.n_force_labels) += sparse_gp.Kuf_struc_force[i];
    Kuf_sum.block(0, sparse_gp.n_energy_labels + sparse_gp.n_force_labels,
                  n_sparse, sparse_gp.n_stress_labels) +=
        sparse_gp.Kuf_struc_stress[i];
  }

  for (int i = 0; i < sparse_gp.Kuu.rows(); i++) {
    for (int j = 0; j < sparse_gp.Kuu.cols(); j++) {
      EXPECT_NEAR(Kuu_sum(i, j), sparse_gp.Kuu(i, j), threshold);
    }
  }

  for (int i = 0; i < sparse_gp.Kuf_struc.rows(); i++) {
    for (int j = 0; j < sparse_gp.Kuf_struc.cols(); j++) {
      EXPECT_NEAR(Kuf_sum(i, j), sparse_gp.Kuf_struc(i, j), threshold);
    }
  }

  // Check that the variances on all quantities are positive.
  for (int i = 0; i < mean_size; i++) {
    EXPECT_GE(test_struc.variance_efs[i], 0);
  }

  // Compute the marginal likelihood.
  sparse_gp.compute_likelihood();
  EXPECT_EQ(sparse_gp.data_fit + sparse_gp.complexity_penalty +
                sparse_gp.constant_term,
            sparse_gp.log_marginal_likelihood);
  double like1 = sparse_gp.log_marginal_likelihood;

  // Check the likelihood function.
  Eigen::VectorXd hyps = sparse_gp.hyperparameters;
  double like2 = sparse_gp.compute_likelihood_gradient(hyps);

  EXPECT_EQ(like1, like2);
}

TEST_F(SparseTest, Update_QR) {
  double sigma_e = 1;
  double sigma_f = 2;
  double sigma_s = 3;

  SparseGP_DTC sparse_gp = SparseGP_DTC(kernels, sigma_e, sigma_f, sigma_s);

  sparse_gp.add_sparse_environments(test_struc.local_environments);
  sparse_gp.add_training_structure(test_struc);

  SparseGP_DTC sparse_gp_qr = sparse_gp;

  sparse_gp.update_matrices();
  sparse_gp_qr.update_matrices_QR();

  // Check for agreement in alpha.
  for (int i = 0; i < sparse_gp.alpha.size(); i++) {
    EXPECT_NEAR(sparse_gp.alpha(i), sparse_gp_qr.alpha(i), 1e-14);
  }

  // Check for agreement in Sigma.
  for (int i = 0; i < sparse_gp.Sigma.rows(); i++) {
    for (int j = 0; j < sparse_gp.Sigma.cols(); j++) {
      EXPECT_NEAR(sparse_gp.Sigma(i, j), sparse_gp_qr.Sigma(i, j), 1e-13);
    }
  }

  // Check for agreement in Kuu_inverse.
  for (int i = 0; i < sparse_gp.Kuu_inverse.rows(); i++) {
    for (int j = 0; j < sparse_gp.Kuu_inverse.cols(); j++) {
      EXPECT_NEAR(sparse_gp.Kuu_inverse(i, j), sparse_gp_qr.Kuu_inverse(i, j),
                  1e-14);
    }
  }
}

TEST_F(SparseTest, Set_Hyps) {
  // Check the reset hyperparameters method.

  int power = 2;
  double sig1 = 1.5, sig2 = 2.0, sig3 = 2.5, sig4 = 3.0, sig_e_1 = 1.0,
         sig_e_2 = 2.0, sig_f_1 = 1.5, sig_f_2 = 2.5, sig_s_1 = 3.0,
         sig_s_2 = 3.5;

  DotProductKernel kernel_1 = DotProductKernel(sig1, power, 0);
  DotProductKernel kernel_2 = DotProductKernel(sig2, power, 1);
  DotProductKernel kernel_3 = DotProductKernel(sig3, power, 0);
  DotProductKernel kernel_4 = DotProductKernel(sig4, power, 1);

  std::vector<Kernel *> kernels_1{&kernel_1, &kernel_2};
  std::vector<Kernel *> kernels_2{&kernel_3, &kernel_4};

  SparseGP_DTC sparse_gp_1 = SparseGP_DTC(kernels_1, sig_e_1, sig_f_1, sig_s_1);
  SparseGP_DTC sparse_gp_2 = SparseGP_DTC(kernels_2, sig_e_2, sig_f_2, sig_s_2);

  // Add sparse environments and training structures.
  LocalEnvironment env1 = test_struc.local_environments[0];
  LocalEnvironment env2 = test_struc.local_environments[1];
  std::vector<LocalEnvironment> sparse_envs{env1, env2};
  sparse_gp_1.add_sparse_environments(sparse_envs);
  sparse_gp_1.add_training_structure(test_struc);
  sparse_gp_2.add_sparse_environments(sparse_envs);
  sparse_gp_2.add_training_structure(test_struc);

  sparse_gp_1.update_matrices();
  sparse_gp_2.update_matrices();

  // Compute likelihoods.
  sparse_gp_1.compute_likelihood();
  sparse_gp_2.compute_likelihood();

  EXPECT_NE(sparse_gp_1.log_marginal_likelihood,
            sparse_gp_2.log_marginal_likelihood);

  // Reset the hyperparameters of the second GP.
  Eigen::VectorXd new_hyps(5);
  new_hyps << sig1, sig2, sig_e_1, sig_f_1, sig_s_1;
  sparse_gp_2.set_hyperparameters(new_hyps);

  sparse_gp_2.compute_likelihood();

  EXPECT_EQ(sparse_gp_1.log_marginal_likelihood,
            sparse_gp_2.log_marginal_likelihood);
}

TEST_F(SparseTest, LikeGrad) {
  // Check that the DTC likelihood gradient is correctly computed.

  double sigma_e = 1;
  double sigma_f = 2;
  double sigma_s = 3;

  SparseGP_DTC sparse_gp = SparseGP_DTC(kernels, sigma_e, sigma_f, sigma_s);

  LocalEnvironment env1 = test_struc.local_environments[0];
  LocalEnvironment env2 = test_struc.local_environments[1];
  std::vector<LocalEnvironment> sparse_envs{env1, env2};
  sparse_gp.add_sparse_environments(sparse_envs);
  sparse_gp.add_training_structure(test_struc);

  EXPECT_EQ(sparse_gp.Sigma.rows(), 0);
  EXPECT_EQ(sparse_gp.Kuu_inverse.rows(), 0);

  sparse_gp.update_matrices();

  // Check the likelihood function.
  Eigen::VectorXd hyps = sparse_gp.hyperparameters;
  sparse_gp.compute_likelihood_gradient(hyps);
  Eigen::VectorXd like_grad = sparse_gp.likelihood_gradient;

  int n_hyps = hyps.size();
  Eigen::VectorXd hyps_up, hyps_down;
  double pert = 1e-4, like_up, like_down, fin_diff;

  for (int i = 0; i < n_hyps; i++) {
    hyps_up = hyps;
    hyps_down = hyps;
    hyps_up(i) += pert;
    hyps_down(i) -= pert;

    like_up = sparse_gp.compute_likelihood_gradient(hyps_up);
    like_down = sparse_gp.compute_likelihood_gradient(hyps_down);

    fin_diff = (like_up - like_down) / (2 * pert);

    EXPECT_NEAR(like_grad(i), fin_diff, 1e-8);
  }
}

TEST_F(SparseTest, AddOrder) {
  double sigma_e = 1;
  double sigma_f = 2;
  double sigma_s = 3;

  SparseGP_DTC sparse_gp_1 = SparseGP_DTC(kernels, sigma_e, sigma_f, sigma_s);
  SparseGP_DTC sparse_gp_2 = SparseGP_DTC(kernels, sigma_e, sigma_f, sigma_s);

  LocalEnvironment env1 = test_struc.local_environments[0];
  LocalEnvironment env2 = test_struc.local_environments[1];
  std::vector<LocalEnvironment> sparse_envs{env1, env2};

  // Add structure first.
  sparse_gp_1.add_training_structure(test_struc);
  sparse_gp_1.add_sparse_environments(sparse_envs);
  sparse_gp_1.update_matrices();

  // Add environments first.
  sparse_gp_2.add_sparse_environments(sparse_envs);
  sparse_gp_2.add_training_structure(test_struc);
  sparse_gp_2.update_matrices();

  // Check that matrices match.
  for (int i = 0; i < sparse_gp_1.Kuf.rows(); i++) {
    for (int j = 0; j < sparse_gp_1.Kuf.cols(); j++) {
      EXPECT_EQ(sparse_gp_1.Kuf(i, j), sparse_gp_2.Kuf(i, j));
    }
  }

  for (int i = 0; i < sparse_gp_1.Kuu.rows(); i++) {
    for (int j = 0; j < sparse_gp_1.Kuu.cols(); j++) {
      EXPECT_EQ(sparse_gp_1.Kuu(i, j), sparse_gp_2.Kuu(i, j));
    }
  }
}

// TEST(QRTest, QRTest){
//     Eigen::MatrixXd A(3, 3);
//     A << 1, 2, 3, 4, 5, 6, 7, 8, 9;
//     Eigen::MatrixXd thinQ(Eigen::MatrixXd::Identity(5,3)), Q, QR;
//     Eigen::HouseholderQR<Eigen::MatrixXd> qr(A);
//     Q = qr.householderQ();
//     QR = qr.matrixQR();
//     thinQ = qr.householderQ() * thinQ;
//     std::cout << "The complete unitary matrix Q is:\n" << Q << "\n\n";
//     std::cout << "The thin matrix Q is:\n" << thinQ << "\n\n";
//     std::cout << "The full QR matrix is\n" << QR << "\n\n";
// }

// TEST(QR_Col_Test, QR_Col_Test){
//     Eigen::MatrixXd A(3, 3);
//     A << 1, 2, 3, 4, 5, 6, 7, 8, 9;
//     Eigen::MatrixXd P, Q, QR;
//     Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(A);
//     P = qr.colsPermutation();
//     Q = qr.householderQ();
//     QR = qr.matrixQR();
//     std::cout << "The permutation matrix P is:\n" << P << "\n\n";
//     std::cout << "The complete unitary matrix Q is:\n" << Q << "\n\n";
//     std::cout << "The full QR matrix is\n" << QR << "\n\n";
//     std::cout << "The rank of A is\n" << qr.rank() << "\n\n";
// }

// TEST(Assignment_Test, Assignment_Test){
//     Eigen::MatrixXd A(3, 3);
//     A << 1, 2, 3, 4, 5, 6, 7, 8, 9;
//     Eigen::MatrixXd B = A;
//     B /= 2;
//     std::cout << A << std::endl;
//     std::cout << B << std::endl;

// }

// TEST(QR_Col_Test, QR_Col_Test){
//     // Benchmark various decompositions.
//     Eigen::MatrixXd matrix1 = Eigen::MatrixXd::Random(1000, 1000);
//     Eigen::MatrixXd matrix2 = Eigen::MatrixXd::Random(1000, 1000);
//     Eigen::VectorXd vector = Eigen::VectorXd::Random(1000);
//     Eigen::MatrixXd matrix1_inv, matmul, c_solve, h_solve, piv_inv;
//     Eigen::VectorXd vec1, vec2, vec3;

//     auto start = std::chrono::steady_clock::now();
//     Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(matrix1);
//     auto end = std::chrono::steady_clock::now();
//     std::chrono::duration<double> elapsed_seconds = end-start;
//     std::cout << "QR decomposition (ColPiv): " <<
//         elapsed_seconds.count() << "s\n";

//     start = std::chrono::steady_clock::now();
//     Eigen::HouseholderQR<Eigen::MatrixXd> qr2(matrix1);
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "QR decomposition (Householder): " <<
//         elapsed_seconds.count() << "s\n";

//     start = std::chrono::steady_clock::now();
//     Eigen::PartialPivLU<Eigen::MatrixXd> piv(matrix1);
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "PartialPivLU decomposition: " <<
//         elapsed_seconds.count() << "s\n";
//     std::cout << "Determinant:" << piv.determinant() << std::endl;

//     start = std::chrono::steady_clock::now();
//     piv_inv = piv.inverse();
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "PartialPivLU inverse: " <<
//         elapsed_seconds.count() << "s\n";

//     start = std::chrono::steady_clock::now();
//     matrix1_inv = matrix1.inverse();
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "Matrix inverse: " << elapsed_seconds.count() << "s\n";

//     start = std::chrono::steady_clock::now();
//     matmul = matrix1_inv * matrix2;
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "Multiplication: " << elapsed_seconds.count() << "s\n";

//     start = std::chrono::steady_clock::now();
//     c_solve = qr.solve(matrix2);
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "QR solve (ColPiv): " << elapsed_seconds.count() << "s\n";

//     start = std::chrono::steady_clock::now();
//     h_solve = qr2.solve(matrix2);
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "QR solve (Householder): " <<
//         elapsed_seconds.count() << "s\n";

//     std::cout << matmul(0, 0) << std::endl;
//     std::cout << c_solve(0, 0) << std::endl;
//     std::cout << h_solve(0, 0) << std::endl;

//     start = std::chrono::steady_clock::now();
//     vec1 = matrix1_inv * vector;
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "Matrix/vector multiplication: " << elapsed_seconds.count()
//     << "s\n";

//     start = std::chrono::steady_clock::now();
//     vec2 = qr.solve(vector);
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "QR vector solve (ColPiv): " << elapsed_seconds.count() <<
//     "s\n";

//     start = std::chrono::steady_clock::now();
//     vec3 = qr2.solve(vector);
//     end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     std::cout << "QR vector solve (Householder): " <<
//         elapsed_seconds.count() << "s\n";
// }

// TEST(TimeMat, TimeMat){
//     // Benchmark various decompositions.
//     Eigen::MatrixXd matrix1 = Eigen::MatrixXd::Random(20000, 5000);
//     Eigen::MatrixXd inv;

//     auto start = std::chrono::steady_clock::now();
//     Eigen::HouseholderQR<Eigen::MatrixXd> qr(matrix1);
//     // inv = matrix1.inverse();
//     auto end = std::chrono::steady_clock::now();
//     std::chrono::duration<double> elapsed_seconds = end-start;
//     std::cout << "QR decomposition: " <<
//         elapsed_seconds.count() << "s\n";
// }
