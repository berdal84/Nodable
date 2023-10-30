#include "WhileLoopNode.h"
#include "core/Scope.h"
#include "InstructionNode.h"
#include "GraphUtil.h"

using namespace ndbl;

REGISTER
{
    fw::registration::push_class<WhileLoopNode>("WhileLoopNode")
        .extends<Node>()
        .extends<IConditionalStruct>();
}

void WhileLoopNode::init()
{
    Node::init();

    auto cond_id = add_prop<PoolID<Node>>( CONDITION_PROPERTY ); // while ( <here> ) { ... }
    add_slot( SlotFlag_INPUT, 1, cond_id );

    add_slot( SlotFlag_PREV, SLOT_MAX_CAPACITY, m_this_property_id );
    add_slot( SlotFlag_PARENT, 1, m_this_property_id );

    m_next_slot[Branch_FALSE]  = add_slot( SlotFlag_NEXT, 1, Branch_FALSE );
    m_next_slot[Branch_TRUE]   = add_slot( SlotFlag_NEXT, 1, Branch_TRUE );
    m_child_slot[Branch_FALSE] = add_slot( SlotFlag_CHILD, 1 , Branch_FALSE );
    m_child_slot[Branch_TRUE]  = add_slot( SlotFlag_CHILD, 1 , Branch_TRUE );
}

PoolID<Scope> WhileLoopNode::get_scope_at(size_t _pos) const
{
    const SlotRef adjacent = get_child_slot_at(_pos).first_adjacent();
    if ( adjacent )
    {
        return adjacent->get_node()->get_component<Scope>();
    }
    return {};
}


Slot& WhileLoopNode::get_child_slot_at( size_t _pos )
{
    return get_slot_at( m_child_slot.at(_pos) );
}


const Slot& WhileLoopNode::get_child_slot_at( size_t _pos ) const
{
    return get_slot_at( m_child_slot.at(_pos) );
}