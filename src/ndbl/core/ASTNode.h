#pragma once

#include <string>
#include <memory>
#include <algorithm>

#include "tools/core/assertions.h"
#include "tools/core/reflection/reflection"
#include "tools/core/types.h"
#include "tools/core/Component.h"

#include "constants.h"
#include "ASTSlotLink.h"
#include "ASTNodeProperty.h"
#include "ASTNodeSlot.h"
#include "ASTNodeType.h"

namespace ndbl
{
    // forward declarations
    class Graph;
    class ASTNodeFactory;
    class ASTScope;

    typedef int ASTNodeFlags;
    enum ASTNodeFlag_
    {
        ASTNodeFlag_NONE                = 0,
        ASTNodeFlag_IS_DIRTY            = 1 << 0,
        ASTNodeFlag_WAS_IN_A_SCOPE_ONCE = 1 << 1,
        ASTNodeFlag_MUST_BE_DELETED          = 1 << 2,
        ASTNodeFlag_ALL                 = ~ASTNodeFlag_NONE,
        ASTNodeFlag_DEFAULT             = ASTNodeFlag_NONE,
    };

    class ASTNode
	{
    public:
        DECLARE_REFLECT_virtual
        friend class ASTScope;
        friend class Graph;
        friend class ASTNodeFactory;
//===== CONSTRUCTORS/DESTRUCTORS =======================================================================================
    public:
        ASTNode(): m_component_collection(this), m_adjacent_nodes_cache(this) {};
        virtual ~ASTNode();
//===== SIGNALS ========================================================================================================
        tools::SimpleSignal                     signal_shutdown; // emit once shutdown() has been called
        tools::Signal<void(const std::string&)> signal_name_change;
//===== COMMON METHODS =================================================================================================
        void                        init(ASTNodeType type, const std::string& name);
        void                        shutdown();
        const std::string&          name() const { return m_name; }
        void                        set_name(const std::string& name) { m_name = name; signal_name_change.emit(name); }
        bool                        update();
        ASTNodeType                 type() const { return m_type; }
        bool                        is_invokable() const;
        bool                        is_expression() const;
        bool                        has_flags(ASTNodeFlags flags)const { return (m_flags & flags) == flags; };
        void                        set_flags(ASTNodeFlags flags) { m_flags |= flags; }
        void                        clear_flags(ASTNodeFlags flags = ASTNodeFlag_ALL) { m_flags &= ~flags; }
        Graph*                      graph() { return m_graph; }
        const Graph*                graph() const { return m_graph; }
        ASTToken&                   suffix() { return m_suffix; };
        const ASTToken&             suffix() const { return m_suffix; };
        void                        set_suffix(const ASTToken& token);
        bool                        is_orphan() const { return m_parent_scope == nullptr; }
        ASTScope*                   scope() const { return m_parent_scope; };
        bool                        has_scope() const { return m_parent_scope != nullptr; }
        void                        init_internal_scope();
        bool                        has_internal_scope() const { return m_internal_scope != nullptr; }
        ASTScope*                   internal_scope() const { return m_internal_scope; }
    protected:
        void                        reset_scope(ASTScope*);
//===== SLOT RELATED METHODS ===========================================================================================
    public:
        ASTNodeSlot*                value_in();
        const ASTNodeSlot*          value_in() const;
        ASTNodeSlot*                value_out();
        const ASTNodeSlot*          value_out() const;
        ASTNodeSlot*                flow_in();
        const ASTNodeSlot*          flow_in() const;
        ASTNodeSlot*                flow_out();
        const ASTNodeSlot*          flow_out() const;
        ASTNodeSlot*                flow_enter();
        const ASTNodeSlot*          flow_enter() const;
        ASTNodeSlot*                add_slot(ASTNodeProperty *, SlotFlags, size_t _capacity, size_t _position = 0);
        size_t                      adjacent_slot_count(SlotFlags flags)const { return filter_adjacent_slots(flags).size(); }
        ASTNodeSlot*                slot_at(size_t pos) { return m_slots.at(pos); }
        const ASTNodeSlot*          slot_at(size_t pos) const { return m_slots.at(pos); }
        std::vector<ASTNodeSlot*>   filter_slots(SlotFlags) const;
        std::vector<ASTNodeSlot*>   filter_slots(const std::function<bool(const ASTNodeSlot*)>& predicate) const;
        std::vector<ASTNodeSlot*>   filter_adjacent_slots(SlotFlags) const;
        ASTNodeSlot*                find_slot(SlotFlags flags) { return find_slot_by_property(m_value, flags ); }// implicitly DEFAULT_PROPERTY's slot
        const ASTNodeSlot*          find_slot(SlotFlags flags) const { return find_slot_by_property(m_value, flags ); }// implicitly DEFAULT_PROPERTY's slot
        ASTNodeSlot*                find_slot_at(SlotFlags flags, size_t pos ) { return const_cast<ASTNodeSlot*>( const_cast<const ASTNode*>(this)->find_slot_at(flags, pos)); } // implicitly DEFAULT_PROPERTY's slot
        const ASTNodeSlot*          find_slot_at(SlotFlags, size_t _position ) const; // implicitly DEFAULT_PROPERTY's slot
        ASTNodeSlot*                find_slot_by_property_name(const char* name, SlotFlags flags) { return const_cast<ASTNodeSlot*>( const_cast<const ASTNode*>(this)->find_slot_by_property_name(name, flags) ); };
        const ASTNodeSlot*          find_slot_by_property_name(const char* name, SlotFlags ) const;
        ASTNodeSlot*                find_slot_by_property_type(SlotFlags _way, const tools::TypeDescriptor *_type) const;
        ASTNodeSlot*                find_slot_by_property(const ASTNodeProperty* prop, SlotFlags flags ) { return const_cast<ASTNodeSlot*>( const_cast<const ASTNode*>( this )->find_slot_by_property(prop, flags ) ); }
        const ASTNodeSlot*          find_slot_by_property(const ASTNodeProperty*, SlotFlags ) const;
        ASTNodeSlot*                find_adjacent_at(SlotFlags, size_t _index ) const;
        size_t                      slot_count(SlotFlags flags) const { return filter_slots( flags ).size(); }
        std::vector<ASTNodeSlot*>&  slots() { return m_slots; }
        const std::vector<ASTNodeSlot*>& slots() const { return m_slots; }
        bool                        has_flow_adjacent() const;
        const std::vector<ASTNode*>& inputs() const       { return m_adjacent_nodes_cache.get(SlotFlag_INPUT); }
        const std::vector<ASTNode*>& outputs() const      { return m_adjacent_nodes_cache.get(SlotFlag_OUTPUT); }
        const std::vector<ASTNode*>& flow_inputs() const  { return m_adjacent_nodes_cache.get(SlotFlag_FLOW_IN); }
        const std::vector<ASTNode*>& flow_outputs() const { return m_adjacent_nodes_cache.get(SlotFlag_FLOW_OUT); }
    protected:
        void                        _handle_slot_change(ASTNodeSlot::Event event, ASTNodeSlot *slot);
//===== PROPERTY RELATED METHODS =======================================================================================
    public:
        const ASTNodeProperty*  value() const { return m_value; }
        ASTNodeProperty*        value() { return m_value; }
        std::vector<ASTNodeProperty*>& props() { return m_properties; }
        const std::vector<ASTNodeProperty*>& props() const { return m_properties; }
        ASTNodeProperty*        add_prop(const tools::TypeDescriptor*, const char* name, PropertyFlags = PropertyFlag_NONE);
        ASTNodeProperty*        get_prop(const char* _name) { return find_prop_by_name( _name ); }
        const ASTNodeProperty*  get_prop(const char* _name) const { return find_prop_by_name( _name ); }
        const tools::FunctionDescriptor* get_connected_function_type(const char *property_name) const; //
        bool                    has_input_connected( const ASTNodeProperty*) const;
        bool                    has_prop(const char*) const;
        ASTNodeProperty*        prop_at(size_t pos) { return m_properties.at(pos); }
        const ASTNodeProperty*  prop_at(size_t pos ) const { return m_properties.at(pos); }
        ASTNodeProperty*        find_prop_by_name(const char* name) { return const_cast<ASTNodeProperty*>( const_cast<const ASTNode*>(this)->find_prop_by_name(name) );}
        const ASTNodeProperty*  find_prop_by_name(const char* name) const;
        ASTNodeProperty*        find_first_prop(PropertyFlags flags, const tools::TypeDescriptor* type ) { return const_cast<ASTNodeProperty*>( const_cast<const ASTNode*>(this)->find_first_prop(flags, type) );}
        const ASTNodeProperty*  find_first_prop(PropertyFlags, const tools::TypeDescriptor* ) const;
        ASTNodeProperty*        get_this_property() { return m_properties.at(self_property_index); }
        const ASTNodeProperty*  get_this_property() const { return m_properties.at(self_property_index); }
        template<typename T>
        ASTNodeProperty*        add_prop(const char* name, PropertyFlags flags = PropertyFlag_NONE ) { return add_prop(tools::type::get<T>(), name, flags); }
//===== COMPONENT RELATED METHODS ======================================================================================
    public:
        template<class T> T*                component() const  { return m_component_collection.get<T>(); }
        tools::ComponentBag<ASTNode>*       components()       { return &m_component_collection; }
        const tools::ComponentBag<ASTNode>* components() const { return &m_component_collection; }
//===== MEMBERS ========================================================================================================
    private:
        struct AdjacentNodesCache
        {
            explicit AdjacentNodesCache(const ASTNode* node): _node(node) {}
            const std::vector<ASTNode*>& get(SlotFlags) const;
            void set_dirty() { _cache.clear(); }
        private:
            const ASTNode* _node;
            std::unordered_map<SlotFlags, std::vector<ASTNode*>> _cache;
        };

        static constexpr size_t self_property_index = 0; //

        std::string            m_name;
        ASTToken               m_suffix = ASTToken{};
        Graph*                 m_graph  = nullptr;
        ASTNodeType            m_type   = ASTNodeType_DEFAULT;
        ASTNodeFlags           m_flags  = ASTNodeFlag_IS_DIRTY;
        ASTNodeProperty*       m_value  = nullptr; // Short had for prop_at( self_property_index )
        ASTScope*              m_parent_scope   = nullptr;
        ASTScope*              m_internal_scope = nullptr;
        AdjacentNodesCache     m_adjacent_nodes_cache;
        std::vector<ASTNodeSlot*>                                m_slots;
        std::unordered_map<const ASTNodeProperty*, std::vector<ASTNodeSlot*>>   m_slots_by_property; // TODO: use multimap
        std::vector<ASTNodeProperty*>                            m_properties;
        std::map<std::string, ASTNodeProperty*>                  m_properties_by_name;
        tools::ComponentBag<ASTNode>                             m_component_collection;
    };
}
