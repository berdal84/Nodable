#pragma once

#include "ASTSwitchBehavior.h"
#include "ASTToken.h"
#include <memory>

namespace ndbl
{
    // forward declarations
    class ASTScope;
    class InstructionNode;

    /**
     * @class Represent a conditional and iterative structure "for"
     * for( <init_ex>; <condition>; <iteration> ) { ... }
     */
    class ASTForLoop : public ASTNode, public ASTSwitchBehavior
    {
    public:
        DECLARE_REFLECT_override
        ASTToken token_for = {ASTToken_t::keyword_for };

        void        init(const std::string& _name);
        ASTNodeSlot*       iteration_slot()            { ASSERT(m_iteration_slot); return m_iteration_slot; }
        ASTNodeSlot*       initialization_slot()       { ASSERT(m_initialization_slot); return m_initialization_slot; }
        const ASTNodeSlot* iteration_slot() const      { ASSERT(m_iteration_slot);return m_iteration_slot;}
        const ASTNodeSlot* initialization_slot() const { ASSERT(m_initialization_slot);return m_initialization_slot;}
    private:
        ASTNodeSlot*       m_initialization_slot{nullptr};
        ASTNodeSlot*       m_iteration_slot{nullptr};
    };
}
