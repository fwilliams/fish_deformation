#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include <igl/marching_tets.h>

#include <Eigen/Core>

#include <vector>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <array>


class DeformationConstraints {
  double one_pair_bone_constraints(
      const Eigen::MatrixXd& TV,
      const Eigen::MatrixXi& TT,
      const Eigen::VectorXd& geodesic_distances,
      const std::array<int, 2>& endpoints,
      const Eigen::RowVector3d& straight_dir,
      const Eigen::RowVector3d& straight_origin,
      int num_verts);

  double one_pair_bone_constraints(
      const Eigen::MatrixXd& TV_fat,
      const Eigen::MatrixXi& TT_fat,
      const Eigen::MatrixXd& TV_thin,
      const Eigen::MatrixXi& TT_thin,
      const Eigen::VectorXd& geodesic_distances,
      const std::array<int, 2>& endpoints,
      const Eigen::RowVector3d& straight_dir,
      const Eigen::RowVector3d& straight_origin,
      int num_verts);

    void scale_bone_constraints(double amount, std::vector<Eigen::RowVector3d>& scaled);

public:
  static bool validate_endpoint_pairs(const std::vector<std::array<int, 2>>& endpoints, const Eigen::VectorXi& components);

  float m_scale = 1.0f;
  std::vector<int> m_bone_constraints_idx;
  std::vector<Eigen::RowVector3d> m_bone_constraints_pos;
  std::vector<int> m_orientation_constraints_idx;
  std::vector<Eigen::RowVector3d> m_orientation_constraints_pos;

//  std::vector<double> m_level_set_isovalues;
//  std::vector<double> m_level_set_distances;

  // Indices of tets which can possibly have rotation constraints
  std::vector<int> m_constrainable_tets_idx;
  std::unordered_map<int, std::tuple<int, double, bool>> m_tet_constraints;

  std::pair<Eigen::Matrix3d, Eigen::RowVector3d> frame_for_tet(const Eigen::MatrixXd& TV_fat,
                                                               const Eigen::MatrixXi& TT_fat,
                                                               int idx, double angle,
                                                               bool flip_x=false,
                                                               int n_lookahead=4);

  int num_constraints() const;

  int num_orientation_constraints() const;

  int num_bone_constraints() const;

  int num_constrainable_tets() const;

  void clear_orientation_constraints();

  double update_bone_constraints(const Eigen::MatrixXd& TV,
                                 const Eigen::MatrixXi& TT,
                                 const Eigen::VectorXd& isovals,
                                 const Eigen::VectorXi& components,
                                 const std::vector<std::array<int, 2>>& endpoint_pairs,
                                 int num_verts);

  double update_bone_constraints(const Eigen::MatrixXd& TV_fat,
                                 const Eigen::MatrixXi& TT_fat,
                                 const Eigen::MatrixXd& TV_thin,
                                 const Eigen::MatrixXi& TT_thin,
                                 const Eigen::VectorXd& geodesic_distances,
                                 const Eigen::VectorXi& components,
                                 const std::vector<std::array<int, 2>>& endpoint_pairs,
                                 int num_verts);

  void update_orientation_constraint(const Eigen::MatrixXd& TV,
                                     const Eigen::MatrixXi& TT,
                                     int idx, double angle, bool flipped_x);

  void slim_constraints(Eigen::VectorXi& slim_b, Eigen::MatrixXd& slim_bc, bool only_ends_and_tets);
};


#endif // CONSTRAINTS_H
