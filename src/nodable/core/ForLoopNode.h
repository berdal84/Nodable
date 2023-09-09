#pragma once

#include <memory>
#include "Token.h"
#include "Node.h" // base class
#include "IConditionalStruct.h" // interface

namespace ndbl
{
    // forward declarations
    class Scope;
    class InstructionNode;

    /**
     * @class Represent a conditional and iterative structure "for"
     * for( init_state, condition_expr, iterate_expr ) { ... }
     */
    class ForLoopNode
        : public Node
        , public IConditionalStruct {
    public:
        ForLoopNode();
        ForLoopNode(ForLoopNode&& other) = default;
        ForLoopNode& operator=(ForLoopNode&& other) = default;
        ~ForLoopNode() = default;

        Token token_for;
        PoolID<InstructionNode> init_instr;
        PoolID<InstructionNode> cond_instr;
        PoolID<InstructionNode> iter_instr;

        const Property* get_init_expr()const { return get_prop(INITIALIZATION_PROPERTY); }
        const Property* get_iter_expr()const { return get_prop(ITERATION_PROPERTY); }

        // implements IConditionalStruct (which is already documented)

        const Property* condition_property()const override { return get_prop(CONDITION_PROPERTY);}
        PoolID<Scope>  get_condition_true_scope()const override;;
        PoolID<Scope>  get_condition_false_scope()const override;;

        REFLECT_DERIVED_CLASS()
    };
}
