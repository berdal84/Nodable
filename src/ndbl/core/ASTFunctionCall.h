#pragma once
#include <memory>

#include "tools/core/types.h"
#include "tools/core/reflection/reflection"
#include "ASTNode.h"

namespace ndbl
{
	class ASTFunctionCall : public ASTNode
    {
	public:
        DECLARE_REFLECT_override

        void                        init(ASTNodeType node_type, const tools::FunctionDescriptor& func_type);
        ASTNodeSlot*                       get_arg_slot(size_t i) const { return m_argument_slot[i]; }
        const std::vector<ASTNodeSlot*>&   get_arg_slots() const { return m_argument_slot; }
		const tools::FunctionDescriptor& get_func_type()const { return m_func_type; }
        const ASTToken&                get_identifier_token() const { return m_identifier_token; }
        void                        set_identifier_token(const ASTToken& tok) { m_identifier_token = tok; }
        ASTNodeSlot*                       lvalue_in() const { return m_argument_slot[0]; }
        ASTNodeSlot*                       rvalue_in() const { return m_argument_slot[1]; }
    protected:
        ASTToken                       m_identifier_token = {ASTToken_t::identifier };
        tools::FunctionDescriptor   m_func_type; // not owned
        std::vector<ASTNodeSlot*>          m_argument_slot;
        std::vector<ASTNodeProperty*>      m_argument_props;
    };
}
