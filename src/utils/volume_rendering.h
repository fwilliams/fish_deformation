#ifndef __VOLUME_RENDERING_H__
#define __VOLUME_RENDERING_H__

#include <igl/opengl/load_shader.h>
#include <igl/opengl/create_shader_program.h>
#include <Eigen/Core>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace volumerendering {

struct Bounding_Box {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;

    GLuint entry_framebuffer = 0;
    GLuint entry_texture = 0;

    GLuint exit_framebuffer = 0;
    GLuint exit_texture = 0;

    GLuint program = 0;
    struct {
        GLint model_matrix = 0;
        GLint view_matrix = 0;
        GLint projection_matrix = 0;
    } uniform_location;
};

struct Transfer_Function {
    struct Node {
        float t;
        glm::vec4 rgba;
    };

    std::vector<Node> nodes;
    bool is_dirty = false;
    GLuint texture = 0;
};

struct Parameters {
    glm::ivec3 volume_dimensions = { 0, 0, 0 };
    glm::vec3 volume_dimensions_rcp = { 0.f, 0.f, 0.f };

    glm::vec3 normalized_volume_dimensions = { 0.f, 0.f, 0.f };

    GLfloat sampling_rate = 10.0;
};

struct Volume_Rendering {
    Bounding_Box bounding_box;
    Transfer_Function transfer_function;
    Parameters parameters;

    struct {
        GLuint program_object = 0;
        struct {
            GLint entry_texture = 0;
            GLint exit_texture = 0;
            GLint volume_texture = 0;
            GLint transfer_function = 0;

            GLint volume_dimensions = 0;
            GLint volume_dimensions_rcp = 0;
            GLint sampling_rate = 0;

            GLint light_position = 0;
            GLint light_color_ambient = 0;
            GLint light_color_diffuse = 0;
            GLint light_color_specular = 0;
            GLint light_exponent_specular = 0;
        } uniform_location;
    } program;

    GLuint picking_framebuffer = 0;
    GLuint picking_texture = 0;

    struct {
        GLuint program_object = 0;
        struct {
            GLint entry_texture = 0;
            GLint exit_texture = 0;
            GLint volume_texture = 0;
            GLint transfer_function = 0;

            GLint volume_dimensions = 0;
            GLint volume_dimensions_rcp = 0;
            GLint sampling_rate = 0;
        } uniform_location;
    } picking_program;
};


void initialize(Volume_Rendering& volume_rendering, const glm::ivec2& viewport_size,
    const char* fragment_shader = nullptr, const char* picking_shader = nullptr);

void upload_volume_data(GLuint volume_texture, const glm::ivec3& tex_size,
    double* texture_data, int size);

void update_transfer_function(Transfer_Function& transfer_function);

void render_bounding_box(const Volume_Rendering& volume_rendering, glm::mat4 model_matrix,
    glm::mat4 view_matrix, glm::mat4 proj_matrix);

void render_volume(const Volume_Rendering& volume_rendering, glm::vec3 light_position, GLuint volume_texture);

glm::vec3 pick_volume_location(const Volume_Rendering& volume_rendering,
    glm::ivec2 mouse_position, GLuint volume_texture);

} // namespace volumerendering

#endif
