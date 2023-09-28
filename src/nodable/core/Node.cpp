#include "Node.h"
#include "ForLoopNode.h"
#include "core/InstructionNode.h"
#include "core/Scope.h"

#include <utility>
#include <algorithm> // for std::find

#include "Graph.h"
#include "GraphUtil.h"
#include "InvokableComponent.h"

using namespace ndbl;
using fw::Pool;

REGISTER
{
    using namespace ndbl;
    fw::registration::push_class<Node>("Node");
}

Node::Node(std::string _label)
: name(std::move(_label))
, dirty(true)
, flagged_to_delete(false)
{
}

Node::Node( Node&& other )
: name(std::move(other.name))
, dirty(other.dirty)
, flagged_to_delete(other.flagged_to_delete)
, props(std::move(other.props))
, slots(std::move(other.slots))
, m_components(std::move(other.m_components))
, m_id( other.m_id )
{
}

Node& Node::operator=( Node&& other )
{
   name              = std::move(other.name);
   dirty             = other.dirty;
   flagged_to_delete = other.flagged_to_delete;
   props             = std::move(other.props);
   slots             = std::move(other.slots);
   m_components      = std::move(other.m_components);
   m_id              = std::move(other.m_id);

   return *this;
}

void Node::init()
{
    FW_EXPECT(slots.size() == 0, "Slots should not exist prior to call init()");

    // Add a property acting like a "this" for the owner Node.
    ID<Property> this_property_id = add_prop<PoolID<Node>>( THIS_PROPERTY );
    FW_ASSERT(this_property_id == THIS_PROPERTY_ID);

    get_prop_at( this_property_id )->set(m_id);

    add_slot( this_property_id, SlotFlag_PREV, 0 );
    add_slot( this_property_id, SlotFlag_NEXT, 0 );

    add_slot( this_property_id, SlotFlag_PARENT, 1);
    add_slot( this_property_id, SlotFlag_CHILD, SLOT_MAX_CAPACITY );

    add_slot( this_property_id, SlotFlag_OUTPUT, SLOT_MAX_CAPACITY );

    m_components.set_owner( m_id );
}

size_t Node::adjacent_count(SlotFlags _flags )const
{
    return filter_adjacent_slots( _flags ).size();
}

const fw::iinvokable* Node::get_connected_invokable(const char* property_name) const
{
    const Slot&   slot          = *find_slot( property_name, SlotFlag_INPUT );
    const SlotRef adjacent_slot = slot.first_adjacent();

    if ( adjacent_slot != SlotRef::null )
    {
        if ( auto* invokable = adjacent_slot.node->get_component<InvokableComponent>().get() )
        {
            return invokable->get_function();
        }
    }
    return nullptr;
}

bool Node::has_edge_heading(const char* _name) const
{
    ID<Property> id = props.find_id_from_name( _name );
    return has_edge_heading( id );
}

bool Node::has_edge_heading(ID<Property> property_id) const
{
    const Slot* slot = find_slot( property_id, SlotFlag_ORDER_SECOND );
    return slot && !slot->adjacent.empty();
}

void Node::set_name(const char *_label)
{
    name = _label;
    on_name_change.emit(m_id);
}

std::vector<PoolID<Component>> Node::get_components()
{
    return m_components.get_all();
}

const Slot* Node::find_slot(const char* property_name, SlotFlags desired_way) const
{
    const Property* property = get_prop(property_name);
    return find_slot( property->id, desired_way );
}

Slot* Node::find_slot(const char* property_name, SlotFlags _flags)
{
    const Property* property = get_prop(property_name);
    return find_slot( property->id, _flags );
}

const Slot* Node::find_slot(ID<Property> property_id, SlotFlags _flags) const
{
    return slots.find_by_property( property_id, _flags );
}

Slot *Node::find_slot(ID<Property> property_id, SlotFlags _flags)
{
    return slots.find_by_property( property_id, _flags );
}

std::vector<Slot *> Node::get_slots(const std::vector<ID<Property>>& properties, SlotFlags flags) const
{
    std::vector<Slot*> result;
    for(ID<Property> prop : properties)
    {
        if (const Slot* slot = slots.find_by_property( prop, flags ))
        {
            result.push_back(const_cast<Slot*>( slot ));
        }
    }
    return std::move(result);
}

const Property* Node::get_prop(const char *_name) const
{
    return props.find_by_name( _name );
}

Property* Node::get_prop(const char *_name)
{
    return props.find_by_name( _name );
}

const Property* Node::get_prop_at(ID<Property> id) const
{
    return props.at(id);
}

Property* Node::get_prop_at(ID<Property> id)
{
    return props.at(id);
}

Slot* Node::find_slot(SlotFlags _flags)
{
    return find_slot( THIS_PROPERTY, _flags );
}

Slot& Node::get_slot(ID8<Slot> id)
{
    return slots[id];
}

std::vector<PoolID<Node>> Node::outputs() const
{
    return filter_adjacent(SlotFlag_OUTPUT);
}

std::vector<PoolID<Node>> Node::inputs() const
{
    return filter_adjacent(SlotFlag_INPUT);
}

std::vector<PoolID<Node>> Node::predecessors() const
{
    return filter_adjacent(SlotFlag_PREV);
}

std::vector<PoolID<Node>> Node::rchildren() const
{
    auto v = children();
    std::reverse( v.begin(), v.end() );
    return v;
}

std::vector<PoolID<Node>> Node::children() const
{
    return filter_adjacent(SlotFlag_CHILD);
}

std::vector<PoolID<Node>> Node::successors() const
{
    return filter_adjacent(SlotFlag_NEXT);
}

std::vector<PoolID<Node>> Node::filter_adjacent( SlotFlags _flags ) const
{
    return GraphUtil::get_adjacent_nodes(this, _flags);
}

size_t Node::get_slot_count( SlotFlags _flags ) const
{
    return slots.count( _flags );
}

Slot* Node::get_first_slot(SlotFlags flags, const fw::type* _type)
{
    FW_EXPECT( (flags & SlotFlag_ORDER_MASK ) == flags, "Only compatible with SlotFlag_ACCEPTS_XXXX")
    for(Slot* slot : slots.filter( flags ) )
    {
        if( slot->get_property()->get_type()->equals( _type ) )
        {
            return slot;
        }
    }
    return nullptr;
}

Slot & Node::find_nth_slot( u8_t _n, SlotFlags _flags )
{
    u8_t count = 0;
    for( auto& slot : slots.data() )
    {
        if ( (slot.flags & _flags) == _flags)
        {
            if( count == _n )
            {
                return slot;
            }
            count++;
        }
    }
    FW_EXPECT(false, "Not found")
}

void Node::set_slot_capacity( SlotFlags _way, u8_t _n )
{
    Slot* slot = find_slot( THIS_PROPERTY, _way );
    FW_ASSERT(slot != nullptr)
    slot->set_capacity( _n );
}

ID<Property> Node::add_prop(const fw::type *_type, const char *_name, PropertyFlags _flags)
{
    return props.add(_type, _name, _flags);
}

ID8<Slot> Node::add_slot(ID<Property> _prop_id, SlotFlags _flags, u8_t _capacity)
{
    return slots.add( m_id, _prop_id, _flags, _capacity );
}

PoolID<Node> Node::get_parent() const
{
    const Slot* slot = find_slot( THIS_PROPERTY, SlotFlag_PARENT );
    return slot->first_adjacent().node;
}

Node* Node::last_child()
{
    auto _children = children();
    if( _children.empty() )
    {
        return nullptr;
    }
    return  _children.back().get();
}

std::vector<SlotRef> Node::filter_adjacent_slots( SlotFlags _flags ) const
{
    std::vector<SlotRef> result;
    for(const Slot* slot : slots.filter(_flags))
    {
        std::copy(slot->adjacent.begin(), slot->adjacent.end(), result.end());
    }
    return result;
}

std::vector<Slot*> Node::filter_slots( SlotFlags _flags) const
{
    return slots.filter(_flags);
}

bool Node::has_input_connected( const ID<Property>& id ) const
{
    const Slot* slot = find_slot( id, SlotFlag_INPUT );
    return slot && slot->adjacent_count() > 0;
}

std::vector<Slot*> Node::get_all_slots( ID<Property> _id ) const
{
    std::vector<Slot*> result;
    for(auto& slot : slots.data())
    {
        if( slot.property == _id )
        {
            result.push_back( const_cast<Slot*>(&slot) );
        }
    }
    return result;
}
