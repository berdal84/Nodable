#pragma once
#include "ASTNodeSlotFlag.h"
#include "tools/core/Signals.h"
#include <vector>

namespace ndbl
{
    // Forward declaration
    class ASTNode;
    class ASTNodeProperty;
    class ASTNodeView;
    struct ASTNodeSlotView;

    struct ASTNodeSlot
    {
        typedef std::vector<ASTNodeSlot*> Slots;

        ASTNodeSlot();
        ASTNodeSlot(const ASTNodeSlot& other);
        ASTNodeSlot(
                ASTNode*     owner,
                SlotFlags flags,
                ASTNodeProperty* prop = nullptr,
                size_t    capacity = 1,
                size_t    position = 0
        );

        enum Event
        {
            Event_Add,
            Event_Remove
        };

        ASTNodeSlot* adjacent_at(u8_t) const;
        size_t       adjacent_count() const{return _adjacent.size();}
        const Slots  adjacent() const{return _adjacent;}
        ASTNode*     first_adjacent_node() const { return !_adjacent.empty() ? _adjacent[0]->node : nullptr; }
        ASTNodeSlot* first_adjacent() const { return !_adjacent.empty() ? _adjacent[0] : nullptr; }
        void         expand_capacity(size_t _capacity);
        SlotFlags    flags() const { return _flags; }
        void         set_flags( SlotFlags flags){_flags |= flags;}
        bool         has_flags( SlotFlags flags ) const{return (_flags & flags) == flags;}
        SlotFlags    type() const{return _flags & SlotFlag_TYPE_MASK;}
        SlotFlags    type_and_order() const { return _flags & (SlotFlag_TYPE_MASK | SlotFlag_ORDER_MASK); }
        SlotFlags    order() const{return _flags & SlotFlag_ORDER_MASK;}
        bool         empty() const{return _adjacent.empty();}
        size_t       capacity() const{return _adjacent.capacity(); }
        bool         is_full() const {return !has_flags(SlotFlag_NOT_FULL);}
        void         add_adjacent(ASTNodeSlot*);
        bool         remove_adjacent(ASTNodeSlot*);

        tools::Signal<void(Event, ASTNodeSlot*)> signal_change;
        const size_t     position; // In case multiple Slot exists for the same type and order, we distinguish them with their position.
        ASTNode* const   node; // parent node
        ASTNodeProperty* const  property; // parent node's property
        SlotFlags        _flags = SlotFlag_NONE;
        ASTNodeSlotView* view = nullptr;
        Slots            _adjacent;

        static constexpr size_t MAX_CAPACITY = 8;
        static const     ASTNodeSlot   null;
    };
}
