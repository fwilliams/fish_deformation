#ifndef __FISH_DEFORMATION_STATE__
#define __FISH_DEFORMATION_STATE__

#include "volume_rendering.h"
#include "utils/datfile.h"
#include "utils/utils.h"
#include "preprocessing.hpp"

#include <igl/opengl/glfw/Viewer.h>

enum class Application_State {
    Initial_File_Selection = 0,
    Segmentation,
    Meshing,
    EndPointSelection,
    BoundingPolygon,
    Straightening,
    Rasterization
};

struct State {
    Application_State application_state = Application_State::Initial_File_Selection;

    std::string volume_base_name;
    DatFile volume_file;

    GLuint index_volume = 0;
    struct {
        GLuint index_volume = 0;
        GLuint color_by_identifier = 0;
        GLuint selection_emphasis_type = 0;
        GLuint highlight_factor = 0;
    } uniform_locations_rendering;

    struct {
        GLuint index_volume = 0;
    } uniform_locations_picking;

    contourtree::TopologicalFeatures topological_features;
    GLuint contour_information_ssbo;

    Eigen::VectorXd volume_data;
    std::vector<unsigned int> index_volume_data;
    volumerendering::Volume_Rendering volume_rendering;


    bool should_select = false;

    struct Fish_Status {
        std::vector<uint32_t> feature_list;
    };
    std::vector<Fish_Status> fishes;
    size_t current_fish = 0;

    // Sorted list of selected features
    std::vector<uint32_t> total_selection_list;
    bool selection_list_is_dirty = false;
    GLuint selection_list_ssbo;


    Eigen::VectorXd skeleton_masking_volume;

    Eigen::Vector4f target_viewport_size = { -1.f, -1.f, -1.f, -1.f };
    uint64_t frame_counter = 0;
    const int Delta_Frame_Count_Until_Resize = 10;


    // Surface extraction output
    struct ExtractedSurface {
      Eigen::MatrixXd V;
      Eigen::MatrixXi F;

      Eigen::MatrixXd TV;
      Eigen::MatrixXi TT;
    } extracted_surface;
};

#endif // __FISH_DEFORMATION_STATE__