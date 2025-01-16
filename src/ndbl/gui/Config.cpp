#include "Config.h"
#include "tools/gui/Color.h"
#include "tools/gui/Config.h"

using namespace tools;

ndbl::Config* g_conf{nullptr};

ndbl::Config* ndbl::init_config()
{
    ASSERT(g_conf == nullptr);

    // Make sure to get a valid tools::Config
    tools::Config* tools_cfg = tools::get_config();
    if ( tools_cfg == nullptr )
    {
        tools_cfg = tools::init_config();
    }

    g_conf = new Config(tools_cfg);

    return g_conf;
}

void ndbl::shutdown_config(Config* config)
{
    ASSERT(g_conf == config); // singleton
    tools::shutdown_config(g_conf->tools_cfg);
    delete g_conf;
    g_conf = nullptr;
}

ndbl::Config* ndbl::get_config()
{
    return g_conf;
}

ndbl::Config::Config(tools::Config* _tools_cfg)
{
    VERIFY(_tools_cfg != nullptr, "tools config not initialised");
    tools_cfg = _tools_cfg;
    ui_splashscreen_imagePath       = "assets/images/nodable-logo-xs.png";
    ui_text_textEditorPalette       = {
            0xffffffff, // None
            0xffd69c56, // Keyword
            0xff00ff00, // Number
            0xff7070e0, // String
            0xff70a0e0, // Char literal
            0xffffffff, // Punctuation
            0xff409090, // Preprocessor
            0xffdddddd, // Identifier
            0xff9bc64d, // Known identifier
            0xffc040a0, // Preproc identifier
            0xff909090, // Comment (single line)
            0xff909090, // Comment (multi line)
            0x30000000, // Background
            0xffe0e0e0, // Cursor
            0x20ffffff, // Selection
            0x800020ff, // ErrorMarker
            0x40f08000, // Breakpoint
            0x88909090, // Line number
            0x40000000, // Current line fill
            0x40808080, // Current line fill (inactive)
            0x40a0a0a0, // Current line edge
    };

    // nodes
    ui_node_borderWidth                   = 1.0f;
    ui_node_instructionBorderRatio        = 2.0f;
    ui_node_padding                       = Vec4{ 3.0f, 5.0f, 10.0f, 5.0f };
    ui_slot_circle_radius_base            = 4.0f;

    ui_node_highlightedColor              = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ui_node_borderColor                   = Vec4(1.0f, 1.0f, 1.0f, 0.8f);
    ui_node_borderHighlightedColor        = Vec4(1.0f, 1.0f, 1.0f, 0.8f);
    ui_node_shadowColor                   = Vec4(0.0f, 0.0f, 0.0f, 0.2f);

    ui_node_fill_color = {};
    for (auto& col : ui_node_fill_color)
        col = Vec4(0.7f, 0.9f, 0.7f, 1.0f); // green;

    ui_node_fill_color[ASTNodeType_VARIABLE]     =  Color(171, 190, 255);
    ui_node_fill_color[ASTNodeType_VARIABLE_REF] =  ui_node_fill_color[ASTNodeType_VARIABLE];
    ui_node_fill_color[ASTNodeType_LITERAL]      =  Color(200, 200, 200);
    ui_node_fill_color[ASTNodeType_FUNCTION]     =  Color(255, 199, 115);
    ui_node_fill_color[ASTNodeType_OPERATOR]     =  ui_node_fill_color[ASTNodeType_FUNCTION];

    ui_slot_border_color                  = Vec4(0.2f, 0.2f, 0.2f, 1.0f);
    ui_slot_hovered_color                 = Color(200, 200, 200);
    ui_slot_color_light                   = Color(255, 255, 255);
    ui_slot_color_dark                    = Color(127, 127, 127);
    ui_node_gap_base                      = Vec2(40.0f, 40.f);
    ui_node_speed                         = 20.0f;
    ui_node_physics_frequency             = 120.f;
    ui_node_detail                        = ViewDetail::NORMAL;
    ui_slot_rectangle_size                = Vec2{10.f, 10.f};
    ui_slot_gap                           = 4.0f;
    ui_slot_border_radius                 = 0.1f;
    ui_slot_invisible_btn_expand_size     = 4.f; // +4px

    // wires
    ui_wire_bezier_roundness              = Vec2{0.25f, 2.0f};
    ui_wire_bezier_thickness              = 2.0f;
    ui_wire_bezier_fade_lensqr_range      = {300.0f*300.f, 1000.0f*1000.0f};
    ui_wire_color                         = Color(255, 255, 255);
    ui_wire_shadowColor                   = ui_node_shadowColor;

    // code flow
    ui_codeflow_color                     = Color(150, 170, 140); // slightly green
    ui_codeflow_shadowColor               = Color(0, 0, 0, 64);
    ui_codeflow_thickness_ratio           = 0.45f; // relative to ui_slot_rectangle_size.x

    // buttons
    ui_toolButton_size                    = Vec2(0.0f, 25.0f);

    // history
    ui_history_btn_spacing                = 1.f;
    ui_history_btn_height                 = 10.f;
    ui_history_btn_width_max              = 20.f;
    ui_history_size_max                   = 200;

    // overlay
    ui_overlay_margin                     = 10.0f;
    ui_overlay_indent                     = 5.0f;
    ui_overlay_window_bg_golor            = Vec4(0.9f,0.9f,0.9f,0.2f);
    ui_overlay_border_color               = Vec4(0,0,0,0);
    ui_overlay_text_color                 = Vec4(0,0,0,0.5f);

    // Window names
    ui_file_info_window_label             = "File";
    ui_help_window_label                  = "Help";
    ui_imgui_config_window_label          = "ImGui";
    ui_node_properties_window_label       = "Node";
    ui_config_window_label                = "Settings";
    ui_startup_window_label               = "Startup";
    ui_toolbar_window_label               = "Toolbar";
    ui_interpreter_window_label           = "VM";

    // Scopes
    ui_scope_content_rect_margin                       = {{10.f, 15.f}, {10.f, 15.f}};
    ui_scope_child_margin                 = ui_scope_content_rect_margin.min.x;
    ui_scope_border_radius                = 7.f;
    ui_scope_border_thickness             = 3.f;
    ui_scope_gap_base                     = 10.f;
    ui_scope_fill_col_light               = Color(100, 100, 100);
    ui_scope_fill_col_dark                = Color(70,70,70);
    ui_scope_border_col                   = Color(255,255,255,40);

    // Graph
    ui_graph_grid_color_major             = Color(0, 0, 0, 42);
    ui_graph_grid_color_minor             = Color(0, 0, 0, 17);
    ui_grid_subdiv_count                  = 4;
    ui_grid_size                          = 100.0f;

    // Misc.
    flags                                 = ConfigFlag_EXPERIMENTAL_HYBRID_HISTORY
                                          | ConfigFlag_EXPERIMENTAL_MULTI_SELECTION;
    isolation                             = Isolation_OFF;
    graph_view_unfold_duration            = 2.0f; // simulate 2sec

    // NodableView
    tools_cfg->dockspace_right_ratio       = 0.25f;
    tools_cfg->dockspace_top_size          = 36.f;
    tools_cfg->dockspace_bottom_size       = 110.f;

    const char *k_paragraph = "Paragraph";
    const char *k_heading   = "Heading 1";
    const char *k_code      = "Code";
    const char *k_tool      = "Tool Button";

    tools_cfg->font_manager.text = {
        // id          , font_path                                  , size , icons? , icons size
        { k_paragraph  , "assets/fonts/JetBrainsMono-Regular.ttf"   , 16.0f, true   , 16.0f      },
        { k_heading    , "assets/fonts/JetBrainsMono-Bold.ttf"      , 20.0f, true   , 20.0f      },
        { k_code       , "assets/fonts/JetBrainsMono-Regular.ttf"   , 16.0f, true   , 16.0f      },
        { k_tool       , "assets/fonts/JetBrainsMono-Medium.ttf"    , 16.0f, true   , 16.0f      }
    };

    tools_cfg->font_manager.defaults[FontSlot_Paragraph] = k_paragraph;
    tools_cfg->font_manager.defaults[FontSlot_Heading]   = k_heading;
    tools_cfg->font_manager.defaults[FontSlot_Code]      = k_code;
    tools_cfg->font_manager.defaults[FontSlot_ToolBtn]   = k_tool;
    tools_cfg->font_manager.subsamples                   = 1.0f;
    tools_cfg->font_manager.icon                         = {"Icons", "assets/fonts/fa-solid-900.ttf" };
    tools_cfg->app_default_title = NDBL_APP_NAME " " NDBL_BUILD_REF " - Built " __DATE__ " at " __TIME__;
}

void ndbl::Config::reset()
{
    *this = { tools_cfg };
}

float ndbl::Config::ui_codeflow_thickness() const
{
    return ui_slot_rectangle_size.x * ui_codeflow_thickness_ratio;
}

Vec2 ndbl::Config::ui_node_gap(Size size) const
{
    return ui_node_gap_base * tools_cfg->size_factor[size];
}

float ndbl::Config::ui_slot_circle_radius(tools::Size size) const
{
    return ui_slot_circle_radius_base * tools_cfg->size_factor[size];
}

Vec4& ndbl::Config::ui_slot_color(ndbl::SlotFlags slot_flags)
{
    if ( (slot_flags & SlotFlag_INPUT) == SlotFlag_INPUT )
        return ui_slot_color_light;

    return ui_slot_color_dark;
}

float ndbl::Config::ui_scope_gap(tools::Size size) const
{
    return ui_scope_gap_base * tools_cfg->size_factor[size];
}
