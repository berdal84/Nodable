#include "File.h"

#include <fstream>

#include "ndbl/core/Utils.h"
#include "ndbl/core/FunctionNode.h"
#include "ndbl/core/LiteralNode.h"
#include "ndbl/core/language/Nodlang.h"

#include "GraphView.h"
#include "History.h"
#include "NodeView.h"
#include "Physics.h"

using namespace ndbl;
using namespace tools;

File::File()
: path()
, dirty(true)
, view()
, history()
{
    LOG_VERBOSE( "File", "Constructor being called ...\n");

    // FileView
    view.init(*this);
    CONNECT(view.on_text_view_changed, &File::update_graph_from_text);
    CONNECT(view.on_graph_view_changed, &File::update_text_from_graph);

    LOG_VERBOSE( "File", "View built, creating History ...\n");

    // History
    TextEditor*       text_editor     = view.get_text_editor();
    TextEditorBuffer* text_editor_buf = history.configure_text_editor_undo_buffer(text_editor);
    view.set_undo_buffer(text_editor_buf);

    LOG_VERBOSE( "File", "History built, creating graph ...\n");

    // Graph
    _graph = new Graph(get_node_factory());
    auto* graph_view = new GraphView(_graph);
    _graph->set_view(graph_view);

    // Fill the "create node" context menu
    for( IAction* action : get_action_manager()->get_actions() )
        if ( auto create_node_action = dynamic_cast<Action_CreateNode*>(action))
            _graph->view()->add_action_to_node_menu(create_node_action);

    LOG_VERBOSE( "File", "Constructor being called.\n");
}

File::~File()
{
    DISCONNECT(view.on_text_view_changed);
    DISCONNECT(view.on_graph_view_changed);

    delete _graph->view();
    delete _graph;
}

void File::update_text_from_graph()
{
    if ( _graph->root() )
    {
        std::string code;
        get_language()->_serialize_node( code, _graph->root().get(), SerializeFlag_RECURSE );
        view.set_text(code, _isolation );
    }
    else
    {
        LOG_WARNING("File", "Unable to update text from graph: no root found in the Graph.\n");
    }
}

void File::update()
{
    //
    // When history is dirty we update the graph from the text.
    // (By default undo/redo are text-based only, if hybrid_history is ON, the behavior is different
    if (history.is_dirty && !get_config()->has_flags(ConfigFlag_EXPERIMENTAL_HYBRID_HISTORY) )
    {
        update_graph_from_text();
        history.is_dirty = false;
    }

    if( _requires_text_update )
    {
        update_text_from_graph();
        _requires_text_update = false;
    }
    else if( _requires_graph_update )
    {
        update_graph_from_text();
        _requires_graph_update = false;
    }

   _graph->update(); // ~ garbage collection
}

void File::update_graph_from_text()
{
    // Parse source code
    // note: File owns the parsed text buffer
    std::string result;
    result = view.get_text(_isolation);
    _parsed_text = result;
    get_language()->parse(_parsed_text, _graph );
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
        LOG_ERROR("File", "No path defined, unable to save file\n");
        return false;
    }

    if ( !file.dirty )
    {
        LOG_MESSAGE("File", "Nothing to save\n");
    }

    std::ofstream out_fstream(path.string());
    std::string result;
    result = file.view.get_text(file._isolation);
    std::string content = result;
    out_fstream.write(content.c_str(), content.size()); // TODO: size can exceed fstream!
    file.dirty = false;
    file.path = path;
    LOG_MESSAGE("File", "%s saved\n", file.filename().c_str() );

    return true;
}

bool File::read( File& file, const tools::Path& path)
{
    LOG_MESSAGE("File", "\"%s\" loading... (%s).\n", path.filename().c_str(), path.c_str());
    if(path.empty() )
    {
        LOG_ERROR("File", "Path is empty \"%s\"\n", path.c_str());
        return false;
    }

    std::ifstream file_stream(path.string());
    if (!file_stream.is_open())
    {
        LOG_ERROR("File", "Unable to load \"%s\"\n", path.c_str());
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
    file.view.set_text(content, file._isolation);
    file.dirty = false;
    file.path = path;

    LOG_MESSAGE("File", "\"%s\" loaded (%s).\n", path.filename().c_str(), path.c_str());

    return true;
}

void File::set_isolation(Isolation isolation)
{
    if ( _isolation == isolation )
        return;

    _isolation   = isolation;
    _parsed_text = view.get_text(_isolation);

    // when isolation changes, the text has the priority over the graph.
    _requires_graph_update = true;
    _requires_text_update  = false;
}
