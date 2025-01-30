#pragma once

#include "ASTNode.h"
#include "ASTSwitchBehavior.h"
#include "ASTToken.h"
#include <memory>
#include <utility>

namespace ndbl
{

    /**
     * @class Represent a conditional structure with two branches ( IF/ELSE )
     * @note Multiple ConditionalNode can be chained to form an IF / ELSE IF / ... / ELSE.
     */
    class ASTIf : public ASTNode, public ASTSwitchBehavior
    {
    public:
        DECLARE_REFLECT_override

        ASTToken token_if   = {ASTToken_t::keyword_if};
        ASTToken token_else = {ASTToken_t::ignore};

        void init(const std::string& _name);
    };
}
