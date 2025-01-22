#include "ASTVariable.h"
#include "ASTNodeProperty.h"
#include "ASTScope.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTVariable).extends<ASTNode>();
)

void ASTVariable::init(const tools::TypeDescriptor* _type, const char* _identifier)
{
    // Init node
    ASTNode::init(ASTNodeType_VARIABLE, "Variable");

    // Init identifier property
    value()->set_type(_type);
    value()->set_token({ASTToken_t::identifier});
    value()->token().word_replace(_identifier); // might come from std::string::c_str()

    // Init Slots
    add_slot(value(), SlotFlag_INPUT, 1); // to connect an initialization expression
    add_slot(value(), SlotFlag_FLOW_OUT, 1);
    add_slot(value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY);

    m_as_declaration_slot = add_slot(value(), SlotFlag_OUTPUT, 1); // as declaration
    m_as_reference_slot   = add_slot(value(), SlotFlag_OUTPUT, ASTNodeSlot::MAX_CAPACITY ); // as reference
}
