#include "ASTWhileLoop.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTWhileLoop).extends<ASTNode>();
)

void ASTWhileLoop::init(const std::string &_name)
{
    ASTNode::init(ASTNodeType_WHILE_LOOP, _name);
    ASTSwitchBehavior::init(this, 2);
}
