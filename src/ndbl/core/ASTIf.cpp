#include "ASTUtils.h"
#include "ASTIf.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTIf).extends<ASTNode>();
)

void ASTIf::init(const std::string&_name)
{
    ASTNode::init(ASTNodeType_IF_ELSE, _name);
    ASTSwitchBehavior::init(this, 2);
}
// required to link static code above