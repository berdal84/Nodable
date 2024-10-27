#pragma once

#include "Node.h"
#include "SwitchBehavior.h"
#include "Token.h"
#include <memory>
#include <utility>

namespace ndbl
{

    /**
     * @class Represent a conditional structure with two branches ( IF/ELSE )
     * @note Multiple ConditionalNode can be chained to form an IF / ELSE IF / ... / ELSE.
     */
    class IfNode : public Node, public SwitchBehavior
    {
    public:
        Token token_if   = {Token_t::keyword_if};
        Token token_else = {Token_t::keyword_else};

        void init(const std::string& _name);

        REFLECT_DERIVED_CLASS()
    };
}
