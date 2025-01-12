#include "NodableHeadless.h"
#include "ndbl/core/language/Nodlang.h"
#include "tools/core/TaskManager.h"

using namespace ndbl;

void NodableHeadless::init()
{
    // init managers
    m_task_manager    = tools::init_task_manager();
    m_language        = init_language();
    m_node_factory    = init_node_factory();

    // configure
    m_graph = new Graph(m_node_factory);
    m_language->_state.reset( m_graph ); // in some cases (like during tests), we call parse_xxx methods that implicitly requires the state to be reset
}

void NodableHeadless::shutdown()
{
    ASSERT(m_graph);
    clear();
    delete m_graph;
    tools::shutdown_task_manager(m_task_manager);
    shutdown_language(m_language);
    shutdown_node_factory(m_node_factory);
}

std::string& NodableHeadless::serialize( std::string& out ) const
{
    return m_language->serialize_graph(out, m_graph);
}

Graph* NodableHeadless::parse( const std::string& code )
{
    m_language->parse( m_graph, code );
    return m_graph;
}

Nodlang* NodableHeadless::get_language() const
{
    return m_language;
}

Graph* NodableHeadless::graph() const
{
    return m_graph;
}

void NodableHeadless::update()
{
}

void NodableHeadless::clear()
{
    m_graph->reset();
    m_source_code.clear();
}

bool NodableHeadless::should_stop() const
{
    return m_should_stop;
}