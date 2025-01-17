#include "Nodable.h"

#include <algorithm>

#if PLATFORM_WEB
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "tools/core/assertions.h"
#include "tools/core/System.h"
#include "tools/core/EventManager.h"

#include "ndbl/core/ASTFunctionCall.h"
#include "ndbl/core/ASTLiteral.h"
#include "ndbl/core/ASTNodeSlot.h"
#include "ndbl/core/language/Nodlang.h"

#include "commands/Cmd_ConnectEdge.h"
#include "commands/Cmd_DeleteEdge.h"
#include "commands/Cmd_Group.h"

#include "Condition.h"
#include "Config.h"
#include "Event.h"
#include "File.h"
#include "GraphView.h"
#include "NodableView.h"
#include "ASTNodeView.h"
#include "ASTNodeSlotView.h"
#include "ndbl/core/ASTUtils.h"

using namespace ndbl;
using namespace tools;

Nodable* Nodable::s_instance = nullptr;

void Nodable::init()
{
    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "ndbl::Nodable", "init_ex ...\n");

    m_config = init_config();
    m_view = new NodableView();
    m_base_app.init_ex(m_view->get_base_view_handle(), m_config->tools_cfg ); // the pointers are owned by this class, base app just use them.
    m_language          = init_language();
    m_view->init(this); // must be last

    s_instance = this;

    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "ndbl::Nodable", "init_ex " TOOLS_OK "\n");
}

void Nodable::_do_frame()
{
    update();
    draw();
};

#if PLATFORM_WEB
void emscripten_do_frame()
{
    Nodable::instance()->_do_frame();
}
#endif

void Nodable::run()
{
#if PLATFORM_WEB
    emscripten_set_main_loop(&emscripten_do_frame, 0, true);
#else
    while( !should_stop() )
    {
        _do_frame();
    }
#endif
}

void Nodable::update()
{
    m_base_app.update();
    m_view->update();

    // 1. delete flagged files
    for( File* file : m_flagged_to_delete_file )
    {
        TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "Nodable", "Delete files flagged to delete: %s\n", file->filename().c_str());
        delete file;
    }
    m_flagged_to_delete_file.clear();

    // 2. Update current file
    if (m_current_file)
    {
        m_current_file->set_isolation( m_config->isolation ); // might change
        m_current_file->update();
    }

    // 3. Handle events

    // Nodable events
    IEvent*       event = nullptr;
    EventManager* event_manager     = get_event_manager();
    GraphView*    graph_view        = m_current_file ? m_current_file->graph()->component<GraphView>() : nullptr; // TODO: should be included in the event
    History*      curr_file_history = m_current_file ? &m_current_file->history : nullptr; // TODO: should be included in the event
    while( (event = event_manager->poll_event()) )
    {
        switch ( event->id )
        {
            case EventID_RESET_GRAPH:
            {
                m_current_file->set_graph_dirty();
                break;
            }

            case EventID_TOGGLE_ISOLATION_FLAGS:
            {
                m_config->isolation = ~m_config->isolation;
                if(m_current_file)
                {
                    m_current_file->set_graph_dirty();
                }
                break;
            }

            case EventID_REQUEST_EXIT:
            {
                m_base_app.request_stop();
                break;
            }

            case EventID_FILE_CLOSE:
            {
                if(m_current_file) close_file(m_current_file);
                break;
            }
            case EventID_UNDO:
            {
                if(curr_file_history) curr_file_history->undo();
                break;
            }

            case EventID_REDO:
            {
                if(curr_file_history) curr_file_history->redo();
                break;
            }

            case EventID_FILE_BROWSE:
            {
                Path path;
                if( m_view->pick_file_path(path, AppView::DIALOG_Browse))
                {
                    open_file(path);
                    break;
                }
                TOOLS_LOG(TOOLS_MESSAGE, "App", "Browse file aborted by user.\n");
                break;

            }

            case EventID_FILE_NEW:
            {
                new_file();
                break;
            }

            case EventID_FILE_SAVE_AS:
            {
                if (m_current_file != nullptr)
                {
                    Path path;
                    if( m_view->pick_file_path(path, AppView::DIALOG_SaveAs))
                    {
                        save_file_as(m_current_file, path);
                    }
                }

                break;
            }

            case EventID_FILE_SAVE:
            {
                if (!m_current_file) break;
                if( !m_current_file->path.empty())
                {
                    save_file(m_current_file);
                }
                else
                {
                    Path path;
                    if( m_view->pick_file_path(path, AppView::DIALOG_SaveAs))
                    {
                        save_file_as(m_current_file, path);
                    }
                }
                break;
            }

            case Event_ShowWindow::id:
            {
                auto _event = reinterpret_cast<Event_ShowWindow*>(event);
                if ( _event->data.window_id == "splashscreen" )
                {
                    m_view->show_splashscreen(_event->data.visible);
                }
                break;
            }

            case Event_FrameSelection::id:
            {
                auto _event = reinterpret_cast<Event_FrameSelection*>( event );
                VERIFY(graph_view, "a graph_view is required");
                graph_view->frame_content(_event->data.mode);
                break;
            }

            case EventID_FILE_OPENED:
            {
                ASSERT(m_current_file != nullptr );
                m_current_file->view.clear_overlay();
                m_current_file->view.refresh_overlay(Condition_ENABLE_IF_HAS_NO_SELECTION );
                break;
            }
            case Event_DeleteSelection::id:
            {
                if ( graph_view )
                {
                    for(const Selectable& elem : graph_view->selection() )
                    {
                        if ( auto nodeview = elem.get_if<ASTNodeView*>() )
                            graph_view->graph()->flag_node_to_delete(nodeview->node(), GraphFlag_NONE);
                        else if ( auto scopeview = elem.get_if<ASTScopeView*>() )
                            graph_view->graph()->flag_node_to_delete(scopeview->node(), GraphFlag_ALLOW_SIDE_EFFECTS);
                    }
                }

                break;
            }

            case Event_ArrangeSelection::id:
            {
                if ( graph_view )
                {
                    for( const Selectable& elem : graph_view->selection() )
                    {
                        switch ( elem.index() )
                        {
                            case Selectable::index_of<ASTNodeView*>():
                                elem.get<ASTNodeView*>()->arrange_recursively();
                                break;
                            case Selectable::index_of<ASTScopeView*>():
                                elem.get<ASTScopeView*>()->arrange_content();
                                break;
                        }
                    }
                }

                break;
            }

            case Event_SelectNext::id:
            {
                if ( graph_view && graph_view->selection().contains<ASTNodeView*>() )
                {
                    graph_view->selection().clear();
                    for(auto* _view : graph_view->selection().collect<ASTNodeView*>() )
                        for (auto* _successor : _view->node()->component<ASTNode>()->flow_outputs() )
                            if (auto* _successor_view = _successor->component<ASTNodeView>() )
                                graph_view->selection().append( _successor_view );
                }
                break;
            }

            case Event_ToggleFolding::id:
            {
                if ( graph_view )
                    break;

                for( ASTNodeView* view : graph_view->selection().collect<ASTNodeView*>() )
                {
                    auto _event = reinterpret_cast<Event_ToggleFolding*>(event);
                    _event->data.mode == RECURSIVELY ? view->expand_toggle_rec()
                                                     : view->expand_toggle();
                }
                break;
            }

            case Event_SlotDropped::id:
            {
                ASSERT(curr_file_history != nullptr);
                auto _event = reinterpret_cast<Event_SlotDropped*>(event);
                ASTNodeSlot* tail = _event->data.first;
                ASTNodeSlot* head = _event->data.second;
                ASSERT(head != tail);
                if ( tail->order() == SlotFlag_ORDER_2ND )
                {
                    if ( head->order() == SlotFlag_ORDER_2ND )
                    {
                        TOOLS_LOG(TOOLS_ERROR, "Nodable", "Unable to connect incompatible edges\n");
                        break; // but if it still the case, that's because edges are incompatible
                    }
                    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "Nodable", "Swapping edges to try to connect them\n");
                    std::swap(tail, head);
                }
                ASTSlotLink edge(tail, head);
                auto cmd = std::make_shared<Cmd_ConnectEdge>(edge);
                curr_file_history->push_command(cmd);

                break;
            }

            case Event_DeleteEdge::id:
            {
                ASSERT(curr_file_history != nullptr);
                auto command = std::make_shared<Cmd_DeleteEdge>( static_cast<Event_DeleteEdge*>(event) );
                curr_file_history->push_command(std::static_pointer_cast<AbstractCommand>(command));
                break;
            }

            case Event_SlotDisconnectAll::id:
            {
                ASSERT(curr_file_history != nullptr);
                auto _event = static_cast<Event_SlotDisconnectAll*>(event);
                ASTNodeSlot* slot = _event->data.first;

                auto cmd_grp = std::make_shared<Cmd_Group>("Disconnect All Edges");
                Graph* graph = _event->data.first->node->graph();
                for(ASTNodeSlot* adjacent_slot: slot->adjacent() )
                {
                    auto each_cmd = std::make_shared<Cmd_DeleteEdge>(ASTSlotLink{slot, adjacent_slot}, graph );
                    cmd_grp->push_cmd( std::static_pointer_cast<AbstractCommand>(each_cmd) );
                }
                curr_file_history->push_command(std::static_pointer_cast<AbstractCommand>(cmd_grp));
                break;
            }

            case Event_CreateNode::id:
            {
                auto _event = reinterpret_cast<Event_CreateNode*>(event);
                Graph* graph = _event->data.graph;

                // 1) create the node
                if ( !graph->root_node() )
                {
                    TOOLS_LOG(TOOLS_ERROR, "Nodable", "Unable to create_new primary_child, no root found on this graph.\n");
                    continue;
                }

                ASTNode* new_node  = graph->create_node( _event->data.node_type,
                                                         _event->data.node_signature,
                                                         graph->root_scope() );

                // Insert an end of line and end of instruction
                switch ( _event->data.node_type )
                {
                    case CreateNodeType_BLOCK_CONDITION:
                    case CreateNodeType_BLOCK_FOR_LOOP:
                    case CreateNodeType_BLOCK_WHILE_LOOP:
                    case CreateNodeType_BLOCK_SCOPE:
                    case CreateNodeType_ROOT:
                        new_node->set_suffix(ASTToken::s_end_of_line );
                        break;
                    case CreateNodeType_VARIABLE_BOOLEAN:
                    case CreateNodeType_VARIABLE_DOUBLE:
                    case CreateNodeType_VARIABLE_INTEGER:
                    case CreateNodeType_VARIABLE_STRING:
                        new_node->set_suffix(ASTToken::s_end_of_instruction );
                        break;
                    case CreateNodeType_LITERAL_BOOLEAN:
                    case CreateNodeType_LITERAL_DOUBLE:
                    case CreateNodeType_LITERAL_INTEGER:
                    case CreateNodeType_LITERAL_STRING:
                    case CreateNodeType_FUNCTION:
                        break;
                }

                // 2) handle connections
                if ( ASTNodeSlotView* slot_view = _event->data.active_slotview )
                {
                    SlotFlags             complementary_flags = switch_order(slot_view->slot->type_and_order());
                    const TypeDescriptor* type                = slot_view->property()->get_type();
                    ASTNodeSlot*                 complementary_slot  = new_node->find_slot_by_property_type(complementary_flags, type);

                    if ( !complementary_slot )
                    {
                        // TODO: this case should not happens, instead we should check ahead of time whether or not this not can be attached
                        TOOLS_LOG(TOOLS_ERROR,  "GraphView", "unable to connect this primary_child" );
                    }
                    else
                    {
                        ASTNodeSlot* out = slot_view->slot;
                        ASTNodeSlot* in  = complementary_slot;

                        if ( out->has_flags( SlotFlag_ORDER_2ND ) )
                            std::swap( out, in );

                        graph->connect(out, in, GraphFlag_ALLOW_SIDE_EFFECTS );

                        // Ensure has a "\n" when connecting using CODEFLOW (to split lines)
                        if (ASTUtils::is_instruction(out->node ) && out->type() == SlotFlag_TYPE_FLOW )
                        {
                            ASTToken& token = out->node->suffix();
                            std::string buffer = token.string();
                            if ( buffer.empty() || std::find(buffer.rbegin(), buffer.rend(), '\n') == buffer.rend() )
                                token.suffix_push_back("\n");
                        }
                    }
                }

                // set new_node's view position, select it
                if ( auto view = new_node->component<ASTNodeView>() )
                {
                    view->spatial_node()->set_position(_event->data.desired_screen_pos, WORLD_SPACE);
                    graph_view->selection().clear();
                    graph_view->selection().append(view);
                }
                break;
            }

            default:
            {
                TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "App", "Ignoring and event, this case is not handled\n");
            }
        }

        // clean memory
        delete event;
    }
}

void Nodable::shutdown()
{
    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "ndbl::Nodable", "_handle_shutdown ...\n");

    for( File* each_file : m_loaded_files )
    {
        TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "ndbl::App", "Delete file %s ...\n", each_file->path.c_str());
        delete each_file;
    }

    // shutdown managers & co.
    shutdown_language(m_language);
    m_view->shutdown();
    m_base_app.shutdown();
    shutdown_config(m_config);

    delete m_view;
    s_instance = nullptr;

    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "ndbl::Nodable", "_handle_shutdown " TOOLS_OK "\n");
}

File* Nodable::open_asset_file(const tools::Path& _path)
{
    if ( _path.is_absolute() )
        return open_file(_path);

    Path path = _path;
    App::make_absolute(path);
    return open_file(path);
}

File* Nodable::open_file(const tools::Path& _path)
{
    File* file = new File();

    if ( File::read( *file, _path ) )
    {
        return add_file(file);
    }

    delete file;
    TOOLS_LOG(TOOLS_ERROR, "File", "Unable to open file %s (%s)\n", _path.filename().c_str(), _path.c_str());
    return nullptr;
}

File* Nodable::add_file( File* _file)
{
    VERIFY(_file, "File is nullptr");
    m_loaded_files.push_back( _file );
    m_current_file = _file;
    get_event_manager()->dispatch( EventID_FILE_OPENED );
    return _file;
}

void Nodable::save_file( File* _file) const
{
    VERIFY(_file, "file must be defined");

	if ( !File::write(*_file, _file->path) )
    {
        TOOLS_LOG(TOOLS_ERROR, "ndbl::App", "Unable to save %s (%s)\n", _file->filename().c_str(), _file->path.c_str());
        return;
    }
    TOOLS_LOG(TOOLS_MESSAGE, "ndbl::App", "File saved: %s\n", _file->path.c_str());
}

void Nodable::save_file_as(File* _file, const tools::Path& _path) const
{
    if ( !File::write(*_file, _path) )
    {
        TOOLS_LOG(TOOLS_ERROR, "ndbl::App", "Unable to save %s (%s)\n", _path.filename().c_str(), _path.c_str());
        return;
    }
    TOOLS_LOG(TOOLS_MESSAGE, "ndbl::App", "File saved: %s\n", _path.c_str());
}

void Nodable::close_file( File* _file)
{
    // Find and delete the file
    VERIFY(_file, "Cannot close a nullptr File!");
    auto it = std::find(m_loaded_files.begin(), m_loaded_files.end(), _file);
    VERIFY(it != m_loaded_files.end(), "Unable to find the file in the loaded_files");
    it = m_loaded_files.erase(it);
    m_flagged_to_delete_file.push_back(_file);

    // Switch to the next file if possible
    if ( it != m_loaded_files.end() )
    {
        m_current_file = *it;
    }
    else
    {
        m_current_file = nullptr;
    }
}

void Nodable::reset_current_graph()
{
    if(!m_current_file) return;

    // n.b. nodable is still text oriented
    m_current_file->set_graph_dirty();
}

File*Nodable::new_file()
{
    m_untitled_file_count++;

    string32 name;
    name.append_fmt("Untitled_%i.cpp", m_untitled_file_count);
    auto* file = new File();
    file->path = name.c_str();

    return add_file(file);
}

NodableView* Nodable::get_view() const
{
    return reinterpret_cast<NodableView*>(m_view);
}

bool Nodable::should_stop() const
{
    return m_base_app.should_stop();
}

void Nodable::draw()
{
    // we have our own view, so we bypass base App
    // m_base_app.draw();
    m_view->draw();
}

void Nodable::set_current_file(File* file)
{
    if ( m_current_file == nullptr )
    {
        m_current_file = file;
        return;
    }

    // TODO:
    //  - unload current file?
    //  - keep the last N files loaded?
    //  - save graph to a temp file to restore it later without using memory and altering original source file?
    // close_file(m_current_file); ??

    m_current_file = file;
}
