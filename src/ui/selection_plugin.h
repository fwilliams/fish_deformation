#include "fish_ui_viewer_plugin.h"
#include "state.h"

#ifndef __FISH_DEFORMATION_SELECTION_MENU__
#define __FISH_DEFORMATION_SELECTION_MENU__


class Selection_Menu : public FishUIViewerPlugin {
public:
    Selection_Menu(State& state);

    void initialize();
    void draw_setup();
    void draw();

    void key_down(unsigned int key, int modifiers);
    bool post_draw() override;

private:
    Eigen::VectorXd export_selected_volume(const std::vector<uint32_t>& feature_list);
    void resize_framebuffer_textures(igl::opengl::ViewerCore& core);

    State& _state;

    float clicked_mouse_position[2] = { 0.f, 0.f };
    bool is_currently_interacting = false;
    int current_interaction_index = -1;
    bool has_added_node_since_initial_click = false;

    int number_features = 10;
    bool number_features_is_dirty = true;

    bool color_by_id = true;

    // Keep in sync with volume_fragment_shader.h and Combobox code generation
    enum class Emphasis {
        None = 0,
        OnSelection = 1,
        OnNonSelection = 2
    };
    Emphasis emphasize_by_selection = Emphasis::OnSelection;

    float highlight_factor = 0.05f;
};


#endif // __FISH_DEFORMATION_SELECTION_MENU__
