#ifndef UTILS_H
#define UTILS_H

#include <Eigen/Core>
#include <spdlog/spdlog.h>
#include <vector>
#include <array>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>


#ifdef _MSC_VER
    static constexpr int PATH_BUFFER_SIZE = 4*4096;
#else
    static const int PATH_BUFFER_SIZE = 4*PATH_MAX;
#endif

// Scale a vector so its values lie between zero and one
void scale_zero_one(const Eigen::VectorXd& V, Eigen::VectorXd& V_scaled);

// Compute approximate geodesic distance
void geodesic_distances(const Eigen::MatrixXd& TV,
                        const Eigen::MatrixXi& TT,
                        const std::vector<std::pair<int, int>>& endpoints,
                        Eigen::VectorXd& isovals,
                        bool normalized = true);

// Compute heat diffusion distances
void heat_diffusion_distances(const Eigen::MatrixXd& TV,
                                const Eigen::MatrixXi& TT,
                                const std::vector<std::pair<int, int>>& endpoints,
                                Eigen::VectorXd& isovals,
                                bool normalize);

void split_mesh_components(const Eigen::MatrixXi& TT, const Eigen::VectorXi& components, std::vector<Eigen::MatrixXi>& out);


void tet_mesh_faces(const Eigen::MatrixXi& TT, Eigen::MatrixXi& TF, bool flip=false);

void load_tet_file(const std::string& tet, Eigen::MatrixXd& TV, Eigen::MatrixXi& TF, Eigen::MatrixXi& TT);

bool load_rawfile(const std::string& rawfilename, const Eigen::RowVector3i& dims, Eigen::VectorXf &out, std::shared_ptr<spdlog::logger> logger, bool normalize = true);

bool load_rawfile(const std::string& rawfilename, const Eigen::RowVector3i& dims, std::vector<uint8_t> &out, std::shared_ptr<spdlog::logger> logger);

<<<<<<< HEAD
=======
void debug_group_action(const std::string& action, const char* message="");

>>>>>>> 35e12f7... Added dependencies for debug group functions
void edge_endpoints(const Eigen::MatrixXd& V,
                    const Eigen::MatrixXi& F,
                    Eigen::MatrixXd& V1,
                    Eigen::MatrixXd& V2);


// Check if the point pt is in the tet at ID tet
bool point_in_tet(const Eigen::MatrixXd& TV,
                  const Eigen::MatrixXi& TT,
                  const Eigen::RowVector3d& pt,
                  int tet);


// Return the index of the tet containing the point p or -1 if the vertex is in no tets
int containing_tet(const Eigen::MatrixXd& TV,
                   const Eigen::MatrixXi& TT,
                   const Eigen::RowVector3d& p);


// Return the index of the closest vertex to p
int nearest_vertex(const Eigen::MatrixXd& TV, const Eigen::RowVector3d& p);

// Compute a new mesh with only the connected component comp
void remesh_connected_components(int comp, const Eigen::VectorXi& C,
                                 const Eigen::MatrixXd& TV, const Eigen::MatrixXi& TT,
                                 Eigen::VectorXi& outCMap, Eigen::MatrixXd& outTV, Eigen::MatrixXi& outTT);

#endif // UTILS_H
