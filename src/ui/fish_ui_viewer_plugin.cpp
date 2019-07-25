#include "state.h"
#include "fish_ui_viewer_plugin.h"

#include <igl/project.h>
#include <imgui/imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <imgui_fonts_droid_sans.h>
#include <GLFW/glfw3.h>

void FishUIViewerPlugin::init(igl::opengl::glfw::Viewer* _viewer) {
    ViewerPlugin::init(_viewer);
    // Setup ImGui binding
    if (_viewer) {
        if (!context_) {
            context_ = ImGui::CreateContext();
        }
        ImGui_ImplGlfwGL3_Init(viewer->window, false);
        ImGui::GetIO().IniFilename = nullptr;
//        ImGui::StyleColorsDark();
        ImGui::StyleColorsLight();
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameRounding = 0.0f;
        reload_font();
    }
}

void FishUIViewerPlugin::reload_font(int font_size) {
    hidpi_scaling_ = hidpi_scaling();
    pixel_ratio_ = pixel_ratio();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryCompressedTTF(droid_sans_compressed_data,
        droid_sans_compressed_size, font_size * hidpi_scaling_);
    io.FontGlobalScale = 1.f / pixel_ratio_;
}

void FishUIViewerPlugin::shutdown() {
    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext(context_);
    context_ = nullptr;
}

bool FishUIViewerPlugin::pre_draw() {
    glfwPollEvents();

    // Check whether window dpi has changed
    float scaling = hidpi_scaling();
    if (std::abs(scaling - hidpi_scaling_) > 1e-5) {
        reload_font();
        ImGui_ImplGlfwGL3_InvalidateDeviceObjects();
    }

    ImGui_ImplGlfwGL3_NewFrame();
    return false;
}

bool FishUIViewerPlugin::post_draw() {
    return false;
}

void FishUIViewerPlugin::post_resize(int width, int height) {
    if (context_) {
        ImGui::GetIO().DisplaySize.x = float(width);
        ImGui::GetIO().DisplaySize.y = float(height);
    }
}


// Mouse IO
bool FishUIViewerPlugin::mouse_down(int button, int modifier) {
    ImGui_ImplGlfwGL3_MouseButtonCallback(viewer->window, button, GLFW_PRESS, modifier);
    return ImGui::GetIO().WantCaptureMouse;
}

bool FishUIViewerPlugin::mouse_up(int button, int modifier) {
    return ImGui::GetIO().WantCaptureMouse;
}

bool FishUIViewerPlugin::mouse_move(int mouse_x, int mouse_y) {
    return ImGui::GetIO().WantCaptureMouse;
}

bool FishUIViewerPlugin::mouse_scroll(float delta_y) {
    ImGui_ImplGlfwGL3_ScrollCallback(viewer->window, 0.f, delta_y);
    return ImGui::GetIO().WantCaptureMouse;
}


// Keyboard IO
bool FishUIViewerPlugin::key_pressed(unsigned int key, int modifiers) {
    ImGui_ImplGlfwGL3_CharCallback(nullptr, key);
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool FishUIViewerPlugin::key_down(int key, int modifiers) {
    ImGui_ImplGlfwGL3_KeyCallback(viewer->window, key, 0, GLFW_PRESS, modifiers);
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool FishUIViewerPlugin::key_up(int key, int modifiers) {
    ImGui_ImplGlfwGL3_KeyCallback(viewer->window, key, 0, GLFW_RELEASE, modifiers);
    return ImGui::GetIO().WantCaptureKeyboard;
}

void FishUIViewerPlugin::window_size(GLFWwindow* handle, int* width, int* height) {
    glfwGetWindowSize(handle, width, height);
#ifdef __APPLE__
    *width *= 2;
    *height *= 2;
#endif
}

float FishUIViewerPlugin::pixel_ratio() {
    // Computes pixel ratio for hidpi devices
    GLFWwindow* window = glfwGetCurrentContext();

    int buf_size[2];
    glfwGetFramebufferSize(window, &buf_size[0], &buf_size[1]);

    int win_size[2];
    window_size(window, &win_size[0], &win_size[1]);
    return static_cast<float>(buf_size[0]) / static_cast<float>(win_size[0]);
}

float FishUIViewerPlugin::hidpi_scaling() {
    // Computes scaling factor for hidpi devices
    GLFWwindow* window = glfwGetCurrentContext();

    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
#ifdef __APPLE__
    // Offsets high window dpi screen resolution scaling on macOS by decreasing 
    // the relative scale of the application's rendered elements and text. Due to
    // widespread inclusion of hidpi resolution in macOS devices, the size of the
    // the application's elements was roughly four times their relative size on 
    // Linux platforms with the double the width and height. To get these elements
    // to appear in normal resolution, the level of hidpi scaling had to be reduced
    // by a factor of two both on the horizonal and vertical scales.
    xscale /= 2, yscale /= 2;
#endif

    return 0.5f * (xscale + yscale);
}
