#ifndef COMPACT_DESCRIPTOR_H
#define COMPACT_DESCRIPTOR_H

#include <Eigen/Dense>
#include <vector>

class Structure;
class DescriptorValues;

// Descriptor calculator.
// TODO: Rename to DescriptorCalculator.
class CompactDescriptor {
public:
  CompactDescriptor();

  virtual DescriptorValues compute_struc(Structure &structure) = 0;
};

// TODO: Rename to StructureDescriptor.
class DescriptorValues {
public:
  DescriptorValues();

  // Descriptor attributes.
  int n_descriptors, n_types, n_atoms;
  double volume;

  std::vector<Eigen::MatrixXd> descriptors, descriptor_force_dervs,
      neighbor_coordinates;
  std::vector<Eigen::VectorXd> descriptor_norms, descriptor_force_dots,
      cutoff_values, cutoff_dervs;
  std::vector<Eigen::VectorXi> neighbor_counts, cumulative_neighbor_counts,
      atom_indices, neighbor_indices;

  // TODO: Rename to n_clusters_by_type.
  std::vector<int> n_atoms_by_type, n_neighbors_by_type;
};

class ClusterDescriptor {
public:
  ClusterDescriptor();
  ClusterDescriptor(const DescriptorValues &structure);

  std::vector<Eigen::MatrixXd> descriptors;
  std::vector<Eigen::VectorXd> descriptor_norms, cutoff_values;
  std::vector<int> type_count, cumulative_type_count;
  int n_descriptors, n_types;
  int n_clusters = 0;

  // Add all clusters in a structure.
  void initialize_cluster(int n_types, int n_descriptors);
  void add_cluster(const DescriptorValues &structure);

  // TODO: Allow specific clusters to be added.
};

#endif
