#include "bounding_polygon_plugin.h"

#include <unordered_map>

#include <igl/edges.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <GLFW/glfw3.h>

#include <utils/colors.h>
#include <utils/utils.h>

#include <igl/copyleft/cgal/convex_hull.h>

static void make_plane(const Eigen::RowVector3d& normal, const Eigen::RowVector3d& up,
                       const Eigen::RowVector3d& ctr, double scale,
                       Eigen::MatrixXd& V, Eigen::MatrixXi& F) {

  Eigen::RowVector3d n = normal;
  n.normalize();
  Eigen::RowVector3d u = up;
  u.normalize();
  Eigen::RowVector3d r = n.cross(u);

  V.resize(4, 3);

  V.row(0) = ctr + scale*(0.5*r + 0.5*u);
  V.row(1) = ctr + scale*(-0.5*r + 0.5*u);
  V.row(2) = ctr + scale*(-0.5*r - 0.5*u);
  V.row(3) = ctr + scale*(0.5*r - 0.5*u);

  F.resize(2, 3);
  F.row(0) = Eigen::RowVector3i(0, 1, 3);
  F.row(1) = Eigen::RowVector3i(1, 2, 3);
}


Bounding_Polygon_Menu::Bounding_Polygon_Menu(State& state)
  : state(state), widget_2d(Bounding_Polygon_Widget(state))
{}


void Bounding_Polygon_Menu::initialize() {
  for (int i = viewer->data_list.size()-1; i > 0; i--) {
    viewer->erase_mesh(i);
  }
  mesh_overlay_id = 0;
  viewer->selected_data_index = mesh_overlay_id;

  const Eigen::MatrixXd& TV = state.extracted_volume.TV;
  const Eigen::MatrixXi& TF = state.extracted_volume.TF;
  viewer->data().set_mesh(TV, TF);
  viewer->core.align_camera_center(TV, TF);

  viewer->append_mesh();
  points_overlay_id = viewer->selected_data_index;

  viewer->selected_data_index = mesh_overlay_id;

  int push_mesh_id = viewer->selected_data_index;

  viewer->selected_data_index = mesh_overlay_id;
  viewer->data().clear();
  Eigen::MatrixXi E;
  igl::edges(state.extracted_volume.TF, E);
  viewer->data().set_edges(state.extracted_volume.TV, E, Eigen::RowVector3d(0.75, 0.75, 0.75));

  Eigen::MatrixXd P1(state.skeleton_vertices.rows()-1, 3), P2(state.skeleton_vertices.rows()-1, 3);
  for (int i = 0; i < state.skeleton_vertices.rows()-1; i++) {
    P1.row(i) = state.skeleton_vertices.row(i);
    P2.row(i) = state.skeleton_vertices.row(i+1);
  }
  viewer->data().add_edges(P1, P2, ColorRGB::LIGHT_GREEN);
  viewer->data().point_size = 10.0;
  viewer->data().add_points(state.skeleton_vertices, ColorRGB::GREEN);

  for (int i = 0; i < state.smooth_skeleton_vertices.rows()-1; i++) {
    P1.row(i) = state.smooth_skeleton_vertices.row(i);
    P2.row(i) = state.smooth_skeleton_vertices.row(i+1);
  }
  viewer->data().add_edges(P1, P2, ColorRGB::RED);
  viewer->data().point_size = 10.0;
  viewer->data().add_points(state.smooth_skeleton_vertices, ColorRGB::RED);

  viewer->selected_data_index = push_mesh_id;

  widget_2d.initialize(viewer);

  Eigen::MatrixXd PV1, PV2;
  Eigen::MatrixXi PF1, PF2;
  Eigen::RowVector3d n1 =
      state.smooth_skeleton_vertices.row(1) -
      state.smooth_skeleton_vertices.row(0);
  n1.normalize();
  Eigen::RowVector3d right1(1, 0, 0);
  Eigen::RowVector3d up1 = right1.cross(n1);
  up1.normalize();
  right1 = up1.cross(n1);

  Eigen::RowVector3d n2 =
      state.smooth_skeleton_vertices.row(state.smooth_skeleton_vertices.rows()-1) -
      state.smooth_skeleton_vertices.row(state.smooth_skeleton_vertices.rows()-2);
  n2.normalize();
  Eigen::RowVector3d right2(1, 0, 0);
  Eigen::RowVector3d up2 = right2.cross(n1);
  up2.normalize();
  right2 = up1.cross(n1);

  make_plane(n1, up1, state.smooth_skeleton_vertices.row(0), 40.0, PV1, PF1);
  make_plane(n2, up2, state.smooth_skeleton_vertices.row(state.smooth_skeleton_vertices.rows()-1), 40.0, PV2, PF2);

  CV.resize(2*PV1.rows(), 3);
  for (int i = 0; i < PV1.rows(); i++) {
    CV.row(i) = PV1.row(i);
  }
  for (int i = 0; i < PV2.rows(); i++) {
    CV.row(i+PV1.rows()) = PV2.row(i);
  }
  igl::copyleft::cgal::convex_hull(CV, CF);
  P1.resize(0, 0);
  P2.resize(0, 0);
  edge_endpoints(CV, CF, P1, P2);
  viewer->data().add_points(CV, ColorRGB::BLUE);
  viewer->data().add_edges(P1, P2, ColorRGB::BLUE);
}


bool Bounding_Polygon_Menu::mouse_move(int mouse_x, int mouse_y) {
  return widget_2d.mouse_move(mouse_x, mouse_y);
}


bool Bounding_Polygon_Menu::mouse_down(int button, int modifier) {
  return widget_2d.mouse_down(button, modifier);
}


bool Bounding_Polygon_Menu::mouse_up(int button, int modifier) {
  return widget_2d.mouse_up(button, modifier);
}


bool Bounding_Polygon_Menu::post_draw() {
  bool ret = FishUIViewerPlugin::post_draw();

  int width;
  int height;

  glfwGetWindowSize(viewer->window, &width, &height);

  ImGui::SetNextWindowBgAlpha(0.5);
  ImGui::SetNextWindowPos(ImVec2(.0f, int(0.8*height)), ImGuiSetCond_Always);
  ImGui::SetNextWindowSize(ImVec2(width, int(height*0.35)), ImGuiSetCond_Always);
  ImGui::Begin("Select Boundary", NULL,
               ImGuiWindowFlags_NoSavedSettings |
               ImGuiWindowFlags_AlwaysAutoResize |
               ImGuiWindowFlags_NoTitleBar);

  if (ImGui::Button("< Prev")) {
    current_vertex_id = std::max(current_vertex_id - 1, 0);
  }
  ImGui::SameLine();
  if (ImGui::SliderInt("#vertexid", &current_vertex_id, 0, state.skeleton_vertices.rows() - 2)) {}
  ImGui::SameLine();
  if (ImGui::Button("Next >")) {
    current_vertex_id = std::min(current_vertex_id + 1, int(state.skeleton_vertices.rows() - 2));
  }

  ImGui::Checkbox("Show slice view", &show_slice_view);
  if (show_slice_view) {
    widget_2d.post_draw(PV, current_vertex_id);
  }
  ImGui::End();
  ImGui::Render();

  return ret;
}


bool Bounding_Polygon_Menu::pre_draw() {
  bool ret = FishUIViewerPlugin::pre_draw();

  glDisable(GL_CULL_FACE);
  Eigen::RowVector3d n =
      state.smooth_skeleton_vertices.row(current_vertex_id+1) -
      state.smooth_skeleton_vertices.row(current_vertex_id);
  n.normalize();
  Eigen::RowVector3d right(1, 0, 0);
  Eigen::RowVector3d up = right.cross(n);
  up.normalize();
  right = up.cross(n);

  Eigen::MatrixXi PF;
  make_plane(n, up, state.smooth_skeleton_vertices.row(current_vertex_id), 40.0, PV, PF);

  int push_overlay_id = viewer->selected_data_index;
  viewer->selected_data_index = points_overlay_id;
  viewer->data().clear();
  viewer->data().set_mesh(PV, PF);
  viewer->data().add_points(PV, ColorRGB::CRIMSON);
  viewer->data().point_size = 10.0;

  viewer->selected_data_index = push_overlay_id;

  return ret;
}
