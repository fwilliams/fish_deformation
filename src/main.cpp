#include <string>

#include <spdlog/sinks/stdout_color_sinks.h>

#include "ui/initial_file_selection_state.h"
#include "ui/selection_plugin.h"
#include "ui/meshing_plugin.h"
#include "ui/endpoint_selection_plugin.h"
#include "ui/bounding_polygon_plugin.h"
#include "ui/state.h"
#include "Logger.hpp"

State _state;

Application_State previous_state;
Initial_File_Selection_Menu initial_file_selection(_state);
Selection_Menu selection_menu(_state);
Meshing_Menu meshing_menu(_state);
EndPoint_Selection_Menu endpoint_selection_menu(_state);
Bounding_Polygon_Menu bounding_polygon_menu(_state);


void log_opengl_debug(GLenum source, GLenum type, GLuint id, GLenum severity,
                      GLsizei length, const GLchar* message, const void* userParam)
{
    if (id == 131185 || id == 7 || id == 131218) {
        return;
    }
    if (source == GL_DEBUG_SOURCE_APPLICATION) {
        return;
    }
    _state.logger->error(
        "OpenGL Debug msg: Source: {}, Type: {}, Id: {}, Severity: {}, Message: {}",
        source, type, id, severity, std::string(message)
    );
#ifdef WIN32
    DebugBreak();
#endif
}


bool init(igl::opengl::glfw::Viewer& viewer) {
    initial_file_selection.init(&viewer);
    selection_menu.init(&viewer);
    meshing_menu.init(&viewer);
    endpoint_selection_menu.init(&viewer);
    bounding_polygon_menu.init(&viewer);

    viewer.plugins.push_back(&initial_file_selection);

    _state.logger = spdlog::stdout_color_mt(FISH_LOGGER_NAME);
    _state.logger->set_level(FISH_LOGGER_LEVEL);
    _state.cage.set_logger(_state.logger);

    std::shared_ptr<spdlog::logger> ct_logger = spdlog::stdout_color_mt(CONTOURTREE_LOGGER_NAME);
    ct_logger->set_level(CONTOURTREE_LOGGER_LEVEL);
    contourtree::Logger::setLogger(ct_logger);

#ifdef WIN32
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(log_opengl_debug, NULL);
#endif

    return false;
}

bool pre_draw(igl::opengl::glfw::Viewer& viewer) {
    if (previous_state != _state.application_state) {

        switch (previous_state) {
        case Application_State::Initial_File_Selection:
            initial_file_selection.deinitialize();
            break;
        case Application_State::BoundingPolygon:
            bounding_polygon_menu.deinitialize();
            break;
        case Application_State::Segmentation:
            selection_menu.deinitialize();
            break;
        case Application_State::EndPointSelection:
            endpoint_selection_menu.deinitialize();
            break;
        }

        viewer.plugins.clear();

        switch (_state.application_state) {
            case Application_State::Initial_File_Selection:
                initial_file_selection.initialize();
                viewer.plugins.push_back(&initial_file_selection);
                break;
            case Application_State::Segmentation:
                selection_menu.initialize();
                viewer.plugins.push_back(&selection_menu);
                break;
            case Application_State::Meshing:
                meshing_menu.initialize();
                viewer.plugins.push_back(&meshing_menu);
                break;
            case Application_State::EndPointSelection:
                endpoint_selection_menu.initialize();
                viewer.plugins.push_back(&endpoint_selection_menu);
                break;
            case Application_State::BoundingPolygon:
                bounding_polygon_menu.initialize();
                viewer.plugins.push_back(&bounding_polygon_menu);
                break;
        }

        previous_state = _state.application_state;

        glfwPostEmptyEvent();
        return true;
    }

    return false;
}

int main(int argc, char** argv) {
    previous_state = Application_State::NoState;
    igl::opengl::glfw::Viewer viewer;
    // viewer.core.background_color = Eigen::Vector4f(0.1f, 0.1f, 0.1f, 1.f);
    viewer.core.background_color = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
    // viewer.core.background_color = Eigen::Vector4f(0.8f, 0.8f, 0.8f, 1.f);
    viewer.core.is_animating = true;
    viewer.callback_init = init;
    viewer.callback_pre_draw = pre_draw;
    viewer.launch();

    return EXIT_SUCCESS;
}
