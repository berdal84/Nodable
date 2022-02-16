#include <nodable/ForLoopNode.h>
#include <nodable/Scope.h>

using namespace Nodable;

R_DEFINE_CLASS(ForLoopNode)

ForLoopNode::ForLoopNode()
        :
        m_token_for(nullptr)
{
    m_props.add("init"     , Visibility::Always, R::Type::Unknown        , Way::Way_In);
    m_props.add("condition", Visibility::Always, R::add_ptr(R::Type::Void)            , Way::Way_In);
    m_props.add("iter"     , Visibility::Always, R::Type::Unknown        , Way::Way_In);
}

Scope* ForLoopNode::get_condition_true_branch() const
{
    return !m_successors.empty() ? m_successors[0]->get<Scope>() : nullptr;
}

Scope*  ForLoopNode::get_condition_false_branch() const
{
    return m_successors.size() > 1 ? m_successors[1]->get<Scope>() : nullptr;
}
