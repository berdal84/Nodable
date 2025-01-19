#pragma once
#include <string>
#include "ASTNodeSlot.h"

namespace ndbl
{
    struct ASTSlotLink
    {
        ASTNodeSlot* tail;
        ASTNodeSlot* head;

        ASTSlotLink(): tail(nullptr), head(nullptr) {};
        ASTSlotLink(ASTNodeSlot* tail, ASTNodeSlot* head);
        ASTSlotLink(const ASTSlotLink&) = default;

        ASTSlotLink& operator=(const ASTSlotLink& other) { tail = other.tail; head = other.head; return *this;}
                      operator bool () const { return tail != nullptr && head != nullptr; }
        bool          operator!=( const ASTSlotLink& other ) const { return !(*this == other); }
        bool          operator==( const ASTSlotLink &other ) const { return tail == other.tail && head == other.head; }
        SlotFlags     type() const { return tail->type(); /* both tail and head share the same type */ }
    };

    std::string to_string(const ASTSlotLink& _slot);
}

