#include "Nodable.h"

#include <algorithm>

#include "fw/core/assertions.h"
#include "fw/core/system.h"
#include "fw/gui/EventManager.h"
#include "core/DataAccess.h"
#include "core/VariableNode.h"
#include "gui/NodableView.h"
#include "gui/Condition.h"
#include "gui/Event.h"
#include "gui/File.h"
#include "gui/FileView.h"
#include "gui/GraphView.h"
#include "gui/NodeConnector.h"
#include "gui/NodeView.h"
#include "gui/PropertyConnector.h"
#include "gui/commands/Cmd_ConnectEdge.h"
#include "gui/commands/Cmd_DisconnectEdge.h"
#include "gui/commands/Cmd_Group.h"

using namespace ndbl;

Nodable *Nodable::s_instance = nullptr;

Nodable::Nodable()
    : current_file(nullptr)
    , framework(config.framework)
    , view(this)
    , virtual_machine()
{
    LOG_VERBOSE("ndbl::App", "Constructor ...\n");

    // set this instance as s_instance to access it via App::get_instance()
    FW_EXPECT(s_instance == nullptr, "Can't create two concurrent App. Delete first instance.");
    s_instance = this;

    // Bind methods to framework events
    LOG_VERBOSE("ndbl::App", "Binding framework ...\n");
    using fw::App;
    framework.signal_handler = [this](App::Signal evt) {
        switch (evt)
        {
            case App::Signal_ON_INIT:     on_init(); break;
            case App::Signal_ON_DRAW:     on_update(); break;
            case App::Signal_ON_SHUTDOWN: on_shutdown(); break;
            case App::Signal_ON_UPDATE: break;
        }
    };
    LOG_VERBOSE("ndbl::App", "Constructor " OK "\n");
}

Nodable::~Nodable()
{
    s_instance = nullptr;
    LOG_VERBOSE("ndbl::App", "Destructor " OK "\n");
}

bool Nodable::on_init()
{
    LOG_VERBOSE("ndbl::App", "on_init ...\n");

    fw::EventManager& event_manager = framework.event_manager;

    // Bind commands to shortcuts
    using fw::EventType;
    event_manager.bind(
            {"Delete",
             EventType_delete_node_action_triggered,
             {SDLK_DELETE, KMOD_NONE},
             Condition_ENABLE});
    event_manager.bind(
            {"Arrange",
             EventType_arrange_node_action_triggered,
             {SDLK_a, KMOD_NONE},
             Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR});
    event_manager.bind(
            {"Fold",
             EventType_toggle_folding_selected_node_action_triggered,
             {SDLK_x, KMOD_NONE},
             Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR});
    event_manager.bind(
            {"Next",
             EventType_select_successor_node_action_triggered,
             {SDLK_n, KMOD_NONE},
             Condition_ENABLE});
    event_manager.bind(
            {ICON_FA_SAVE " Save",
             fw::EventType_save_file_triggered,
             {SDLK_s, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {ICON_FA_SAVE " Save as",
             fw::EventType_save_file_as_triggered,
             {SDLK_s, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {ICON_FA_TIMES "  Close",
             fw::EventType_close_file_triggered,
             {SDLK_w, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {ICON_FA_FOLDER_OPEN " Open",
             fw::EventType_browse_file_triggered,
             {SDLK_o, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {ICON_FA_FILE " New",
             fw::EventType_new_file_triggered,
             {SDLK_n, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {"Splashscreen",
             fw::EventType_show_splashscreen_triggered,
             {SDLK_F1},
             Condition_ENABLE});
    event_manager.bind(
            {ICON_FA_SIGN_OUT_ALT " Exit",
             fw::EventType_exit_triggered,
             {SDLK_F4, KMOD_ALT},
             Condition_ENABLE});
    event_manager.bind(
            {"Undo",
             fw::EventType_undo_triggered,
             {SDLK_z, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {"Redo",
             fw::EventType_redo_triggered,
             {SDLK_y, KMOD_CTRL},
             Condition_ENABLE});
    event_manager.bind(
            {"Isolate",
             EventType_toggle_isolate_selection,
             {SDLK_i, KMOD_CTRL},
             Condition_ENABLE | Condition_HIGHLIGHTED_IN_TEXT_EDITOR});
    event_manager.bind(
            {"Deselect",
             fw::EventType_none,
             {0, KMOD_NONE, "Double click on bg"},
             Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR});
    event_manager.bind(
            {"Move Graph",
             fw::EventType_none,
             {0, KMOD_NONE, "Drag background"},
             Condition_ENABLE | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR});
    event_manager.bind(
            {"Frame Selection",
             EventType_frame_selected_node_views,
             {SDLK_f, KMOD_NONE},
             Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR});
    event_manager.bind(
            {"Frame All",
             EventType_frame_all_node_views,
             {SDLK_f, KMOD_LCTRL},
             Condition_ENABLE});

    LOG_VERBOSE("ndbl::App", "events ...\n");
    signal_handler(Nodable::Signal_ON_INIT);
    LOG_VERBOSE("ndbl::App", "on_init " OK "\n");

    return true;
}

void Nodable::on_update()
{
    LOG_VERBOSE("ndbl::App", "on_update ...\n");
    fw::EventManager& event_manager = framework.event_manager;

    // 1. Update current file
    if (current_file)
    {
        current_file->update();
    }

    // 2. Handle events

    // shorthand to push all shortcuts to a file view overlay depending on conditions
    auto push_overlay_shortcuts = [&](ndbl::FileView& _view, Condition _condition) -> void {
        for (const auto& _binded_event: event_manager.get_binded_events())
        {
            if( (_binded_event.condition & _condition) == _condition)
            {
                if (_binded_event.condition & Condition_HIGHLIGHTED_IN_GRAPH_EDITOR)
                {
                    _view.push_overlay(
                        {
                            _binded_event.label.substr(0, 12),
                            _binded_event.shortcut.to_string()
                        }, OverlayType_GRAPH);
                }
                if ( _binded_event.condition & Condition_HIGHLIGHTED_IN_TEXT_EDITOR)
                {
                    _view.push_overlay(
                        {
                            _binded_event.label.substr(0,12),
                            _binded_event.shortcut.to_string()
                        }, OverlayType_TEXT);
                }
            }

        }
    };

    // Nodable events ( SDL_ API inspired, but with custom events)
    ndbl::Event event{};
    NodeView*  selected_view = NodeView::get_selected();
    GraphView* graph_view          = current_file ? current_file->get_graph_view() : nullptr;
    History*   curr_file_history   = current_file ? current_file->get_history() : nullptr;

    while(event_manager.poll_event((fw::Event&)event) )
    {
        switch ( event.type )
        {
            case EventType_toggle_isolate_selection:
            {
                config.isolate_selection = !config.isolate_selection;
                if(current_file) current_file->update_graph();
                break;
            }

            case fw::EventType_exit_triggered:
            {
                framework.should_stop = true;
                break;
            }

            case fw::EventType_close_file_triggered:
            {
                if(current_file) close_file(current_file);
                break;
            }
            case fw::EventType_undo_triggered:
            {
                if(curr_file_history) curr_file_history->undo();
                break;
            }

            case fw::EventType_redo_triggered:
            {
                if(curr_file_history) curr_file_history->redo();
                break;
            }

            case fw::EventType_browse_file_triggered:
            {
                std::string path;
                if( pick_file_path(path, fw::AppView::DIALOG_Browse))
                {
                    open_file(path);
                    break;
                }
                LOG_MESSAGE("App", "Browse file aborted by user.\n");
                break;

            }

            case fw::EventType_new_file_triggered:
            {
                new_file();
                break;
            }

            case fw::EventType_save_file_as_triggered:
            {
                if (current_file)
                {
                    std::string path;
                    if(pick_file_path(path, fw::AppView::DIALOG_SaveAs))
                    {
                        save_file_as(path);
                        break;
                    }
                }
                break;
            }

            case fw::EventType_save_file_triggered:
            {
                if (current_file)
                {
                    if( !current_file->path.empty())
                    {
                        save_file(current_file);
                    }
                    else
                    {
                        std::string path;
                        if(pick_file_path(path, fw::AppView::DIALOG_SaveAs))
                        {
                            save_file_as(path);
                        }
                    }
                }
                break;
            }

            case fw::EventType_show_splashscreen_triggered:
            {
                framework.config.splashscreen = true;
                break;
            }
             case EventType_frame_selected_node_views:
            {
                if (graph_view) graph_view->frame_selected_node_views();
                break;
            }

            case EventType_frame_all_node_views:
            {
                if (graph_view) graph_view->frame_all_node_views();
                break;
            }
            case EventType_node_view_selected:
            {
                current_file->view.clear_overlay();
                push_overlay_shortcuts(current_file->view, Condition_ENABLE_IF_HAS_SELECTION);
                LOG_MESSAGE( "App", "NodeView selected\n")
                break;
            }
            case fw::EventType_file_opened:
            {
                if (!current_file) break;
                current_file->update_graph();
                current_file->view.clear_overlay();
                push_overlay_shortcuts(current_file->view, Condition_ENABLE_IF_HAS_NO_SELECTION);
                break;
            }
            case EventType_node_view_deselected:
            {
                current_file->view.clear_overlay();
                push_overlay_shortcuts(current_file->view, Condition_ENABLE_IF_HAS_NO_SELECTION );
                break;
            }
            case EventType_delete_node_action_triggered:
            {
                if ( selected_view && !ImGui::IsAnyItemFocused() )
                {
                    selected_view->get_owner()->flagged_to_delete = true;
                }
                break;
            }

            case EventType_arrange_node_action_triggered:
            {
                if ( selected_view )selected_view->arrange_recursively();
                break;
            }

            case EventType_select_successor_node_action_triggered:
            {
                if ( selected_view )
                {
                    Node* possible_successor = selected_view->get_owner()->successors.get_front_or_nullptr();
                    if (possible_successor)
                    {
                        if (NodeView* successor_view = possible_successor->get_component<NodeView>())
                        {
                            NodeView::set_selected(successor_view);
                        }
                    }
                }
                break;
            }
            case EventType_toggle_folding_selected_node_action_triggered:
            {
                if ( selected_view )
                {
                    if( event.toggle_folding.recursive )
                    {
                        selected_view->expand_toggle_rec();
                    }
                    else
                    {
                        selected_view->expand_toggle();
                    }
                }
                break;
            }
            case EventType_node_connector_dropped:
            {
                const NodeConnector *src = event.node_connectors.src;
                const NodeConnector *dst = event.node_connectors.dst;
                if ( src->share_parent_with(dst) )
                {
                    LOG_WARNING("App", "Unable to drop_on these two Connectors from the same Node.\n")
                }
                else if( src->m_way == dst->m_way )
                {
                    LOG_WARNING("App", "Unable to drop_on these two Node Connectors (must have different ways).\n")
                }
                else
                {
                    if ( src->m_way != Way_Out ) std::swap(src, dst); // ensure src is predecessor
                    DirectedEdge edge(src->get_node(), Edge_t::IS_PREDECESSOR_OF, dst->get_node());
                    auto cmd = std::make_shared<Cmd_ConnectEdge>(edge);
                    curr_file_history->push_command(cmd);
                }
                break;
            }
            case EventType_property_connector_dropped:
            {
                const PropertyConnector *src = event.property_connectors.src;
                const PropertyConnector *dst = event.property_connectors.dst;
                const fw::type* src_meta_type = src->get_property_type();
                const fw::type* dst_meta_type = dst->get_property_type();

                if ( src->share_parent_with(dst) )
                {
                    LOG_WARNING( "App", "Unable to drop_on two connectors from the same Property.\n" )
                }
                else if (src->m_display_side == dst->m_display_side)
                {
                    LOG_WARNING( "App", "Unable to drop_on two connectors with the same nature (in and in, out and out)\n" )
                }
                else if ( !fw::type::is_implicitly_convertible(src_meta_type, dst_meta_type) )
                {
                    LOG_WARNING( "App", "Unable to drop_on %s to %s\n",
                                src_meta_type->get_fullname().c_str(),
                                dst_meta_type->get_fullname().c_str())
                }
                else
                {
                    if (src->m_way != Way_Out) std::swap(src, dst); // guarantee src to be the output
                    DirectedEdge edge(src->get_property(), dst->get_property());
                    auto cmd = std::make_shared<Cmd_ConnectEdge>(edge);
                    curr_file_history->push_command(cmd);
                }
                break;
            }

            case EventType_node_connector_disconnected:
            {
                const NodeConnector* src_connector = event.node_connectors.src;
                Node* src = src_connector->get_node();
                Node* dst = src_connector->get_connected_node();

                if (src_connector->m_way != Way_Out ) std::swap(src, dst); // ensure src is predecessor

                DirectedEdge edge(src, Edge_t::IS_PREDECESSOR_OF, dst);
                auto cmd = std::make_shared<Cmd_DisconnectEdge>( edge );
                curr_file_history->push_command(cmd);
                
                break;
            }
            case EventType_property_connector_disconnected:
            {
                const PropertyConnector* src_connector = event.property_connectors.src;
                Property * src = src_connector->get_property();

                auto edges = src->get_owner()->parent_graph->filter_edges(src, src_connector->m_way);

                auto cmd_grp = std::make_shared<Cmd_Group>("Disconnect All Edges");
                for(auto each_edge: edges)
                {
                    auto each_cmd = std::make_shared<Cmd_DisconnectEdge>(*each_edge);
                    cmd_grp->push_cmd( std::static_pointer_cast<AbstractCommand>(each_cmd) );
                }
                curr_file_history->push_command(std::static_pointer_cast<AbstractCommand>(cmd_grp));

                break;
            }

            default:
            {
                LOG_VERBOSE("App", "Ignoring and event, this case is not handled\n")
            }
        }
    }
    LOG_VERBOSE("ndbl::App", "on_update " OK "\n");
}

bool Nodable::on_shutdown()
{
    LOG_VERBOSE("ndbl::App", "on_shutdown ...\n");
    for( File* each_file : m_loaded_files )
    {
        LOG_VERBOSE("ndbl::App", "Delete file %s ...\n", each_file->path.c_str())
        delete each_file;
    }
    LOG_VERBOSE("ndbl::App", "on_shutdown " OK "\n");
    return true;
}

File *Nodable::open_file(const ghc::filesystem::path& _path)
{
    auto file = new File( fw::App::asset_path(_path) );

    if ( !file->load() )
    {
        LOG_ERROR("File", "Unable to open file %s (%s)\n", _path.filename().c_str(), _path.c_str());
        delete file;
        return nullptr;
    }
    add_file(file);
    return file;
}

File *Nodable::add_file(File* _file)
{
    FW_EXPECT(_file, "File is nullptr");
    m_loaded_files.push_back( _file );
    current_file = _file;
    framework.event_manager.push(fw::EventType_file_opened);
    return _file;
}

void Nodable::save_file(File* _file) const
{
    FW_EXPECT(_file,"file must be defined");

	if ( !_file->write_to_disk() )
    {
        LOG_ERROR("ndbl::App", "Unable to save %s (%s)\n", _file->name.c_str(), _file->path.c_str());
        return;
    }
    LOG_MESSAGE("ndbl::App", "File saved: %s\n", _file->path.c_str());
}

void Nodable::save_file_as(const ghc::filesystem::path& _path) const
{
    ghc::filesystem::path absolute_path = fw::App::asset_path(_path);
    current_file->path = absolute_path.string();
    current_file->name = absolute_path.filename().string();
    if( !current_file->write_to_disk() )
    {
        LOG_ERROR("App", "Unable to save as %s (%s)\n", absolute_path.filename().c_str(), absolute_path.c_str());
    }
}

void Nodable::close_file(File* _file)
{
    // Find and delete the file
    FW_EXPECT(_file, "Cannot close a nullptr File!");
    auto it = std::find(m_loaded_files.begin(), m_loaded_files.end(), _file);
    FW_EXPECT(it != m_loaded_files.end(), "Unable to find the file in the loaded_files");
    it = m_loaded_files.erase(it);
    delete _file;

    // Switch to the next file if possible
    if ( it != m_loaded_files.end() )
    {
        current_file = *it;
    }
    else
    {
        current_file = nullptr;
    }
}

bool Nodable::compile_and_load_program()
{
    if ( current_file )
    {
        const Graph* graph = current_file->get_graph();

        if (graph)
        {
            assembly::Compiler compiler;
            auto asm_code = compiler.compile_syntax_tree(graph);

            if (asm_code)
            {
                virtual_machine.release_program();

                if (virtual_machine.load_program(std::move(asm_code)))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void Nodable::run_program()
{
    if (compile_and_load_program() )
    {
        virtual_machine.run_program();
    }
}

void Nodable::debug_program()
{
    if (compile_and_load_program() )
    {
        virtual_machine.debug_program();
    }
}

void Nodable::step_over_program()
{
    virtual_machine.step_over();
    if (!virtual_machine.is_there_a_next_instr() )
    {
        NodeView::set_selected(nullptr);
    }
    else if (NodeView* next_node_view = virtual_machine.get_next_node()->get_component<NodeView>() )
    {
        NodeView::set_selected(next_node_view);
    }
}

void Nodable::stop_program()
{
    virtual_machine.stop_program();
}

void Nodable::reset_program()
{
    if(!current_file) return;

    if (virtual_machine.is_program_running() )
    {
        virtual_machine.stop_program();
    }
    current_file->update_graph();
}

File *Nodable::new_file()
{
    m_untitled_file_count++;
    std::string name{"Untitled_"};
    name.append(std::to_string(m_untitled_file_count));
    name.append(".cpp");

    File* file = new File(ghc::filesystem::path{name});
    // file->set_text( "// " + name);

    return add_file(file);
}

Nodable &Nodable::get_instance()
{
    FW_EXPECT(s_instance, "No App instance available. Did you forget App app(...) or App* app = new App(...)");
    return *s_instance;
}

void Nodable::toggle_fullscreen()
{
    framework.set_fullscreen(!is_fullscreen() );
}

bool Nodable::is_fullscreen() const
{
    return framework.is_fullscreen();
}

bool Nodable::pick_file_path(std::string &out, fw::AppView::DialogType type)
{
    return framework.view.pick_file_path(out, type);
}