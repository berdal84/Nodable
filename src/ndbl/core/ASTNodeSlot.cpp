#include "ASTNodeSlot.h"
#include "ASTNode.h"

using namespace ndbl;

const ASTNodeSlot ASTNodeSlot::null{};

ASTNodeSlot::ASTNodeSlot()
: node(nullptr)
, _flags(SlotFlag_NONE)
, property(nullptr)
, position(0)
{
}

ASTNodeSlot::ASTNodeSlot(
        ASTNode*     node,
        SlotFlags flags,
        ASTNodeProperty* property,
        size_t    capacity,
        size_t    position
    )
: node(node)
, _flags(flags)
, property(property)
, position(position)
{
    VERIFY(!has_flags(SlotFlag_NOT_FULL), "SlotFlag_NOT_FULL is for readonly use" );
    ASSERT( capacity > 0 );
    _adjacent.reserve(capacity);
    _flags |= SlotFlag_NOT_FULL;
}

ASTNodeSlot::ASTNodeSlot(const ASTNodeSlot &other)
: node(other.node)
, property(other.property)
, _flags(other._flags)
, _adjacent(other._adjacent)
, position(other.position)
{
    expand_capacity(other.capacity());
}

ASTNodeSlot* ASTNodeSlot::adjacent_at(u8_t pos) const
{
    u8_t count{0};
    for (auto& each : _adjacent )
    {
        if( count == pos )
        {
            return each;
        }
        ++count;
    }
    return nullptr;
}

void ASTNodeSlot::add_adjacent(ASTNodeSlot* other)
{
    ASSERT(other != nullptr);
    VERIFY(other != this, "Reflexive edge not handled" );
    VERIFY(type() == other->type() , "Slot must have common type" );
    VERIFY(_adjacent.size() < _adjacent.capacity(), "Slot is full" );
    _adjacent.emplace_back( other );
    if (_adjacent.size() == _adjacent.capacity() )
    {
        _flags &= ~SlotFlag_NOT_FULL; // Make sure IS_NOT_FULL is 0
    }
    signal_change.emit(Event_Add, other);
}

bool ASTNodeSlot::remove_adjacent(ASTNodeSlot* other)
{
    auto it = std::find(_adjacent.begin(), _adjacent.end(), other);
    if( it == _adjacent.end())
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "ASTNodeSlot", "remove_adjacent(Slot*) - slot not found");
        return false;
    }
    _adjacent.erase(it );
    _flags |= SlotFlag_NOT_FULL;
    signal_change.emit(Event_Remove, other);
    return true;
}

void ASTNodeSlot::expand_capacity(size_t capacity )
{
    VERIFY(_adjacent.capacity() <= capacity, "New capacity must be strictly greater than current" );
    _adjacent.reserve(capacity);
    _flags |= SlotFlag_NOT_FULL;
}

