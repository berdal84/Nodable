#pragma once
#include <nodable/app/Command.h>
#include <nodable/core/Node.h>
#include <nodable/core/GraphNode.h>

namespace ndbl
{
    class Cmd_ConnectEdge : public IUndoableCmd
    {
    public:
        Cmd_ConnectEdge(DirectedEdge _edge)
        : m_edge(_edge)
        , m_graph(_edge.prop.src->get_owner()->get_parent_graph())
        {
            char str[200];
            snprintf(str
                    , sizeof(str)
                    , "ConnectEdge\n"
                      " - src: \"%s\"\n"
                      " - dst: \"%s\"\n"
                      " - edge: \"%s\"\n"
                    , _edge.prop.src->get_name().c_str()
                    , _edge.prop.dst->get_name().c_str()
                    , to_string(_edge.type) );
            m_description.append(str);
        }

        ~Cmd_ConnectEdge() override = default;

        void execute() override
        {
            m_graph->connect(m_edge);
        }

        void undo() override
        {
            m_graph->disconnect(&m_edge);
        }

        const char* get_description() const override
        {
            return m_description.c_str();
        }

    private:
        std::string   m_description;
        DirectedEdge  m_edge;
        GraphNode*    m_graph;
    };
}