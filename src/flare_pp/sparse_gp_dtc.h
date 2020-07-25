#ifndef SPARSE_GP_DTC_H
#define SPARSE_GP_DTC_H

#include "sparse_gp.h"
#include <Eigen/Dense>

class SparseGP_DTC : public SparseGP {
public:
  // TODO: Modify "add" methods to keep track of each kernel contribution.
  std::vector<Eigen::MatrixXd> Kuf_env_kernels, Kuu_kernels,
    Kuf_struc_energy, Kuf_struc_force, Kuf_struc_stress;
  Eigen::VectorXd noise_vector, y, energy_labels, force_labels, stress_labels;
  Eigen::MatrixXd Sigma, Kuu_inverse, Kuf;

  int n_energy_labels = 0, n_force_labels = 0, n_stress_labels = 0;

  // Likelihood attributes.
  double log_marginal_likelihood, data_fit, complexity_penalty, trace_term,
    constant_term;
  Eigen::VectorXd likelihood_gradient;

  SparseGP_DTC();
  SparseGP_DTC(std::vector<Kernel *> kernels, double sigma_e, double sigma_f,
               double sigma_s);

  // Methods for augmenting the training set.
  void add_sparse_environments(const std::vector<LocalEnvironment> &envs);
  void add_training_structure(const StructureDescriptor &training_structure);

  // Not implemented.
  void add_sparse_environment(const LocalEnvironment &env);
  void add_training_environment(const LocalEnvironment &training_environment);
  void add_training_environments(const std::vector<LocalEnvironment> &envs);

  // Update matrices and vectors needed for mean and variance prediction:
  // Sigma, Kuu_inverse, and alpha.
  void update_matrices();

  // Compute the DTC mean and variance.
  void predict_DTC(StructureDescriptor test_structure,
      Eigen::VectorXd & mean_vector, Eigen::VectorXd & variance_vector,
      std::vector<Eigen::VectorXd> & mean_contributions);

  // Calculate the log marginal likelihood of the current hyperparameters and its gradient.
  void compute_DTC_likelihood();
  void compute_VFE_likelihood();

  // Change the model hyperparameters and covariance matrices.
  void set_hyperparameters(Eigen::VectorXd hyperparameters);
};

// TODO: Invert noise in likelihood expression.
double compute_likelihood_gradient(const SparseGP_DTC &sparse_gp,
                                   const Eigen::VectorXd &hyperparameters,
                                   Eigen::VectorXd &like_grad);

#endif
