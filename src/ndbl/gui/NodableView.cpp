#include "NodableView.h"

#include <utility>

#include "tools/core/log.h"
#include "tools/core/System.h"
#include "tools/gui/ActionManagerView.h"
#include "tools/gui/Config.h"
#include "tools/gui/Texture.h"
#include "tools/gui/TextureManager.h"
#include "tools/gui/AppView.h"

#include "ndbl/core/ASTUtils.h"
#include "ndbl/core/language/Nodlang.h"

#include "Config.h"
#include "Event.h"
#include "File.h"
#include "FileView.h"
#include "GraphView.h"
#include "History.h"
#include "Nodable.h"
#include "ASTNodeView.h"

using namespace ndbl;
using namespace tools;

template<typename T>
static FunctionDescriptor* create_variable_node_signature()
{
    static FunctionDescriptor* descriptor = FunctionDescriptor::create<T(T)>("variable");
    return descriptor;
}

void NodableView::update()
{
    float dt = m_base_view.delta_time();
    if( File* current_file = m_app->get_current_file() )
        current_file->view.update( dt );
}

void NodableView::init(Nodable * _app)
{
    TOOLS_LOG(tools::Verbosity_Diagnostic, "ndbl::NodableView", "init ...\n");
    m_app = _app;
    // Initialize wrapped view and inject some code ...
    tools::App* base_app = _app->get_base_app_handle();
    ASSERT(base_app != nullptr);
    m_base_view.init(base_app);

    m_base_view.signal_reset_layout.connect<&NodableView::_on_reset_layout>(this);
    m_base_view.signal_draw_splashscreen_content.connect<&NodableView::_on_draw_splashscreen_content>(this);

    // Load splashscreen image
    Config* cfg = get_config();
    tools::Path path = App::get_asset_path(cfg->ui_splashscreen_imagePath );
    m_logo = get_texture_manager()->load(path);

    // Add a bunch of new actions
    tools::ActionManager* action_manager = get_action_manager();
    ASSERT(action_manager != nullptr); // initialized by base_view
    // (With shortcut)
    action_manager->new_action<Event_DeleteSelection>("Delete", Shortcut{SDLK_DELETE, KMOD_NONE } );
    action_manager->new_action<Event_ArrangeSelection>("Arrange", Shortcut{SDLK_a, KMOD_NONE }, Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR );
    action_manager->new_action<Event_ToggleFolding>("Fold", Shortcut{SDLK_x, KMOD_NONE }, Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR );
    action_manager->new_action<Event_SelectNext>("Next", Shortcut{SDLK_n, KMOD_NONE } );
    action_manager->new_action<Event_FileSave>(ICON_FA_SAVE " Save", Shortcut{SDLK_s, KMOD_CTRL } );
    action_manager->new_action<Event_FileSaveAs>(ICON_FA_SAVE " Save as", Shortcut{SDLK_s, KMOD_CTRL } );
    action_manager->new_action<Event_FileClose>(ICON_FA_TIMES "  Close", Shortcut{SDLK_w, KMOD_CTRL } );
    action_manager->new_action<Event_FileBrowse>(ICON_FA_FOLDER_OPEN " Open", Shortcut{SDLK_o, KMOD_CTRL } );
    action_manager->new_action<Event_FileNew>(ICON_FA_FILE " New", Shortcut{SDLK_n, KMOD_CTRL } );
    action_manager->new_action<Event_ShowWindow>("Splashscreen", Shortcut{SDLK_F1 }, EventPayload_ShowWindow{"splashscreen" } );
    action_manager->new_action<Event_Exit>(ICON_FA_SIGN_OUT_ALT " Exit", Shortcut{SDLK_F4, KMOD_ALT } );
    action_manager->new_action<Event_Undo>("Undo", Shortcut{SDLK_z, KMOD_CTRL } );
    action_manager->new_action<Event_Redo>("Redo", Shortcut{SDLK_y, KMOD_CTRL } );
    action_manager->new_action<Event_ToggleIsolationFlags>("Isolation", Shortcut{SDLK_i, KMOD_CTRL }, Condition_ENABLE | Condition_HIGHLIGHTED_IN_TEXT_EDITOR );
    action_manager->new_action<Event_MoveSelection>("Drag whole graph", Shortcut{SDLK_SPACE, KMOD_NONE, "Space + Drag" }, Condition_ENABLE | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR );
    action_manager->new_action<Event_FrameSelection>("Frame Selection", Shortcut{SDLK_f, KMOD_NONE }, EventPayload_FrameNodeViews{FRAME_SELECTION_ONLY }, Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR );
    action_manager->new_action<Event_FrameSelection>("Frame All", Shortcut{SDLK_f, KMOD_LCTRL }, EventPayload_FrameNodeViews{FRAME_ALL } );
    // (to create block nodes)
    action_manager->new_action<Event_CreateNode>(ICON_FA_CODE " Condition", Shortcut{}, EventPayload_CreateNode{CreateNodeType_BLOCK_CONDITION } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_CODE " For Loop", Shortcut{}, EventPayload_CreateNode{CreateNodeType_BLOCK_FOR_LOOP } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_CODE " While Loop", Shortcut{}, EventPayload_CreateNode{CreateNodeType_BLOCK_WHILE_LOOP } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_CODE " Scope", Shortcut{}, EventPayload_CreateNode{CreateNodeType_BLOCK_SCOPE } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_CODE " Entry Point", Shortcut{}, EventPayload_CreateNode{CreateNodeType_ROOT } );
    // (to create variables)
    action_manager->new_action<Event_CreateNode>(ICON_FA_DATABASE " Boolean Variable", Shortcut{}, EventPayload_CreateNode{CreateNodeType_VARIABLE_BOOLEAN, create_variable_node_signature<bool>() } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_DATABASE " Double Variable", Shortcut{}, EventPayload_CreateNode{CreateNodeType_VARIABLE_DOUBLE, create_variable_node_signature<double>() } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_DATABASE " Integer Variable", Shortcut{}, EventPayload_CreateNode{CreateNodeType_VARIABLE_INTEGER, create_variable_node_signature<int>() } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_DATABASE " String Variable", Shortcut{}, EventPayload_CreateNode{CreateNodeType_VARIABLE_STRING, create_variable_node_signature<std::string>() } );
    //(to create literals)
    action_manager->new_action<Event_CreateNode>(ICON_FA_FILE " Boolean Literal", Shortcut{}, EventPayload_CreateNode{CreateNodeType_LITERAL_BOOLEAN, create_variable_node_signature<bool>() } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_FILE " Double Literal", Shortcut{}, EventPayload_CreateNode{CreateNodeType_LITERAL_DOUBLE, create_variable_node_signature<double>() } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_FILE " Integer Literal", Shortcut{}, EventPayload_CreateNode{CreateNodeType_LITERAL_INTEGER, create_variable_node_signature<int>() } );
    action_manager->new_action<Event_CreateNode>(ICON_FA_FILE " String Literal", Shortcut{}, EventPayload_CreateNode{CreateNodeType_LITERAL_STRING, create_variable_node_signature<std::string>() } );
    // (to create functions/operators from the API)
    // TODO: add a list of preset to create operators/functions
    // action_manager->new_action<Event_CreateNode>(label.c_str(), Shortcut{}, EventPayload_CreateNode{CreateNodeType_FUNCTION, invokable->get_sig() } );

    TOOLS_LOG(tools::Verbosity_Diagnostic, "ndbl::NodableView", "init_ex " TOOLS_OK "\n");
}

void NodableView::_on_draw_splashscreen_content()
{
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    // Image
    ImGui::SameLine((ImGui::GetContentRegionAvail().x - (float)m_logo->width) * 0.5f); // center img
    ImGuiEx::Image(m_logo);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {50.0f, 30.0f});

    // disclaimer
    ImGui::TextWrapped("DISCLAIMER: This software is a prototype, do not expect too much from it. Use at your own risk.");

    ImGui::NewLine();
    ImGui::NewLine();

    // credits
    const char *credit = "by Berdal84";
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(credit).x);
    ImGui::TextWrapped("%s", credit);

    // close on left/rightmouse btn click
    if (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))
    {
        m_base_view.show_splashscreen = false;
    }
    ImGui::PopStyleVar(); // ImGuiStyleVar_FramePadding
}

void NodableView::_on_reset_layout()
{
    auto* cfg = get_config();
    // Dock windows to specific dockspace
    m_base_view.dock_window( cfg->ui_help_window_label             , AppView::Dockspace_RIGHT);
    m_base_view.dock_window( cfg->ui_config_window_label           , AppView::Dockspace_RIGHT);
    m_base_view.dock_window( cfg->ui_file_info_window_label        , AppView::Dockspace_RIGHT);
    m_base_view.dock_window( cfg->ui_node_properties_window_label  , AppView::Dockspace_RIGHT);
    m_base_view.dock_window(cfg->ui_interpreter_window_label  , AppView::Dockspace_RIGHT);
    m_base_view.dock_window( cfg->ui_imgui_config_window_label     , AppView::Dockspace_RIGHT);
    m_base_view.dock_window( cfg->ui_toolbar_window_label          , AppView::Dockspace_TOP);
};

void NodableView::shutdown()
{
    assert(m_base_view.signal_reset_layout.disconnect<&NodableView::_on_reset_layout>(this));
    assert(m_base_view.signal_draw_splashscreen_content.disconnect<&NodableView::_on_draw_splashscreen_content>(this));

    // We could do this there, but the base view is responsible for shutdow the texture manager we used, so all textures will be released.
    // get_texture_manager()->release(m_logo);

    m_base_view.shutdown();
}

void NodableView::draw()
{
    VERIFY(m_logo != nullptr, "Logo is nullptr, did you call init_ex() ?");

    // note: we draw this view nested in base view's begin/end (similar to ImGui API).
    m_base_view.begin_draw();
    float dt = m_base_view.delta_time();

    EventManager*   event_manager   = get_event_manager();
    Config*         cfg             = get_config();
    tools::Config*  tools_cfg       = tools::get_config();
    bool            redock_all      = true;
    File*           current_file    = m_app->get_current_file();

    // 1. Draw Menu Bar
    if (ImGui::BeginMenuBar())
    {
        History* current_file_history = current_file ? &current_file->history : nullptr;
        auto has_selection = current_file != nullptr ? !current_file->graph()->component<GraphView>()->selection().empty()
                                                     : false;

        if (ImGui::BeginMenu("File"))
        {
            bool has_file = current_file != nullptr;
            bool is_current_file_content_dirty = current_file != nullptr && current_file->needs_to_be_saved();
            ImGuiEx::MenuItem_EventTrigger<Event_FileNew>();
            ImGuiEx::MenuItem_EventTrigger<Event_FileBrowse>();
            ImGui::Separator();
            ImGuiEx::MenuItem_EventTrigger<Event_FileSaveAs>(false, has_file);
            ImGuiEx::MenuItem_EventTrigger<Event_FileSave>(false, has_file && is_current_file_content_dirty);
            ImGui::Separator();
            ImGuiEx::MenuItem_EventTrigger<Event_FileClose>(false, has_file);

            auto auto_paste = has_file && current_file->view.experimental_clipboard_auto_paste();

            if (ImGui::MenuItem(ICON_FA_COPY        "  Auto-paste clipboard", "", auto_paste, has_file ) && has_file ) {
                current_file->view.experimental_clipboard_auto_paste(!auto_paste);
            }

            ImGuiEx::MenuItem_EventTrigger<Event_Exit>();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (current_file_history)
            {
                ImGuiEx::MenuItem_EventTrigger<Event_Undo>();
                ImGuiEx::MenuItem_EventTrigger<Event_Redo>();
                ImGui::Separator();
            }
            if (ImGui::MenuItem("Delete Node", "Del.", false, has_selection ))
                event_manager->dispatch( EventID_DELETE_NODE );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            //auto frame = ImGui::MenuItem("Frame All", "F");
            redock_all |= ImGui::MenuItem("Redock documents");

            ImGui::Separator();

            auto menu_item_node_view_detail = [current_file, cfg](ViewDetail _detail, const char *_label) {
                if (ImGui::MenuItem(_label, "", cfg->ui_node_detail == _detail))
                {
                    cfg->ui_node_detail = _detail;
                    if (current_file != nullptr)
                        current_file->graph()->component<GraphView>()->reset_all_properties();
                }
            };

            ImGui::Text("View Detail:");
            ImGui::Indent();
            menu_item_node_view_detail(ViewDetail::MINIMALIST, "Minimalist");
            menu_item_node_view_detail(ViewDetail::NORMAL,     "Normal");
            ImGui::Unindent();

            ImGui::Separator();
            m_show_properties_editor = ImGui::MenuItem(ICON_FA_COGS " Show Properties", "",
                                                       m_show_properties_editor);
            m_show_imgui_demo = ImGui::MenuItem("Show ImGui Demo", "", m_show_imgui_demo);

            ImGui::Separator();

            if (ImGui::MenuItem("Fullscreen", "", is_fullscreen()))
            {
                toggle_fullscreen();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Reset Layout", ""))
            {
                m_base_view.reset_layout();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Code"))
        {
            ImGuiEx::MenuItem_EventTrigger<Event_ToggleIsolationFlags>(cfg->isolation);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Graph"))
        {

            if (ImGui::MenuItem("Reset"))
                event_manager->dispatch( EventID_RESET_GRAPH );

            ImGuiEx::MenuItem_EventTrigger<Event_ArrangeSelection>(false, has_selection);
            ImGuiEx::MenuItem_EventTrigger<Event_ToggleFolding>(false, has_selection);

            if (ImGui::MenuItem("Expand/Collapse recursive", nullptr, false, has_selection))
            {
                event_manager->dispatch<Event_ToggleFolding>( { RECURSIVELY } );
            }

            ImGui::Separator();

            ImGuiEx::MenuItem_EventTrigger<Event_ToggleIsolationFlags>(cfg->isolation);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Developer"))
        {
            bool debug = cfg->flags & ConfigFlag_DRAW_DEBUG_LINES;
            if ( ImGui::MenuItem("Debug Mode", "", debug ) )
            {
                cfg->tools_cfg->runtime_debug = !debug;
                cfg->clear_flags( ConfigFlag_DRAW_DEBUG_LINES );
                cfg->set_flags( !debug * ConfigFlag_DRAW_DEBUG_LINES);
                ImGuiEx::set_debug( !debug );
            }

            if ( ImGui::MenuItem("Limit FPS", "", tools_cfg->fps_limit_on ) )
            {
                tools_cfg->fps_limit_on = !tools_cfg->fps_limit_on;
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Verbosity"))
            {
                auto menu_item_verbosity = [](Verbosity verbosity, const char* label)
                {
                    if (ImGui::MenuItem(label, "", get_log_verbosity() == verbosity))
                    {
                        set_log_verbosity(verbosity);
                    }
                };

                menu_item_verbosity(Verbosity_Diagnostic, "Verbose");
                menu_item_verbosity(Verbosity_Message, "Message");
                menu_item_verbosity(Verbosity_Warning, "Warning");
                menu_item_verbosity(Verbosity_Error,   "Error");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Experimental"))
            {
                auto checkbox_flag = [&](const char* label, ConfigFlag_ flag )
                {
                    bool enabled = cfg->has_flags(flag);
                    if ( ImGui::Checkbox(label, &enabled) )
                    {
                        if ( !enabled )
                            cfg->clear_flags(flag);
                        else
                            cfg->set_flags(flag);
                    }
                };
                checkbox_flag("Hybrid history"       , ConfigFlag_EXPERIMENTAL_HYBRID_HISTORY);
                checkbox_flag("Multi-Selection"      , ConfigFlag_EXPERIMENTAL_MULTI_SELECTION);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("An issue ?")) {
            if (ImGui::MenuItem("Report on Github.com")) {
                System::open_url_async("https://github.com/berdal84/nodable/issues");
            }

            if (ImGui::MenuItem("Report by email")) {
                System::open_url_async("mail:berenger@42borgata.com");
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Show Splash Screen", "F1"))
            {
                m_base_view.show_splashscreen = true;
            }

            if (ImGui::MenuItem("Browse source code"))
            {
                System::open_url_async("https://www.github.com/berdal84/nodable");
            }

            if (ImGui::MenuItem("Credits"))
            {
                System::open_url_async("https://github.com/berdal84/nodable#credits-");
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // 2. Draw windows
    // All draw_xxx_window() are ImGui windows docked to a dockspace (defined in signal_reset_layout() )

    if(!m_app->has_files())
    {
        bool show_startup_window = !m_base_view.show_splashscreen;
        if( show_startup_window )
        {
            draw_startup_window( m_base_view.get_dockspace(AppView::Dockspace_ROOT));
        }
    }
    else
    {
        draw_toolbar_window();

        auto ds_root = m_base_view.get_dockspace(AppView::Dockspace_ROOT);
        for ( File*each_file: m_app->get_files())
        {
            draw_file_window( dt, ds_root, redock_all, each_file);
        }

        draw_file_info_window();
        draw_config_window();
        draw_imgui_config_window();

        if ( draw_node_properties_window() )
            m_app->get_current_file()->set_text_dirty();
        draw_help_window();
    }

    // end the drawing
    m_base_view.end_draw();
}

void NodableView::draw_help_window() const
{
    Config* cfg = get_config();
    if (ImGui::Begin( cfg->ui_help_window_label))
    {
        FontManager* font_manager = get_font_manager();
        ImGui::PushFont(font_manager->get_font(FontSlot_Heading));
        ImGui::Text("Welcome to Nodable!");
        ImGui::PopFont();
        ImGui::NewLine();
        ImGui::TextWrapped(
                "Nodable is primary_child-able.\n"
                "\n"
                "Nodable allows you to edit a program using both text and graph paradigms."
                "More precisely, it means:"
        );
        ImGuiEx::BulletTextWrapped("any change on the text will affect the graph");
        ImGuiEx::BulletTextWrapped("any change (structure or values) on the graph will affect the text");
        ImGuiEx::BulletTextWrapped(
                "but keep in mind the state is the text, any change not affecting the text (such as child positions or orphan primary_child) will be lost.");
        ImGui::NewLine();
        ImGui::PushFont(font_manager->get_font(FontSlot_Heading));
        ImGui::Text("Quick start");
        ImGui::PopFont();
        ImGui::NewLine();
        ImGui::TextWrapped("Nodable UI is designed as following:\n");
        ImGuiEx::BulletTextWrapped("On the left side a (light) text editor allows to edit source code.\n");
        ImGuiEx::BulletTextWrapped(
                "At the center, there is the graph editor where you can create_new/delete/connect primary_child\n");
        ImGuiEx::BulletTextWrapped(
                "On the right side (this side) you will find many tabs to manage additional config such as primary_child, interpreter, or app properties\n");
        ImGuiEx::BulletTextWrapped("At the top, between the menu and the editors, there is a tool bar."
                                       " There, few buttons will serve to compile, run and debug your program.");
        ImGuiEx::BulletTextWrapped("And at the bottom, below the editors, there is a status bar."
                                       " This bar will display important messages, warning, and errors. You can expand it to get older messages.");
    }
    ImGui::End();
}

void NodableView::draw_imgui_config_window() const
{
    Config* cfg = get_config();
    tools::Config* tools_cfg = tools::get_config();
    if( !tools_cfg->runtime_debug )
    {
        return;
    }

    if (ImGui::Begin( cfg->ui_imgui_config_window_label))
    {
        ImGui::ShowStyleEditor();
    }
    ImGui::End();
}

void NodableView::draw_file_info_window() const
{
    File* current_file = m_app->get_current_file();
    if ( current_file == nullptr )
    {
        return;
    }

    Config* cfg = get_config();
    if (ImGui::Begin( cfg->ui_file_info_window_label))
    {
        current_file->view.draw_info_panel();
    }

    ImGui::End();
}

bool NodableView::draw_node_properties_window()
{
    bool changed = false;
    Config* cfg = get_config();
    if (ImGui::Begin( cfg->ui_node_properties_window_label))
    {
        if( File* current_file = m_app->get_current_file() )
        {
            const GraphView* graph_view = current_file->graph()->component<GraphView>(); // Graph can't be null
            switch ( graph_view->selection().count<ASTNodeView*>() )
            {
                case 0:
                    break;
                case 1:
                {
                    ImGui::Indent(10.0f);
                    auto* first_nodeview = graph_view->selection().first_of<ASTNodeView*>();
                    changed |= ASTNodeView::draw_as_properties_panel(first_nodeview, &m_show_advanced_node_properties);
                    break;
                }
                default:
                    ImGui::Indent(10.0f);
                    ImGui::Text("Multi-Selection");
            }
        }
    }
    ImGui::End();
    return changed;
}

void NodableView::draw_startup_window(ImGuiID dockspace_id)
{
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 1.f));
    Config* cfg = get_config();
    ImGui::Begin( cfg->ui_startup_window_label);
    {
        FontManager*  font_manager  = get_font_manager();
        EventManager* event_manager = get_event_manager();

        ImGui::PopStyleColor();

        ImVec2 center_area(500.0f, 250.0f);
        ImVec2 avail = ImGui::GetContentRegionAvail();

        ImGui::SetCursorPosX((avail.x - center_area.x) / 2);
        ImGui::SetCursorPosY((avail.y - center_area.y) / 2);

        ImGui::BeginChild("center_area", center_area);
        {
            ImGui::Indent(center_area.x * 0.05f);

            ImGui::PushFont(font_manager->get_font(FontSlot_ToolBtn));
            ImGui::NewLine();

            ImVec2 btn_size(center_area.x * 0.44f, 40.0f);
            if (ImGui::Button(ICON_FA_FILE" New File", btn_size))
                event_manager->dispatch( EventID_FILE_NEW );
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FOLDER_OPEN" Open ...", btn_size))
                event_manager->dispatch( EventID_FILE_BROWSE );

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            ImGui::Text("%s", "Open an example");

            struct Example {
                const char* label;
                const char* path;
            };

            const std::array<Example, 4> examples = {
                Example{ ICON_FA_BOOK" Single expressions    ", "examples/arithmetic.cpp" },
                Example{ ICON_FA_BOOK" Multi instructions    ", "examples/multi-instructions.cpp" },
                Example{ ICON_FA_BOOK" Conditional Structures", "examples/if-else.cpp" },
                Example{ ICON_FA_BOOK" For Loop              ", "examples/for-loop.cpp" }
            };

            const ImVec2 example_btn_size(btn_size.x, btn_size.y * 0.66f);
            const int columns = 2;

            ImGui::NewLine();
            for (size_t i = 0; i < examples.size(); ++i )
            {
                if (i % columns != 0) ImGui::SameLine();
                if (ImGui::Button(examples[i].label, example_btn_size))
                {
                    m_app->open_asset_file(examples[i].path);
                }
            }

            ImGui::PopFont();

            ImGui::NewLine();
            ImGui::Unindent();
        }
        ImGui::EndChild();
    }
    ImGui::End(); // Startup Window
}

void NodableView::draw_file_window( float dt, ImGuiID dockspace_id, bool redock_all, File*file)
{
    ImGui::SetNextWindowDockID(dockspace_id, redock_all ? ImGuiCond_Always : ImGuiCond_Appearing);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar
                                  | ImGuiWindowFlags_UnsavedDocument * file->needs_to_be_saved();

    auto child_bg = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
    child_bg.w = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, child_bg);

    bool open    = true;
    bool visible = ImGui::Begin(file->filename().c_str(), &open, window_flags);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(1);

    if ( visible )
    {
        // Set current file if window is focused
        if ( ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            if ( !m_app->is_current(file) )
                m_app->set_current_file(file);

        // Draw content
        file->view.draw( dt );
    }
    ImGui::End();

    if ( !open )
    {
        m_app->close_file(file);
    }
}

void NodableView::draw_config_window()
{
    auto* cfg = get_config();
    auto* tools_cfg = tools::get_config();

    if (ImGui::Begin( cfg->ui_config_window_label))
    {
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGui::Text("Nodable Settings");
        if ( ImGui::Button("Reset Settings") )
        {
            cfg->reset();
        }

        if (ImGui::CollapsingHeader("Sizes", flags ))
        {
            ImGui::SliderFloat("set_size factor SM", &cfg->tools_cfg->size_factor[Size_SM], 0.0f, 5.0f);
            ImGui::SliderFloat("set_size factor MD", &cfg->tools_cfg->size_factor[Size_MD], 0.0f, 5.0f);
            ImGui::SliderFloat("set_size factor LR", &cfg->tools_cfg->size_factor[Size_LG], 0.0f, 5.0f);
            ImGui::SliderFloat("set_size factor XL", &cfg->tools_cfg->size_factor[Size_XL], 0.0f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Nodes", flags ))
        {
            ImGui::Indent();
            if ( ImGui::CollapsingHeader("Colors", flags ))
            {
                ImGui::ColorEdit4("default"     , &cfg->ui_node_fill_color[ASTNodeType_DEFAULT].x );
                ImGui::ColorEdit4("entry point" , &cfg->ui_node_fill_color[ASTNodeType_SCOPE].x );
                ImGui::ColorEdit4("condition"   , &cfg->ui_node_fill_color[ASTNodeType_IF_ELSE].x );
                ImGui::ColorEdit4("for loop"    , &cfg->ui_node_fill_color[ASTNodeType_FOR_LOOP].x );
                ImGui::ColorEdit4("while loop"  , &cfg->ui_node_fill_color[ASTNodeType_WHILE_LOOP].x );
                ImGui::ColorEdit4("variable"    , &cfg->ui_node_fill_color[ASTNodeType_VARIABLE].x );
                ImGui::ColorEdit4("literal"     , &cfg->ui_node_fill_color[ASTNodeType_LITERAL].x );
                ImGui::ColorEdit4("function"    , &cfg->ui_node_fill_color[ASTNodeType_FUNCTION].x );
                ImGui::ColorEdit4("operator"    , &cfg->ui_node_fill_color[ASTNodeType_OPERATOR].x );
                ImGui::Separator();
                ImGui::ColorEdit4("highlighted"         , &cfg->ui_node_highlightedColor.x);
                ImGui::ColorEdit4("shadow"              , &cfg->ui_node_shadowColor.x);
                ImGui::ColorEdit4("border"              , &cfg->ui_slot_border_color.x);
                ImGui::ColorEdit4("border (highlighted)", &cfg->ui_node_borderHighlightedColor.x);
                ImGui::ColorEdit4("slot (in)"           , &cfg->ui_slot_color_light.x);
                ImGui::ColorEdit4("slot (out)"          , &cfg->ui_slot_color_dark.x);
                ImGui::ColorEdit4("slot (hovered)"      , &cfg->ui_slot_hovered_color.x);
            }

            if ( ImGui::CollapsingHeader("Slots", flags ))
            {
                ImGui::Text("Property Slots:");
                ImGui::SliderFloat("slot radius", &cfg->ui_slot_circle_radius_base, 5.0f, 10.0f);

                ImGui::Separator();

                ImGui::Text("Code Flow Slots:");
                ImGui::SliderFloat2("slot set_size##codeflow"   , &cfg->ui_slot_rectangle_size.x, 2.0f, 100.0f);
                ImGui::SliderFloat("slot padding##codeflow" , &cfg->ui_slot_gap, 0.0f, 100.0f);
                ImGui::SliderFloat("slot radius##codeflow"  , &cfg->ui_slot_border_radius, 0.0f, 40.0f);
            }

            if ( ImGui::CollapsingHeader("Misc.", flags ))
            {
                ImGui::SliderFloat2("gap state (x and y-axis)", &cfg->ui_node_gap_base.x, 0.0f, 400.0f);
                ImGui::SliderFloat("velocity" , &cfg->ui_node_speed, 1.0f, 10.0f);
                ImGui::SliderFloat4("padding" , &cfg->ui_node_padding.x, 0.0f, 20.0f);
                ImGui::SliderFloat("border width", &cfg->ui_node_borderWidth, 0.0f, 10.0f);
                ImGui::SliderFloat("border width ratio (instructions)", &cfg->ui_node_instructionBorderRatio, 0.0f, 10.0f);
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Wires / Code Flow", flags ))
        {
            ImGui::Text("Wires");
            ImGui::SliderFloat("thickness", &cfg->ui_wire_bezier_thickness, 0.5f, 10.0f);
            ImGui::SliderFloat2("roundness (min,max)", &cfg->ui_wire_bezier_roundness.x, 0.0f, 1.0f);
            ImGui::SliderFloat2("fade length (min,max in lensqr)", &cfg->ui_wire_bezier_fade_lensqr_range.x, 0.0f, 100000.0f);
            ImGui::ColorEdit4("color", &cfg->ui_wire_color.x);
            ImGui::ColorEdit4("shadow color", &cfg->ui_wire_shadowColor.x);

            ImGui::Separator();

            ImGui::Text("Code Flow");
            ImGui::ColorEdit4("color##codeflow", &cfg->ui_codeflow_color.x);
            ImGui::SliderFloat("thickness (ratio)##codeflow", &cfg->ui_codeflow_thickness_ratio, 0.1, 1.0);
        }

        if (ImGui::CollapsingHeader("Graph", flags ))
        {
            ImGui::InputFloat("view unfold duration (sec)", &cfg->graph_view_unfold_duration);
            ImGui::ColorEdit4("grid color (major)", &cfg->ui_graph_grid_color_major.x);
            ImGui::ColorEdit4("grid color (minor)", &cfg->ui_graph_grid_color_minor.x);
            ImGui::SliderInt("grid set_size", &cfg->ui_grid_size, 1, 500);
            ImGui::SliderInt("grid subdivisions", &cfg->ui_grid_subdiv_count, 1, 16);
        }

        if (ImGui::CollapsingHeader("Scope", flags ))
        {
            ImGui::SliderFloat4("margins", &cfg->ui_scope_content_rect_margin.min.x, 2, 25);
            ImGui::SliderFloat4("primary_child margin", &cfg->ui_scope_child_margin, 2, 25);
            ImGui::SliderFloat("border radius", &cfg->ui_scope_border_radius, 0, 20);
            ImGui::SliderFloat("border thickness", &cfg->ui_scope_border_thickness, 0, 4);
            ImGui::ColorEdit4("fill color (light)", &cfg->ui_scope_fill_col_light.x);
            ImGui::ColorEdit4("fill color (dark)", &cfg->ui_scope_fill_col_dark.x);
            ImGui::ColorEdit4("border color", &cfg->ui_scope_border_col.x);
        }

        if (ImGui::CollapsingHeader("Shortcuts", flags ))
        {
            ActionManager*  action_manager = get_action_manager();
            draw_action_manager_ui(action_manager);
        }

#if TOOLS_POOL_ENABLE
        if ( tools_cfg->runtime_debug && ImGui::CollapsingHeader("Pool"))
        {
            ImGui::Text("Pool stats:");
            auto pool = get_pool_manager()->get_pool();
            ImGui::Text(" - Node.................... %8zu", pool->get_all<Node>().size() );
            ImGui::Text(" - NodeView................ %8zu", pool->get_all<NodeView>().size() );
            ImGui::Text(" - Physics................. %8zu", pool->get_all<Physics>().size() );
            ImGui::Text(" - Scope................... %8zu", pool->get_all<Scope>().size() );
        }
#endif
    }
    ImGui::End();
}

void NodableView::draw_toolbar_window()
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {5.0f, 5.0f});

    auto* cfg = get_config();

    if ( ImGui::Begin( cfg->ui_toolbar_window_label, NULL, flags ) )
    {
        FontManager*  font_manager  = get_font_manager();
        EventManager* event_manager = get_event_manager();
        const Vec2&   button_size   = cfg->ui_toolButton_size;

        ImGui::PopStyleVar();
        ImGui::PushFont(font_manager->get_font(FontSlot_ToolBtn));
        ImGui::BeginGroup();

        // reset
        if (ImGui::Button(ICON_FA_UNDO " regen. graph", button_size)) {
            event_manager->dispatch( EventID_RESET_GRAPH );
        }
        ImGui::SameLine();

        // enter isolation mode
        bool isolation_on = cfg->isolation & Isolation_ON;
        if (ImGui::Button(isolation_on ? ICON_FA_CROP " isolation mode: ON " : ICON_FA_CROP " isolation mode: OFF", button_size))
        {
            event_manager->dispatch( EventID_TOGGLE_ISOLATION_FLAGS );
        }
        ImGui::SameLine();
        ImGui::EndGroup();

        ImGui::PopFont();
    }
    ImGui::End();
}

bool NodableView::is_fullscreen() const
{
    return m_base_view.is_fullscreen();
}

void NodableView::toggle_fullscreen()
{
    m_base_view.set_fullscreen( !is_fullscreen() );
}

bool NodableView::pick_file_path(Path& _out_path, tools::AppView::DialogType _type) const
{
    return m_base_view.pick_file_path(_out_path, _type);
}

void NodableView::show_splashscreen(bool b)
{
    m_base_view.show_splashscreen = b;
}

void NodableView::save_screenshot(tools::Path& _path) const
{
    m_base_view.save_screenshot(_path);
}
