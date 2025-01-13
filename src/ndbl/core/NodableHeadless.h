#pragma once

#include <string>

namespace tools
{
    struct PoolManager;
    struct TaskManager;
}

namespace ndbl
{
    // forward declarations
    class Graph;
    class ASTNodeFactory;
    class Nodlang;

    class NodableHeadless
    {
    public:
        NodableHeadless() = default;
        virtual ~NodableHeadless() = default;
        virtual void        init();
        virtual void        update();
        virtual void        shutdown();
        virtual void        clear();
        bool                should_stop() const;
        virtual std::string& serialize( std::string& out ) const;
        virtual Graph*      parse( const std::string& in );
        Nodlang*            get_language() const;
        Graph*              graph() const;
    protected:
        tools::TaskManager* m_task_manager{};
        Nodlang*            m_language{};
        ASTNodeFactory*     m_node_factory{};
        bool                m_should_stop{false};
        Graph*              m_graph{};
        std::string         m_source_code;
        bool                m_auto_completion{false};
    };
}

