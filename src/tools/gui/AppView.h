#pragma once

#include <SDL.h>
#include <array>
#include "tools/core/FileSystem.h"
#include <map>
#include <string>
#include "tools/core/types.h"
#include "ImGuiEx.h"
#include "Config.h"
#include "tools/core/Signals.h"

namespace tools
{
    // forward declarations
    class App;
    class History;
    class TextureManager;
    class EventManager;
    class FontManager;
    class VirtualMachine;

	/*
		This class contain the basic setup for and OpenGL/SDL basic window.
	*/
	class AppView
	{
	public:
        friend App;

        enum DialogType // Helps to configure the file browse dialog
        {
            DIALOG_SaveAs,   // Allows to set a new file or select an existing file
            DIALOG_Browse    // Only allows to pick a file
        };

        /*
         * Enum to identify dockspaces
         *
         -------------------------------------
         |                TOP                |
         |-----------------------------------|
         |           CENTER         |  RIGHT |
         |-----------------------------------|
         |              BOTTOM               |
         ------------------------------------
         */
        enum Dockspace
        {
            Dockspace_ROOT,
            Dockspace_CENTER,
            Dockspace_RIGHT,
            Dockspace_BOTTOM,
            Dockspace_TOP,
            Dockspace_COUNT,
        };

        tools::SimpleSignal signal_reset_layout; // add custom code during layout reset
        tools::SimpleSignal signal_draw_splashscreen_content; // to insert custom code into the splashscreen window

        bool        show_splashscreen = true; // flag to show/hide splashscreen
        void        init(App*);
        void        update();
        void        begin_draw();
        void        end_draw();
        void        shutdown();
        void        draw_splashscreen(); // If needed, use begin/end_splashscreen static methods to override this. Ex: if ( AppView::begin_splashscreen(m_app->config) ) { /* your code here */; AppView::end_splashscreen(); }
        ImGuiID     get_dockspace(Dockspace)const;
        bool        pick_file_path(tools::Path& _out_path, DialogType) const; // pick a file and store its path in _out_path
        static int  fps();      // get the current frame per second (un-smoothed)
        void        save_screenshot(tools::Path) const; // Save an LCT_RGBA PNG image to path
        bool        is_fullscreen() const;
        void        set_fullscreen( bool b );
        void        set_title( const char* string );
        void        dock_window(const char* window_name, Dockspace)const; // Must be called within signal_reset_layout
        void        reset_layout();
        std::vector<unsigned char> take_screenshot() const;
        float       delta_time() const { return 1.f / m_last_frame_fps; }
    private:
        std::string         m_title;
        TextureManager*     m_texture_manager  = nullptr;
        FontManager*        m_font_manager     = nullptr;
        EventManager*       m_event_manager    = nullptr;
        ActionManager*      m_action_manager   = nullptr;
        SDL_GLContext       m_sdl_gl_context   = nullptr;
        SDL_Window*         m_sdl_window       = nullptr;
        u32_t               m_last_frame_ticks = 0;
        u32_t               m_last_frame_dt    = 1000 / 30;
        float               m_last_frame_fps   = 30.f;
        App*                m_app              = nullptr;
        bool                m_is_layout_initialized = false;
        std::array<ImGuiID, Dockspace_COUNT>
                            m_dockspaces{};

    };
}