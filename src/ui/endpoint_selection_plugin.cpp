#include "endpoint_selection_plugin.h"

#include "state.h"
#include "utils/colors.h"
#include "utils/utils.h"

#include <igl/boundary_facets.h>
#include <igl/marching_tets.h>
#include <igl/unproject_onto_mesh.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <unordered_set>

namespace {

bool validate_endpoint_pairs(const std::vector<std::pair<int, int>>& endpoints,
                             const Eigen::VectorXi& components) {
    bool success = true;
    std::unordered_set<int> computed_components;

    for (int i = 0; i < endpoints.size(); i++) {
        const int c1 = components[endpoints[i].first];
        const int c2 = components[endpoints[i].second];

        if (c1 != c2) {
            success = false;
            break;
        }
        if (computed_components.find(c1) != computed_components.end()) {
            success = false;
            break;
        }
        else {
            computed_components.insert(c1);
        }
    }

    return success;
}


void compute_skeleton(const Eigen::MatrixXd& TV, const Eigen::MatrixXi& TT,
                      const Eigen::VectorXd normalized_distances,
                      const std::vector<std::pair<int, int>>& endpoint_pairs,
                      const Eigen::VectorXi& connected_components,
                      int num_skeleton_vertices,
                      Eigen::MatrixXd& skeleton_vertices) {
    std::vector<Eigen::MatrixXi> TT_comps;
    split_mesh_components(TT, connected_components, TT_comps);

    Eigen::MatrixXd LV;
    Eigen::MatrixXi LF;

    int vertex_count = 0;
    skeleton_vertices.resize(num_skeleton_vertices, 3);

    for (int ep_i = 0; ep_i < endpoint_pairs.size(); ep_i++) {
        const int component = connected_components[endpoint_pairs[ep_i].first];
        skeleton_vertices.row(vertex_count) = TV.row(endpoint_pairs[ep_i].first);
        vertex_count++;

        const double nd_ep0 = normalized_distances[endpoint_pairs[ep_i].first];
        const double nd_ep1 = normalized_distances[endpoint_pairs[ep_i].second];
        const double isoval_incr = (nd_ep1 - nd_ep0) / num_skeleton_vertices;

        double isovalue = normalized_distances[endpoint_pairs[ep_i].first] + isoval_incr;
        for (int i = 0; i < num_skeleton_vertices - 2; i++) {
            igl::marching_tets(TV, TT_comps[component], normalized_distances, isovalue, LV, LF);
            if (LV.rows() == 0) {
                isovalue += isoval_incr;
                continue;
            }
            Eigen::RowVector3d c = LV.colwise().sum() / LV.rows();
            skeleton_vertices.row(vertex_count) = c;
            vertex_count += 1;
            isovalue += isoval_incr;
        }

        skeleton_vertices.row(vertex_count) = TV.row(endpoint_pairs[ep_i].second);
        vertex_count += 1;
    }

    skeleton_vertices.conservativeResize(vertex_count, 3);
}


} // namespace

EndPoint_Selection_Menu::EndPoint_Selection_Menu(State& state) : state(state) {
  extracting_skeleton = false;
  done_extracting_skeleton = false;
}


void EndPoint_Selection_Menu::initialize() {
    for (size_t i = viewer->data_list.size() - 1; i > 0; i--) {
        viewer->erase_mesh(i);
    }
    viewer->data().clear();
    viewer->append_mesh();

    mesh_overlay_id = static_cast<int>(viewer->selected_data_index);
    viewer->selected_data_index = mesh_overlay_id;

    const Eigen::MatrixXd& TV = state.dilated_tet_mesh.TV;
    const Eigen::MatrixXi& TF = state.dilated_tet_mesh.TF;
    viewer->data().set_mesh(TV, TF);
    viewer->core.align_camera_center(TV, TF);

    viewer->append_mesh();
    points_overlay_id = static_cast<int>(viewer->selected_data_index);

    viewer->selected_data_index = mesh_overlay_id;

    old_viewport = viewer->core.viewport;

    int window_width, window_height;
    glfwGetWindowSize(viewer->window, &window_width, &window_height);
    viewer->core.viewport = Eigen::RowVector4f(view_hsplit*window_width, 0, (1.0-view_hsplit)*window_width, window_height);

    if (state.dirty_flags.endpoints_dirty) {
        state.skeleton_estimation_parameters.endpoint_pairs.clear();
        state.dirty_flags.endpoints_dirty = false;
        state.dirty_flags.bounding_cage_dirty = true;
        selecting_endpoints = true;
    } else {
        selecting_endpoints = false;
    }

    current_endpoint_idx = 0;
    current_endpoints = { -1, -1 };

    done_extracting_skeleton = false;
    extracting_skeleton = false;
    debug.drew_debug_state = false;
}

void EndPoint_Selection_Menu::deinitialize() {
    for (size_t i = viewer->data_list.size() - 1; i > 0; i--) {
        viewer->erase_mesh(i);
    }
    viewer->data().clear();
    viewer->core.viewport = old_viewport;
}

bool EndPoint_Selection_Menu::pre_draw() {
    int window_width, window_height;
    glfwGetWindowSize(viewer->window, &window_width, &window_height);
#ifdef __APPLE__
    viewer->core.viewport = Eigen::RowVector4f(2.0*view_hsplit*window_width, 0, 2.0*(1.0-view_hsplit)*window_width, 2.0*window_height);
#else
    viewer->core.viewport = Eigen::RowVector4f(view_hsplit*window_width, 0, (1.0-view_hsplit)*window_width, window_height);
#endif

    bool ret = FishUIViewerPlugin::pre_draw();
    const Eigen::MatrixXd& TV = state.dilated_tet_mesh.TV;

    int push_mesh_id = static_cast<int>(viewer->selected_data_index);
    viewer->selected_data_index = points_overlay_id;
    viewer->data().clear();

    if (selecting_endpoints) {
        for (unsigned int i = 0; i < current_endpoint_idx; i++) {
            const int vid = current_endpoints[i];
            viewer->data().add_points(TV.row(vid), i == 0 ? ColorRGB::GREEN : ColorRGB::RED);
        }
    } else {
        for (int i = 0; i < state.skeleton_estimation_parameters.endpoint_pairs.size(); i++) {
            std::pair<int, int> ep = state.skeleton_estimation_parameters.endpoint_pairs[i];
            viewer->data().add_points(TV.row(ep.first), ColorRGB::GREEN);
            viewer->data().add_points(TV.row(ep.second), ColorRGB::RED);
        }
    }
    viewer->selected_data_index = push_mesh_id;

    return ret;
}

void EndPoint_Selection_Menu::debug_draw_intermediate_state() {
    if (debug.drew_debug_state) {
        return;
    }

    viewer->data().clear();
    const Eigen::MatrixXd& TV = state.dilated_tet_mesh.TV;
    const Eigen::MatrixXi& TF = state.dilated_tet_mesh.TF;
    Eigen::MatrixXd V1, V2;
    edge_endpoints(TV, TF, V1, V2);
    viewer->data().add_edges(V1, V2, ColorRGB::SILVER);

    size_t skel_mesh = viewer->append_mesh() - 1;
    viewer->selected_data_index = skel_mesh;
    {
        const Eigen::MatrixXd& skV = state.cage.skeleton_vertices();
        V1.resize(skV.rows()-1, 3);
        V2.resize(skV.rows()-1, 3);
        for (int i = 0; i < skV.rows()-1; i++) {
            V1.row(i) = skV.row(i);
            V2.row(i) = skV.row(i+1);
        }
        viewer->data().point_size = 5.0;
        viewer->data().line_width = 2.0;
        viewer->data().add_edges(V1, V2, ColorRGB::GREEN);
        viewer->data().add_points(skV, ColorRGB::GREEN);
    }

    size_t skel_mesh2 = viewer->append_mesh() - 1;
    viewer->selected_data_index = skel_mesh2;
    {
        const Eigen::MatrixXd& skV = state.cage.smooth_skeleton_vertices();
        V1.resize(skV.rows()-1, 3);
        V2.resize(skV.rows()-1, 3);
        for (int i = 0; i < skV.rows()-1; i++) {
            V1.row(i) = skV.row(i);
            V2.row(i) = skV.row(i+1);
        }
        viewer->data().point_size = 5.0;
        viewer->data().line_width = 2.0;
        viewer->data().add_edges(V1, V2, ColorRGB::DARK_MAGENTA);
        viewer->data().add_points(skV, ColorRGB::DARK_MAGENTA);
    }

    size_t kf_mesh = viewer->append_mesh() - 1;
    viewer->selected_data_index = kf_mesh;
    {
        Eigen::MatrixXd kf_centers(state.cage.num_keyframes(), 3);
        Eigen::MatrixXd kf_centroids(state.cage.num_keyframes(), 3);
        int count = 0;
        for (BoundingCage::KeyFrame& kf : state.cage.keyframes) {
            kf_centers.row(count) = kf.origin();
            kf_centroids.row(count) = kf.centroid_3d();

            Eigen::MatrixXd p1(2, 3), p2(2, 3);
            Eigen::MatrixXd c(2, 3);
            c.row(0) = ColorRGB::RED;
            c.row(0) = ColorRGB::GREEN;
            p1.row(0) = kf.origin(); p2.row(0) = kf.origin() + 10.0*kf.up_rotated_3d();
            p1.row(1) = kf.origin(); p2.row(1) = kf.origin() + 10.0*kf.right_rotated_3d();

            viewer->data().add_edges(p1, p2, c);
            Eigen::MatrixXd bboxv = kf.bounding_box_vertices_3d();
            viewer->data().add_points(bboxv, ColorRGB::CYAN);
            count += 1;
        }

        viewer->data().point_size = 10.0;
        viewer->data().line_width = 2.0;
        viewer->data().add_points(kf_centroids, ColorRGB::MAGENTA);
        viewer->data().add_points(kf_centers, ColorRGB::STEEL_BLUE);
    }

    size_t cell_mesh = viewer->append_mesh() - 1;
    viewer->selected_data_index = cell_mesh;
    viewer->data().set_mesh(state.cage.mesh_vertices(), state.cage.mesh_faces());
    viewer->data().show_faces = false;
    viewer->data().line_color = Eigen::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    viewer->data().line_width = 2.0;

    size_t pts_mesh = viewer->append_mesh() - 1;
    viewer->selected_data_index = pts_mesh;
    {
        Eigen::MatrixXd C;
        double min_z = state.dilated_tet_mesh.geodesic_dists.minCoeff();
        double max_z = state.dilated_tet_mesh.geodesic_dists.maxCoeff();
        igl::colormap(igl::COLOR_MAP_TYPE_PARULA, state.dilated_tet_mesh.geodesic_dists, min_z, max_z, C);

        viewer->data().add_points(TV, C);
        viewer->data().point_size = 4.0;
    }

    debug.drew_debug_state = true;
}


bool EndPoint_Selection_Menu::post_draw() {
    bool ret = FishUIViewerPlugin::post_draw();
    int window_width, window_height;
    glfwGetWindowSize(viewer->window, &window_width, &window_height);
    viewer->core.viewport = Eigen::RowVector4f(view_hsplit*window_width, 0, (1.0-view_hsplit)*window_width, window_height);

    int width;
    int height;
    glfwGetWindowSize(viewer->window, &width, &height);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    ImGui::SetNextWindowSize(ImVec2(w * view_hsplit, h), ImGuiSetCond_Always);
    ImGui::Begin("Select Endpoints", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

    if (done_extracting_skeleton) {
        if (debug.enabled) {
            debug_draw_intermediate_state();
            if (ImGui::Button("COOL?")) {
                state.set_application_state(Application_State::BoundingPolygon);
            }
        } else {
            state.set_application_state(Application_State::BoundingPolygon);
        }
    }

    if (extracting_skeleton) {
        ImGui::OpenPopup("Extracting Skeleton");
        ImGui::BeginPopupModal("Extracting Skeleton");
        ImGui::Text("Extracting Fish Skeleton. Please wait, this may take a few seconds.");
        ImGui::NewLine();
        ImGui::EndPopup();
    }

    if (bad_selection) {
        ImGui::OpenPopup("Invalid Endpoint Selection");
        ImGui::BeginPopupModal("Invalid Endpoint Selection");
        ImGui::Text("%s", bad_selection_error_message.c_str());
        ImGui::NewLine();
        ImGui::Separator();
        if (ImGui::Button("OK")) {
            bad_selection = false;
        }
        ImGui::EndPopup();
    }

    std::string button_text("Select Endpoints");
    if (selecting_endpoints) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

        if (current_endpoint_idx == 0) {
            button_text = std::string("Select Front");
        }
        else if (current_endpoint_idx == 1) {
            button_text = std::string("Select Back");
        }
        else {
            assert(false);
        }
    }

    if (ImGui::Button(button_text.c_str(), ImVec2(-1, 0))) {
        selecting_endpoints = true;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (selecting_endpoints) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();

        if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
            current_endpoint_idx = 0;
            current_endpoints = { -1, -1 };
            selecting_endpoints = false;
        }
    }
    ImGui::NewLine();
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Advanced", nullptr, ImGuiTreeNodeFlags(0))) {
        float cage_bbox_rad = (float)state.skeleton_estimation_parameters.cage_bbox_radius;
        int num_subdivs = state.skeleton_estimation_parameters.num_subdivisions;
        int smoothing_iters = state.skeleton_estimation_parameters.num_smoothing_iters;

        ImGui::Spacing();
        ImGui::Text("Skeleton Subdivisions:");
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##skelsubdivs", &num_subdivs, 1, 2)) {
           state.skeleton_estimation_parameters.num_subdivisions = std::max(num_subdivs, 2);
           state.dirty_flags.bounding_cage_dirty = true;
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Text("Skeleton Smoothing Iterations:");
        ImGui::PushItemWidth(-1);
        if (ImGui::InputInt("##smoothingiters", &smoothing_iters, 1, 2)) {
           state.skeleton_estimation_parameters.num_smoothing_iters = std::max(smoothing_iters, 0);
           state.dirty_flags.bounding_cage_dirty = true;
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Text("Cage Bounding-Box Radius:");
        ImGui::PushItemWidth(-1);
        if (ImGui::InputFloat("##bboxrad", &cage_bbox_rad, 0.5, 1.0)) {
            state.skeleton_estimation_parameters.cage_bbox_radius = std::max((double)cage_bbox_rad, 1.0);
            state.dirty_flags.bounding_cage_dirty = true;
        }
        ImGui::PopItemWidth();
    }

    ImGui::NewLine();
    ImGui::Separator();

    if (ImGui::Button("Back")) {
        state.set_application_state(Application_State::Segmentation);
    }
    ImGui::SameLine();
    if (state.skeleton_estimation_parameters.endpoint_pairs.empty()) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    if (ImGui::Button("Next")) {
        if (state.dirty_flags.bounding_cage_dirty) {
            extract_skeleton();
            state.dirty_flags.bounding_cage_dirty = false;
        } else {
            extracting_skeleton = false;
            done_extracting_skeleton = true;
        }
    }
    if (state.skeleton_estimation_parameters.endpoint_pairs.empty()) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
    ImGui::End();

    ImGui::Render();
    return ret;
}

bool EndPoint_Selection_Menu::key_down(int key, int modifier) {
    bool ret = FishUIViewerPlugin::key_down(key, modifier);
    if (!selecting_endpoints) {
        return ret;
    }
    if (key != 32) {
        return ret;
    }

    int fid;            // ID of the clicked face
    Eigen::Vector3f bc; // Barycentric coords of the click point on the face
    double x = viewer->current_mouse_x;
    double y = viewer->core.viewport(3) - viewer->current_mouse_y;

    if (igl::unproject_onto_mesh(Eigen::Vector2f(x, y),
            viewer->core.view * viewer->core.model,
            viewer->core.proj, viewer->core.viewport,
            state.dilated_tet_mesh.TV, state.dilated_tet_mesh.TF, fid, bc))
    {
        int max;
        bc.maxCoeff(&max);
        int vid = state.dilated_tet_mesh.TF(fid, max);
        current_endpoints[current_endpoint_idx] = vid;

        current_endpoint_idx += 1;

        if (current_endpoint_idx >= 2) { // We've selected 2 endpoints
            std::vector<std::pair<int, int>> old_endpoints = state.skeleton_estimation_parameters.endpoint_pairs;
            state.skeleton_estimation_parameters.endpoint_pairs.clear();
            state.skeleton_estimation_parameters.endpoint_pairs.push_back(std::make_pair(current_endpoints[0], current_endpoints[1]));

            if (current_endpoints[0] == current_endpoints[1]) {
                bad_selection = true;
                bad_selection_error_message = "Invalid Endpoints: Selected endpoints are the same.";
                state.skeleton_estimation_parameters.endpoint_pairs.pop_back();
            }
            else if (!validate_endpoint_pairs(state.skeleton_estimation_parameters.endpoint_pairs, state.dilated_tet_mesh.connected_components)) {
                bad_selection = true;
                bad_selection_error_message = "Invalid Endpoints: You can only have one endpoint pair per connected component.";
                state.skeleton_estimation_parameters.endpoint_pairs.pop_back();
            }

            current_endpoints = { -1, -1 };
            current_endpoint_idx = 0;
            selecting_endpoints = false;

            if (bad_selection) {
                state.skeleton_estimation_parameters.endpoint_pairs = old_endpoints;
            } else {
                state.dirty_flags.bounding_cage_dirty = true;
            }
        }
    }

    return ret;
}


void EndPoint_Selection_Menu::extract_skeleton() {
    auto thread_fun = [&]() {
        extracting_skeleton = true;
        glfwPostEmptyEvent();

        const Eigen::MatrixXd& TV = state.dilated_tet_mesh.TV;
        const Eigen::MatrixXi& TT = state.dilated_tet_mesh.TT;
        const Eigen::VectorXi& C = state.dilated_tet_mesh.connected_components;
        const int comp = C[state.skeleton_estimation_parameters.endpoint_pairs[0].first];

        Eigen::MatrixXd TV2;
        Eigen::MatrixXi TT2;
        Eigen::VectorXi C2;
        Eigen::VectorXi CMap;
        Eigen::VectorXd geodesic_dists2;
        std::vector<std::pair<int, int>> selected_endpoints_2;
        remesh_connected_components(comp, C, TV, TT, CMap, TV2, TT2);
        C2 = Eigen::VectorXi::Zero(TV2.rows());
        for (const std::pair<int, int>& p : state.skeleton_estimation_parameters.endpoint_pairs) {
            std::pair<int, int> p2 = std::make_pair(CMap[p.first], CMap[p.second]);
            selected_endpoints_2.push_back(p2);
        }
        Eigen::MatrixXd skeleton_vertices;

        const bool normalized = true;
        geodesic_distances(TV2, TT2, selected_endpoints_2, geodesic_dists2, normalized);
        compute_skeleton(TV2, TT2, geodesic_dists2,
            selected_endpoints_2, C2,
            state.skeleton_estimation_parameters.num_subdivisions, skeleton_vertices);

        state.dilated_tet_mesh.geodesic_dists.resize(TV.rows());
        for (int i = 0; i < TV.rows(); i++) {
            if (CMap[i] >= 0) {
                state.dilated_tet_mesh.geodesic_dists[i] = geodesic_dists2[CMap[i]];
            } else {
                state.dilated_tet_mesh.geodesic_dists[i] = -1.0;
            }
        }
        const double rad = state.skeleton_estimation_parameters.cage_bbox_radius;
        Eigen::Vector4d bbox(-rad, rad, -rad, rad);
        state.cage.set_skeleton_vertices(skeleton_vertices, state.skeleton_estimation_parameters.num_smoothing_iters, bbox);

        extracting_skeleton = false;
        done_extracting_skeleton = true;
        glfwPostEmptyEvent();
    };

    extract_skeleton_thread = std::thread(thread_fun);
    extract_skeleton_thread.detach();
}
