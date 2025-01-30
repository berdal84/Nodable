#include "ASTLiteral.h"
#include "ASTNodeSlot.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTLiteral).extends<ASTNode>();
)

void ASTLiteral::init(const TypeDescriptor* _type, const std::string& _name)
{
    ASTNode::init(ASTNodeType_LITERAL, _name);
    value()->set_type(_type);

    add_slot(value(), SlotFlag_FLOW_OUT , 1);
    add_slot(value(), SlotFlag_FLOW_IN  , ASTNodeSlot::MAX_CAPACITY);
    add_slot(value(), SlotFlag_OUTPUT   , 1);
}

