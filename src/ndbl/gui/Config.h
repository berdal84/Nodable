#pragma once

#include <string>
#include <vector>
#include <memory>

#include "tools/core/reflection/reflection"
#include "tools/core/types.h"
#include "tools/gui/AppView.h"
#include "tools/gui/Config.h"
#include "tools/gui/FontManager.h"
#include "tools/gui/ImGuiEx.h"
#include "tools/gui/Size.h"

#include "ndbl/core/ASTNodeType.h"

#include "types.h"
#include "ViewDetail.h"
#include "Isolation.h"
#include "ndbl/core/ASTNodeSlotFlag.h"

namespace ndbl
{
    using tools::Vec2;
    using tools::Vec4;
    using tools::Color;

    typedef int ConfigFlags;
    enum ConfigFlag_
    {
        ConfigFlag_NONE                              = 0,
        ConfigFlag_DRAW_DEBUG_LINES                  = 1 << 0,
        ConfigFlag_EXPERIMENTAL_HYBRID_HISTORY       = 1 << 2,
        ConfigFlag_EXPERIMENTAL_MULTI_SELECTION      = 1 << 3,
    };

    struct Config
    {
        Config(tools::Config*);
        void reset();

        TextEditor::Palette ui_text_textEditorPalette{};
        Vec2           ui_wire_bezier_roundness; // {min, max}
        float          ui_wire_bezier_thickness;
        Vec2           ui_wire_bezier_fade_lensqr_range;
        Vec4           ui_wire_color;
        Vec4           ui_wire_shadowColor;
        float          ui_slot_circle_radius_base;
        float          ui_slot_circle_radius(tools::Size = tools::Size_DEFAULT) const;
        Vec4           ui_slot_border_color;
        Vec4           ui_slot_color_light;
        Vec4           ui_slot_color_dark;
        Vec4&          ui_slot_color(SlotFlags slot_flags);
        Vec4           ui_slot_hovered_color;
        Vec2           ui_slot_rectangle_size;
        float          ui_slot_gap;
        float          ui_slot_border_radius;
        float          ui_slot_invisible_btn_expand_size;
        Vec2           ui_node_gap_base; // horizontal, vertical
        Vec2           ui_node_gap(tools::Size = tools::Size_DEFAULT) const;
        Vec4           ui_node_padding; // left, top, right, bottom
        float          ui_node_borderWidth;
        float          ui_node_instructionBorderRatio; // ratio to apply to borderWidth
        std::array<Vec4,ASTNodeType_COUNT> ui_node_fill_color;
        Vec4           ui_node_shadowColor;
        Vec4           ui_node_borderColor;
        Vec4           ui_node_borderHighlightedColor;
        Vec4           ui_node_highlightedColor;
        float          ui_node_speed;
        float          ui_node_physics_frequency;
        ViewDetail     ui_node_detail;
        Vec4           ui_codeflow_color;
        Vec4           ui_codeflow_shadowColor;
        float          ui_codeflow_thickness_ratio;
        float          ui_codeflow_thickness() const;
        Vec2           ui_toolButton_size;
        u64_t          ui_history_size_max{};
        float          ui_history_btn_spacing;
        float          ui_history_btn_height;
        float          ui_history_btn_width_max;
        const char*    ui_splashscreen_imagePath;
        float          ui_overlay_margin;
        float          ui_overlay_indent;
        Vec4           ui_overlay_window_bg_golor;
        Vec4           ui_overlay_border_color;
        Vec4           ui_overlay_text_color;
        Vec4           ui_graph_grid_color_major;
        Vec4           ui_graph_grid_color_minor;
        i32_t          ui_grid_subdiv_count;
        i32_t          ui_grid_size;
        const char*    ui_file_info_window_label;
        const char*    ui_help_window_label;
        const char*    ui_imgui_config_window_label;
        const char*    ui_node_properties_window_label;
        const char*    ui_config_window_label;
        const char*    ui_startup_window_label;
        const char*    ui_toolbar_window_label ;
        const char*    ui_interpreter_window_label;
        tools::Rect    ui_scope_content_rect_margin;
        float          ui_scope_child_margin;
        Vec4           ui_scope_fill_col_light;
        Vec4           ui_scope_fill_col_dark;
        Vec4           ui_scope_border_col;
        float          ui_scope_border_radius;
        float          ui_scope_border_thickness;
        float          ui_scope_gap_base;
        float          ui_scope_gap(tools::Size size = tools::Size_DEFAULT) const;
        Isolation      isolation;
        float          graph_view_unfold_duration; // The virtual duration used to simulate a graph view unfolding, like accelerating time.
        ConfigFlags    flags;
        tools::Config* tools_cfg;

        bool has_flags(ConfigFlags _flags)const { return (flags & _flags) == _flags; };
        void set_flags(ConfigFlags _flags) { flags |= _flags; }
        void clear_flags(ConfigFlags _flags) { flags &= ~_flags; }

    };

    [[nodiscard]]
    Config* init_config(); // create a new configuration and set it as current
    void    shutdown_config(Config*); // do the opposite of init
    Config* get_config(); // Get the current config, create_config() must be called first.
}
