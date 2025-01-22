#pragma once

#include <memory>
#include "tools/core/reflection/reflection"

#include "ASTSwitchBehavior.h"// base class
#include "ASTToken.h"

namespace ndbl
{
    /**
     * @class Represent a conditional and iterative structure "while"
     * while( condition_expr ) {
     *   // do something
     * }
     */
    class ASTWhileLoop : public ASTNode, public ASTSwitchBehavior
    {
    public:
        DECLARE_REFLECT_override

        ASTToken token_while = {ASTToken_t::keyword_while};

        void init(const std::string& _name);
    };
}
