#include <nodable/ConditionalStructNode.h>
#include <nodable/Scope.h>

using namespace Nodable;

REFLECT_DEFINE(ConditionalStructNode)

ConditionalStructNode::ConditionalStructNode()
    :
    m_token_if(nullptr),
    m_token_else(nullptr)
{
    m_props.add("condition", Visibility::Always, Type_Boolean, Way::Way_In);
    setPrevMaxCount(1); // allow 1 Nodes to be previous.
    setNextMaxCount(2); // allow 2 Nodes to be next (branches if and else).
}

Scope* ConditionalStructNode::get_condition_true_branch() const
{
    return !m_next.empty() ? m_next[0]->get<Scope>() : nullptr;
}

Scope* ConditionalStructNode::get_condition_false_branch() const
{
    return m_next.size() > 1 ? m_next[1]->get<Scope>() : nullptr;
}
