#pragma once
#include "ndbl/gui/Command.h"
#include "ndbl/core/ASTNode.h"
#include "ndbl/core/Graph.h"

namespace ndbl
{
    class Cmd_ConnectEdge : public AbstractCommand
    {
    public:
        explicit Cmd_ConnectEdge(ASTSlotLink _edge)
        : m_edge(_edge)
        , m_graph(_edge.tail->node->graph()) // deduce graph from edge source' owner
        {
            char str[200];
            snprintf(str
                    , sizeof(str)
                    , "ConnectEdge\n"
                      " - tail: \"%s\"\n"
                      " - head: \"%s\"\n"
                    , _edge.tail->node->name().c_str()
                    , _edge.head->node->name().c_str()
            );
            m_description.append(str);
        }

        ~Cmd_ConnectEdge() override = default;

        void execute() override
        { m_graph->connect( m_edge.tail, m_edge.head, GraphFlag_ALLOW_SIDE_EFFECTS ); }

        void undo() override
        { m_graph->disconnect(m_edge, GraphFlag_ALLOW_SIDE_EFFECTS ); }

        const char* get_description() const override
        { return m_description.c_str(); }

    private:
        std::string   m_description;
        ASTSlotLink  m_edge;
        Graph*        m_graph;
    };
}