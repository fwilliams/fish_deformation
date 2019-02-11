#ifndef __FISH_DEFORMATION_BOUNDING_POLYGON_STATE__
#define __FISH_DEFORMATION_BOUNDING_POLYGON_STATE__

#include "fish_ui_viewer_plugin.h"
#include "bounding_widget_2d.h"
#include "volume_exporter.h"

#include "bounding_widget_3d.h"
#include "transfer_function_edit_widget.h"

struct State;

class Bounding_Polygon_Menu : public FishUIViewerPlugin {
public:
    Bounding_Polygon_Menu(State& state);

    bool mouse_move(int mouse_x, int mouse_y) override;
    bool mouse_down(int button, int modifier) override;
    bool mouse_up(int button, int modifier) override;
    bool mouse_scroll(float delta_y) override;

    bool key_down(int key, int modifiers) override;
    bool key_up(int key, int modifiers) override;

    bool post_draw() override;

    void initialize();
    void deinitialize();

    // We set this when the bounding cage changes to regenerate the straightened texture
    bool cage_dirty = true;

    VolumeExporter exporter;




private:

    bool is_2d_widget_in_focus();

    void post_draw_transfer_function();

    float view_hsplit = 0.5; // Horizontal split for the two menus (normalized distance
    float view_vsplit = 0.2; // Vertical split for the bottom menu (normalized distance from the bottom)

    Bounding_Polygon_Widget widget_2d;
    Bounding_Widget_3d widget_3d;
    TransferFunctionEditWidget tf_widget;

    State& state;

    Eigen::Vector4f old_viewport;
    Eigen::Vector4f viewer_viewport;

    float current_cut_index = 0;
    float keyframe_nudge_amount = 0.1;
    bool draw_straight = false;
    bool show_display_options = false;
    bool mouse_in_popup = false;
};

#endif // __FISH_DEFORMATION_BOUNDING_POLYGON_STATE__
