#pragma once

#include <imgui/imgui.h>

#include <SDL/include/SDL.h>
#include <array>
#include <map>
#include <string>

#include <fw/gui/FontConf.h>
#include <fw/gui/FontSlot.h>
#include <fw/gui/Shortcut.h>
#include <fw/gui/View.h>
#include <fw/core/types.h>

namespace fw
{
    // forward declarations
    class App;
    class File;
    class History;
    struct Texture;
    class VirtualMachine;

	/*
		This class contain the basic setup for and OpenGL/SDL basic window.
	*/
	class AppView : public View
	{
	public:

        enum DialogType // Helps to configure the file browse dialog
        {
            DIALOG_SaveAs,   // Allows to set a new file or select an existing file
            DIALOG_Browse    // Only allows to pick a file
        };

        /*
         * Dockspace: enum to identify dockspaces
         * ----------------------------------------
         *                 TOP                    |
         * ----------------------------------------
         *                           |            |
         *                           |            |
         *            CENTER         |    RIGHT   |
         *                           |            |
         *                           |            |
         *                           |            |
         * ---------------------------------------|
         *                   BOTTOM               |
         * ----------------------------------------
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

        // AppView's configuration
        struct Conf {
            std::string           title                    = "Untitled";
            float                 min_frame_time           = 1.0f / 60.0f;            // limit to 60fps
            ImColor               background_color         = ImColor(0.f,0.f,0.f);
            const char*           splashscreen_title       = "##Splashscreen";
            bool                  show_splashscreen        = true;
            bool                  show_imgui_demo          = false;
            FontConf              icon_font                = {"FA-solid-900", "fonts/fa-solid-900.ttf"};
            float                 dockspace_bottom_size      = 48.f;
            float                 dockspace_top_size       = 48.f;
            float                 dockspace_right_ratio    = 0.3f;
            size_t                log_tooltip_max_count    = 25;
            std::array<
                fw::vec4,
                fw::Log::Verbosity_COUNT> log_color         {
                                                                vec4(0.5f, 0.0f, 0.0f, 1.0f), // red
                                                                vec4(0.5f, 0.0f, 0.5f, 1.0f), // violet
                                                                vec4(0.5f, 0.5f, 0.5f, 1.0f), // grey
                                                                vec4(0.0f, 0.5f, 0.0f, 1.0f)  // green
                                                            };
            std::vector<FontConf> fonts                    = {{
                                                                "default",                          // id
                                                                "fonts/JetBrainsMono-Medium.ttf",   // path
                                                                18.0f,                              // size in px.
                                                                true,                               // include icons?
                                                                18.0f                               // icons size in px.
                                                            }};
            std::array<
                const char*,
                FontSlot_COUNT> fonts_default               {
                                                                "default", // FontSlot_Paragraph
                                                                "default", // FontSlot_Heading
                                                                "default", // FontSlot_Code
                                                                "default"  // FontSlot_ToolBtn
                                                            };
        };

		AppView(App*, Conf);
		~AppView() override;
    private:
        friend App;
        bool               init();                          // called by fw::App automatically
        void               handle_events();                 //              ...
		bool               draw() override;                 //              ...
        void               shutdown();                      //              ...
    protected:
        virtual bool       on_draw(bool& redock_all) = 0;   // implement here your app ui using ImGui
        virtual bool       on_init() = 0;                   // initialize your view here (SDL and ImGui are ready)
        virtual bool       on_reset_layout() = 0;           // implement behavior when layout is reset
        virtual void       on_draw_splashscreen() = 0;      // implement here the splashscreen windows content
    public:
        ImFont*            get_font(FontSlot) const;
        ImFont*            get_font_by_id(const char*);
        ImFont*            load_font(const FontConf&);
        ImGuiID            get_dockspace(Dockspace)const;
        bool               is_fullscreen() const;
        bool               is_splashscreen_visible()const;
        bool               pick_file_path(std::string& _out_path, DialogType);   // pick a file and store its path in _out_path
        void               dock_window(const char* window_name, Dockspace)const; // Call this within on_reset_layout
        void               set_fullscreen(bool b);
        void               set_layout_initialized(bool b);
        void               set_splashscreen_visible(bool b);
    private:
        void               draw_splashcreen_window();
        void               draw_status_window() const;
    protected:
        App*               m_app;
        Conf               m_conf;
    private:
        SDL_GLContext      m_sdl_gl_context;
        SDL_Window*        m_sdl_window;
        bool               m_is_layout_initialized;
        std::array<ImFont*, FontSlot_COUNT>  m_fonts;        // Required fonts
        std::array<ImGuiID, Dockspace_COUNT> m_dockspaces;
        std::map<std::string, ImFont*>       m_loaded_fonts; // Available fonts

        REFLECT_DERIVED_CLASS(View)
    };
}