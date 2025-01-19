#pragma once

#include <string>
#include <map>
#include <array>

#include "tools/core/reflection/reflection"
#include "tools/core/types.h"
#include "tools/gui/AppView.h"

namespace ndbl
{
    // forward declarations
    class History;
    class File;
    class Nodable;

	/*
		This class contain the basic setup for and OpenGL/SDL basic window.
	*/
    class NodableView
	{
	public:

        typedef tools::AppView::DialogType DialogType;

        // Common

        void init(Nodable*);
        void update();
        void shutdown();
        void draw();
        void show_splashscreen(bool b);
        bool is_fullscreen() const;
        void toggle_fullscreen();
        bool pick_file_path(tools::Path& _out_path, DialogType) const;
        void save_screenshot(tools::Path&) const;
        tools::AppView* get_base_view_handle() { return &m_base_view; }
        void _on_draw_splashscreen_content();
        void _on_reset_layout();
    protected:

        // draw_xxx_window

        void draw_file_info_window() const;
        void draw_file_window(float dt, ImGuiID dockspace_id, bool redock_all, File*file);
        void draw_help_window() const;
        void draw_imgui_config_window() const;
        bool draw_node_properties_window();
        void draw_config_window();
        void draw_startup_window(ImGuiID dockspace_id);
        void draw_toolbar_window();

        tools::Texture*    m_logo                    = nullptr;
        bool               m_show_properties_editor  = false;
        bool               m_show_imgui_demo         = false;
        bool               m_show_advanced_node_properties = false;
        bool               m_scroll_to_curr_instr = true;
        Nodable*           m_app                  = nullptr;
        tools::AppView     m_base_view; // wrapped

    };
}