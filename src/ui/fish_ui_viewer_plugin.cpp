#include "fish_ui_viewer_plugin.h"
#include <igl/project.h>
#include <imgui/imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <imgui_fonts_droid_sans.h>
#include <GLFW/glfw3.h>


FishUIViewerPlugin::FishUIViewerPlugin()
{

}

IGL_INLINE void FishUIViewerPlugin::init(igl::opengl::glfw::Viewer *_viewer)
{
  ViewerPlugin::init(_viewer);
  // Setup ImGui binding
  if (_viewer)
  {
    if (context_ == nullptr)
    {
      context_ = ImGui::CreateContext();
    }
    ImGui_ImplGlfwGL3_Init(viewer->window, false);
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 0.0f;
    reload_font();
  }
}

IGL_INLINE void FishUIViewerPlugin::reload_font(int font_size)
{
  hidpi_scaling_ = hidpi_scaling();
  pixel_ratio_ = pixel_ratio();
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->Clear();
  io.Fonts->AddFontFromMemoryCompressedTTF(droid_sans_compressed_data,
    droid_sans_compressed_size, font_size * hidpi_scaling_);
  io.FontGlobalScale = 1.0 / pixel_ratio_;
}

IGL_INLINE void FishUIViewerPlugin::shutdown()
{
  // Cleanup
  ImGui_ImplGlfwGL3_Shutdown();
  ImGui::DestroyContext(context_);
  context_ = nullptr;
}

IGL_INLINE bool FishUIViewerPlugin::pre_draw()
{
  glfwPollEvents();

  // Check whether window dpi has changed
  float scaling = hidpi_scaling();
  if (std::abs(scaling - hidpi_scaling_) > 1e-5)
  {
    reload_font();
    ImGui_ImplGlfwGL3_InvalidateDeviceObjects();
  }

  ImGui_ImplGlfwGL3_NewFrame();
  return false;
}

IGL_INLINE bool FishUIViewerPlugin::post_draw()
{
  return false;
}

IGL_INLINE void FishUIViewerPlugin::post_resize(int width, int height)
{
  if (context_)
  {
    ImGui::GetIO().DisplaySize.x = float(width);
    ImGui::GetIO().DisplaySize.y = float(height);
  }
}


// Mouse IO
IGL_INLINE bool FishUIViewerPlugin::mouse_down(int button, int modifier)
{
  ImGui_ImplGlfwGL3_MouseButtonCallback(viewer->window, button, GLFW_PRESS, modifier);
  return ImGui::GetIO().WantCaptureMouse;
}

IGL_INLINE bool FishUIViewerPlugin::mouse_up(int button, int modifier)
{
  return ImGui::GetIO().WantCaptureMouse;
}

IGL_INLINE bool FishUIViewerPlugin::mouse_move(int mouse_x, int mouse_y)
{
  return ImGui::GetIO().WantCaptureMouse;
}

IGL_INLINE bool FishUIViewerPlugin::mouse_scroll(float delta_y)
{
  ImGui_ImplGlfwGL3_ScrollCallback(viewer->window, 0.f, delta_y);
  return ImGui::GetIO().WantCaptureMouse;
}


// Keyboard IO
IGL_INLINE bool FishUIViewerPlugin::key_pressed(unsigned int key, int modifiers)
{
  ImGui_ImplGlfwGL3_CharCallback(nullptr, key);
  return ImGui::GetIO().WantCaptureKeyboard;
}

IGL_INLINE bool FishUIViewerPlugin::key_down(int key, int modifiers)
{
  ImGui_ImplGlfwGL3_KeyCallback(viewer->window, key, 0, GLFW_PRESS, modifiers);
  return ImGui::GetIO().WantCaptureKeyboard;
}

IGL_INLINE bool FishUIViewerPlugin::key_up(int key, int modifiers)
{
  ImGui_ImplGlfwGL3_KeyCallback(viewer->window, key, 0, GLFW_RELEASE, modifiers);
  return ImGui::GetIO().WantCaptureKeyboard;
}


IGL_INLINE float FishUIViewerPlugin::pixel_ratio()
{
  // Computes pixel ratio for hidpi devices
  int buf_size[2];
  int win_size[2];
  GLFWwindow* window = glfwGetCurrentContext();
  glfwGetFramebufferSize(window, &buf_size[0], &buf_size[1]);
  glfwGetWindowSize(window, &win_size[0], &win_size[1]);
  return (float) buf_size[0] / (float) win_size[0];
}

IGL_INLINE float FishUIViewerPlugin::hidpi_scaling()
{
  // Computes scaling factor for hidpi devices
  float xscale, yscale;
  GLFWwindow* window = glfwGetCurrentContext();
  glfwGetWindowContentScale(window, &xscale, &yscale);
  return 0.5 * (xscale + yscale);
}
