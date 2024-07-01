#pragma once
#include "Node.h"
#include "Scope.h"
#include "Slot.h"

namespace ndbl
{
    typedef size_t Branch;
    enum Branch_ : size_t
    {
        Branch_FALSE   = 0,
        Branch_TRUE    = 1,
    };

    /**
     * Adds a conditional behavior to a given Node
     * @tparam BRANCH_COUNT the possible branch count (ex: for an if we have 2 branches, for a switch it can be N+1)
     */
    template<size_t BRANCH_COUNT>
    class TConditional
    {
    public:
        static_assert( BRANCH_COUNT > 1, "Branch count should be strictly greater than 1");

        void          init(Node* node);
        Scope*        get_scope_at(Branch _branch) const;
        Slot&         get_child_slot_at(Branch _branch);
        const Slot&   get_child_slot_at(Branch _branch) const;
        Node*         get_condition(Branch)const;
        const Slot&   get_condition_slot(Branch) const;
        Slot&         get_condition_slot(Branch);

    private:
        Node* m_node = nullptr;
        std::array<Slot*, BRANCH_COUNT>    m_next_slot;
        std::array<Slot*, BRANCH_COUNT>    m_child_slot;
        std::array<Slot*, BRANCH_COUNT-1>  m_condition_slot; // branch_FALSE has no condition
    };

    template<size_t BRANCH_COUNT>
    Slot& TConditional<BRANCH_COUNT>::get_condition_slot(Branch _branch )
    {
        return const_cast<Slot&>( const_cast<const TConditional<BRANCH_COUNT>*>(this)->get_condition_slot(_branch) );
    }

    template<size_t BRANCH_COUNT>
    const Slot& TConditional<BRANCH_COUNT>::get_condition_slot(Branch _branch ) const
    {
        EXPECT( _branch != Branch_FALSE, "Branch_FALSE has_flags no condition, use Branch_TRUE or any number greater than 0" )
        EXPECT( _branch < BRANCH_COUNT, "branch does not exist" )
        return *m_condition_slot.at(_branch - 1);
    }

    template<size_t BRANCH_COUNT>
    Node* TConditional<BRANCH_COUNT>::get_condition(Branch _branch ) const
    {
        return get_condition_slot(_branch).first_adjacent()->get_node();
    }

    template<size_t BRANCH_COUNT>
    void TConditional<BRANCH_COUNT>::init(Node* node)
    {
        static_assert( BRANCH_COUNT == 2, "Currently only implemented for 2 branches" );
        ASSERT(node != nullptr)
        m_node = node;
        m_node->add_slot( SlotFlag_OUTPUT, Slot::MAX_CAPACITY );

        // A default NEXT branch exists.
        m_next_slot[Branch_FALSE] = m_node->find_slot(SlotFlag_NEXT );
        m_next_slot[Branch_TRUE]  = m_node->add_slot(SlotFlag_NEXT, 1, Branch_TRUE );

        // No condition needed for the first slot
        auto condition_property = m_node->add_prop<Node*>(CONDITION_PROPERTY);
        m_condition_slot[0]     = m_node->add_slot(SlotFlag::SlotFlag_INPUT, 1, condition_property );

        m_child_slot[Branch_FALSE] = m_node->add_slot(SlotFlag_CHILD, 1, Branch_FALSE );
        m_child_slot[Branch_TRUE]  = m_node->add_slot(SlotFlag_CHILD, 1, Branch_TRUE );

    }

    template<size_t BRANCH_COUNT>
    const Slot& TConditional<BRANCH_COUNT>::get_child_slot_at(Branch _branch ) const
    {
        ASSERT(_branch < BRANCH_COUNT )
        return *m_child_slot[_branch];
    }

    template<size_t BRANCH_COUNT>
    Scope* TConditional<BRANCH_COUNT>::get_scope_at(Branch _branch ) const
    {
        ASSERT(_branch < BRANCH_COUNT )
        if ( Slot* adjacent_slot = m_child_slot[_branch]->first_adjacent() )
        {
            return adjacent_slot->get_node()->get_component<Scope>();
        }
        return {};
    }

    template<size_t BRANCH_COUNT>
    Slot& TConditional<BRANCH_COUNT>::get_child_slot_at(Branch _branch )
    {
        return const_cast<Slot&>(const_cast<const TConditional<BRANCH_COUNT>*>(this)->get_child_slot_at(_branch));
    }
}