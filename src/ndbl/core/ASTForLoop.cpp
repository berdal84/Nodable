#include "ASTForLoop.h"
#include "ASTNode.h"
#include "ASTUtils.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTForLoop).extends<ASTNode>();
)

void ASTForLoop::init(const std::string& _name)
{
    ASTNode::init(ASTNodeType_FOR_LOOP, _name);

    // add initialization property and slot
    auto* init_prop = add_prop<any>(INITIALIZATION_PROPERTY );
    m_initialization_slot = add_slot(init_prop, SlotFlag_INPUT, 1);

    // add conditional-related properties and slots
    ASTSwitchBehavior::init(this, 2);
    ASTNodeProperty* condition_prop = ASTSwitchBehavior::condition_in(Branch_TRUE)->property;

    // add iteration property and slot
    auto iter_prop = add_prop<any>(ITERATION_PROPERTY );
    m_iteration_slot = add_slot(iter_prop, SlotFlag_INPUT, 1);
}