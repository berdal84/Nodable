#pragma once
#include "ASTNode.h"
#include "ASTScope.h"
#include "ASTNodeSlot.h"

namespace ndbl
{
    typedef size_t Branch;
    enum Branch_ : size_t
    {
        Branch_FALSE = 0,
        Branch_TRUE  = 1,
    };

    /**
     * Adds a conditional behavior to a given Node.
     */
    class ASTSwitchBehavior
    {
    public:
        static constexpr size_t BRANCH_MAX = 2;

        void                 init(ASTNode* node, size_t branch_count);
        ASTNodeSlot*         branch_out(Branch branch)                       { ASSERT(branch < m_branch_count); return m_branch_slot[branch]; }
        const ASTNodeSlot*   branch_out(Branch branch) const                 { ASSERT(branch < m_branch_count); return m_branch_slot[branch]; }
        size_t               branch_count() const                            { return m_branch_count; }
        const ASTNode*       condition(Branch branch = Branch_TRUE) const    { ASSERT(Branch_FALSE < branch && branch < m_branch_count); return m_condition_in[branch - 1]->first_adjacent_node(); }
        ASTNode*             condition(Branch branch = Branch_TRUE)          { ASSERT(Branch_FALSE < branch && branch < m_branch_count); return m_condition_in[branch - 1]->first_adjacent_node(); }
        const ASTNodeSlot*   condition_in(Branch branch = Branch_TRUE) const { ASSERT(Branch_FALSE < branch && branch < m_branch_count); return m_condition_in[branch - 1]; }
        ASTNodeSlot*         condition_in(Branch branch = Branch_TRUE)       { ASSERT(Branch_FALSE < branch && branch < m_branch_count); return m_condition_in[branch - 1]; }
    private:

        size_t                                    m_branch_count = 0;
        std::array<ASTNodeSlot*, BRANCH_MAX>      m_branch_slot;
        std::array<ASTNodeSlot*, BRANCH_MAX - 1>  m_condition_in; // branch_FALSE has no condition
    };
}
