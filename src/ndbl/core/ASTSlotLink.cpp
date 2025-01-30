#include "ASTSlotLink.h"

#include <utility>
#include "ASTNodeSlot.h"
#include "ASTNode.h"

using namespace ndbl;

ASTSlotLink::ASTSlotLink(ASTNodeSlot* _tail, ASTNodeSlot* _head )
: tail(_tail)
, head(_head)
{
    // Guards
    ASSERT(tail);
    ASSERT(head);
    VERIFY(tail->type() == head->type(), "Slot types are incompatible"  );
    VERIFY(tail->node != head->node    , "Can't connect two slots from the same node");

    if ( tail->order() == SlotFlag_ORDER_2ND ) // Make sure tail is always FIRST ORDER
        std::swap(tail, head);

    ASSERT(tail->flags() & SlotFlag_ORDER_1ST );
    ASSERT(head->flags() & SlotFlag_ORDER_2ND );
    ASSERT(tail->node->graph() != nullptr);
    ASSERT(head->node->graph() != nullptr);
    VERIFY(tail->node->graph() == head->node->graph(),"The slots are from Nodes from different graphs" );
}

std::string ndbl::to_string(const ASTSlotLink& _edge)
{
    std::string result;
    result.reserve(64);

    auto serialize_slot_ref = [&result](const ASTNodeSlot* _slot) -> void
    {
        result.append("[node: ");
        result.append( std::to_string((u64_t) _slot->node));
        result.append(" (slot: ");
        result.append( std::to_string((u64_t)_slot));

        switch (_slot->flags() )
        {
            case SlotFlag_INPUT:   result.append(", INPUT");  break;
            case SlotFlag_OUTPUT:  result.append(", OUTPUT"); break;
        }

        result.append(")]");
    };

    serialize_slot_ref(_edge.tail);

    // TODO: enable reflection on SLotFlag_XXX
    switch ( _edge.tail->type() )
    {
        case SlotFlag_TYPE_VALUE:
            result.append(" >==(VALUE)==> ");
            break;
        case SlotFlag_TYPE_FLOW:
            result.append(" >==(CODEFLOW)==> ");
            break;
        default:
            ASSERT(false); // unhandled type?
    }

    serialize_slot_ref(_edge.head);

    return std::move(result);
}

