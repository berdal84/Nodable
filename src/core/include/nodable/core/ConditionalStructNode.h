#pragma once

#include <nodable/core/memory.h>
#include <nodable/core/Token.h>
#include <nodable/core/Node.h>
#include <nodable/core/IScope.h>
#include <nodable/core/IConditionalStruct.h>

namespace ndbl
{

    /**
     * @brief Class to represent a conditional structure ( IF/ELSE )
     */
    class ConditionalStructNode
            : public Node
            , public IConditionalStruct
    {
    public:
        ConditionalStructNode();
        ~ConditionalStructNode() = default;

        inline void            set_token_if(s_ptr<Token> token) { m_token_if = token; }
        inline void            set_token_else(s_ptr<Token> token) { m_token_else = token; }
        inline s_ptr<const Token> get_token_if()const   { return m_token_if; }
        inline s_ptr<const Token> get_token_else()const { return m_token_else; }

        // override AbstractConditionalStruct
        Scope*        get_condition_true_scope()const override;
        Scope*        get_condition_false_scope()const override;
        Member*       condition_member()const override { return m_props.get(k_condition_member_name); }
        void          set_cond_instr(InstructionNode*) override;
        bool          has_elseif() const;
        InstructionNode* get_cond_instr()const override { return m_cond_instr_node; }

    private:
        s_ptr<Token> m_token_if;
        s_ptr<Token> m_token_else;
        InstructionNode*       m_cond_instr_node;

        REFLECT_DERIVED_CLASS(Node, IConditionalStruct)

    };
}