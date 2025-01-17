#include "File.h"

#include <fstream>

#include "ndbl/core/ASTUtils.h"
#include "ndbl/core/ASTFunctionCall.h"
#include "ndbl/core/ASTLiteral.h"
#include "ndbl/core/language/Nodlang.h"

#include "GraphView.h"
#include "FileView.h"
#include "History.h"
#include "ASTNodeView.h"
#include "PhysicsComponent.h"

using namespace ndbl;
using namespace tools;

File::File()
: path()
, view()
, history()
, _flags(Flags_NEEDS_TO_BE_SAVED | Flags_GRAPH_IS_DIRTY ) // we're text-based!
{
    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "File", "Constructor being called ...\n");

    // Graph
    _graph = new Graph();
    auto* graph_view = _graph->components()->create<GraphView>();

    _graph->signal_change.connect<&File::set_text_dirty>(this);
    graph_view->signal_change.connect<&File::set_text_dirty>(this);

    // Fill the "create node" context menu
    for( IAction* action : get_action_manager()->get_actions() )
        if ( auto create_node_action = dynamic_cast<Action_CreateNode*>(action))
            graph_view->add_action_to_node_menu(create_node_action);

    // FileView
    view.init(*this);
    view.signal_text_view_changed.connect<&File::set_graph_dirty>(this);
    view.signal_graph_view_changed.connect<&File::set_text_dirty>(this);

    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "File", "View built, creating History ...\n");

    // History
    TextEditor*       text_editor     = view.get_text_editor();
    TextEditorBuffer* text_editor_buf = history.configure_text_editor_undo_buffer(text_editor);
    view.set_undo_buffer(text_editor_buf);
    TOOLS_DEBUG_LOG(TOOLS_MESSAGE, "File", "Constructor being called.\n");
}

File::~File()
{
    assert(_graph->signal_change.disconnect<&File::set_text_dirty>(this));
    assert(_graph->component<GraphView>()->signal_change.disconnect<&File::set_text_dirty>(this));
    assert(view.signal_text_view_changed.disconnect<&File::set_graph_dirty>(this));
    assert(view.signal_graph_view_changed.disconnect<&File::set_text_dirty>(this));

    delete _graph;
}

void File::_update_text_from_graph()
{
    if ( auto* root_node = _graph->root_node() )
    {
        std::string code;
        get_language()->serialize_node(code, root_node, SerializeFlag_RECURSE);
        view.set_text(code, _isolation );
    }
    else
    {
        TOOLS_LOG(TOOLS_WARNING, "File", "Unable to update text from graph: no root found in the Graph.\n");
    }
}

void File::update()
{
    //
    // When history is dirty we update the graph from the text.
    // (By default undo/redo are text-based only, if hybrid_history is ON, the behavior is different
    if ( history.is_dirty )
    {
        if ( get_config()->has_flags(ConfigFlag_EXPERIMENTAL_HYBRID_HISTORY) )
        {
            ASSERT(false); // Not implemented yet
        }
        else
        {
            _flags = _flags & ~Flags_TEXT_IS_DIRTY // unset text is dirty
                   | Flags_GRAPH_IS_DIRTY; // set graph dirty (we are text-based!)
        }
        history.is_dirty = false;
    }

    if ( _flags & Flags_GRAPH_IS_DIRTY )
    {
        _update_graph_from_text();
        _graph->update();
        _flags = _flags & ~Flags_IS_DIRTY_MASK;  // clear dirty flags
    }
    else if ( _flags & Flags_TEXT_IS_DIRTY )
    {
        _graph->update();
        _update_text_from_graph();
        _flags = _flags & ~Flags_IS_DIRTY_MASK;  // clear dirty flags
    }
    else
    {
        _graph->update();
    }
}

void File::_update_graph_from_text()
{
    // Parse source code
    // note: File owns the parsed text buffer
    _parsed_text = view.get_text(_isolation);
    get_language()->parse(_graph, _parsed_text);
}

size_t File::size() const
{
    return view.size();
}

std::string File::filename() const
{
    return path.filename().string();
}

bool File::write( File& file, const tools::Path& path)
{
    if( path.empty() )
    {
        TOOLS_LOG(TOOLS_ERROR, "File", "No path defined, unable to save file\n");
        return false;
    }

    if ( (file._flags & Flags_NEEDS_TO_BE_SAVED) == 0 )
    {
        TOOLS_LOG(TOOLS_MESSAGE, "File", "Nothing to save\n");
    }

    std::ofstream out_fstream(path.string());
    std::string result;
    result = file.view.get_text(file._isolation);
    std::string content = result;
    out_fstream.write(content.c_str(), content.size()); // TODO: size can exceed fstream!
    file._flags &= ~Flags_NEEDS_TO_BE_SAVED; // unset flag
    file.path = path;
    TOOLS_LOG(TOOLS_MESSAGE, "File", "%s saved\n", file.filename().c_str() );

    return true;
}

bool File::read( File& file, const tools::Path& path)
{
    TOOLS_LOG(TOOLS_MESSAGE, "File", "\"%s\" loading... (%s).\n", path.filename().c_str(), path.c_str());
    if(path.empty() )
    {
        TOOLS_LOG(TOOLS_ERROR, "File", "Path is empty \"%s\"\n", path.c_str());
        return false;
    }

    std::ifstream file_stream(path.string());
    if (!file_stream.is_open())
    {
        TOOLS_LOG(TOOLS_ERROR, "File", "Unable to load \"%s\"\n", path.c_str());
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
    file.view.set_text(content, file._isolation);
    file._flags &= ~Flags_NEEDS_TO_BE_SAVED; // unset flag
    file.path = path;

    TOOLS_LOG(TOOLS_MESSAGE, "File", "\"%s\" loaded (%s).\n", path.filename().c_str(), path.c_str());

    return true;
}

void File::set_isolation(Isolation isolation)
{
    if ( _isolation == isolation )
        return;

    _isolation   = isolation;
    _parsed_text = view.get_text(_isolation);

    // when isolation changes, the text has the priority over the graph.
    _flags &= ~Flags_IS_DIRTY_MASK; // unset flags
    _flags |= Flags_GRAPH_IS_DIRTY;
}