#include "ASTSwitchBehavior.h"

using namespace ndbl;

void ASTSwitchBehavior::init(ASTNode* node, size_t branch_count)
{
    VERIFY( node != nullptr, "Please provide this Node*" );
    VERIFY( 1 < branch_count && branch_count <= BRANCH_MAX, "branch_count is out of range");

    m_branch_count = branch_count;

    node->add_slot(node->value(), SlotFlag_FLOW_IN  , ASTNodeSlot::MAX_CAPACITY); // accepts N inputs
    node->add_slot(node->value(), SlotFlag_FLOW_OUT , 1); // accepts 0 or 1 output

    node->init_internal_scope();

    // add 1 slot per branch
    for(size_t branch = 0; branch < branch_count; ++branch )
    {
        m_branch_slot[branch] = node->add_slot(node->value(), SlotFlag_FLOW_ENTER, 1, branch);
    }

    // add 1 condition per branch except for the default branch
    for(size_t branch = 1; branch < branch_count; ++branch )
    {
        auto condition_property  = node->add_prop<tools::any>(CONDITION_PROPERTY);
        m_condition_in[branch-1] = node->add_slot(condition_property, SlotFlag_INPUT, 1, branch);
    }
}
