#pragma once

#include <map>
#include <string>
#include <vector>
#include <queue>

#include "tools/core/Component.h"
#include "ndbl/core/ASTNode.h"
#include "ndbl/core/ASTNodeFactory.h"
#include "ndbl/core/ASTWhileLoop.h"
#include "tools/core/reflection/reflection"
#include "tools/core/types.h"

namespace ndbl
{
    // forward declarations
    class Nodlang;
    class ASTNodeFactory;
    class GraphView;
    class ASTVariableRef;

    typedef int GraphFlags;
    enum GraphFlag_
    {
        GraphFlag_NONE               = 0,
        GraphFlag_ALLOW_SIDE_EFFECTS = 1 << 0,
    };

    enum CreateNodeType
    {
        CreateNodeType_ROOT,
        CreateNodeType_BLOCK_CONDITION,
        CreateNodeType_BLOCK_FOR_LOOP,
        CreateNodeType_BLOCK_WHILE_LOOP,
        CreateNodeType_BLOCK_SCOPE,
        CreateNodeType_VARIABLE_BOOLEAN,
        CreateNodeType_VARIABLE_DOUBLE,
        CreateNodeType_VARIABLE_INTEGER,
        CreateNodeType_VARIABLE_STRING,
        CreateNodeType_LITERAL_BOOLEAN,
        CreateNodeType_LITERAL_DOUBLE,
        CreateNodeType_LITERAL_INTEGER,
        CreateNodeType_LITERAL_STRING,
        CreateNodeType_FUNCTION,
    };

    typedef std::vector<ASTNode*>                  NodeRegistry;
    typedef std::multimap<SlotFlags , ASTSlotLink> EdgeRegistry;

    /**
     * @brief To manage a graph (primary_child and edges)
     */
	class Graph
	{
	public:
 		Graph(ASTNodeFactory*);
		~Graph();
//====== Data =======================================================================================================
    public:
        tools::SimpleSignal           signal_reset;
        tools::SimpleBroadcastSignal  signal_change;
        tools::Signal<void(ASTNode*)> signal_add_node;
        tools::Signal<void(ASTNode*)> signal_remove_node;
        using ScopeChanged = void(ASTNode*, ASTScope* /* old_scope */, ASTScope* /* new_scope */ ) ;
        tools::Signal<ScopeChanged>   signal_change_scope;
        tools::SimpleSignal           signal_is_complete; // user defined, usually when parser or user is done
    private:
        const ASTNodeFactory*         m_factory{};
        NodeRegistry                  m_node_registry;
        EdgeRegistry                  m_edge_registry;
        tools::ComponentBag<Graph>    m_components;
//====== Common Methods ================================================================================================
    public:
        bool                     update();
        void                     reset();  // Delete all nodes, wires, edges and reset scope.
        bool                     is_empty() const { return root_scope()->empty(); };
        ASTNode*                 root_node() const { return m_node_registry.front(); /* we have the guarantee it exists, see constructor */}
        ASTScope*                root_scope() const;
        template<class T> T*              component() const  { return m_components.get<T>(); }
        tools::ComponentBag<Graph>*       components()       { return &m_components; }
        const tools::ComponentBag<Graph>* components() const { return &m_components; }
    private:
        void                    _init();
        void                    _clear();
//====== Node(s) Related ===============================================================================================
    public:
        ASTNode*                 create_node() { return create_node( this->root_scope() ); }
        ASTNode*                 create_node(ASTScope*); // Create a raw node.
        ASTNode*                 create_node(CreateNodeType type, const tools::FunctionDescriptor* desc = nullptr) { return create_node(type, desc, this->root_scope()); }
        ASTNode*                 create_node(CreateNodeType, const tools::FunctionDescriptor*, ASTScope*);
        ASTVariable*             create_variable(const tools::TypeDescriptor* type, const std::string& name) { return create_variable(type, name, this->root_scope()); }
        ASTVariable*             create_variable(const tools::TypeDescriptor* type, const std::string& name, ASTScope* scope );
        ASTVariableRef*          create_variable_ref() { return create_variable_ref( this->root_scope()); }
        ASTVariableRef*          create_variable_ref(ASTScope*);
        ASTVariable*             create_variable_decl(const tools::TypeDescriptor* type, const char* name) { return create_variable_decl(type, name, this->root_scope()); }
        ASTVariable*             create_variable_decl(const tools::TypeDescriptor* _type, const char* _name, ASTScope*);
        ASTLiteral*              create_literal(const tools::TypeDescriptor* type) { return create_literal(type, this->root_scope()); }
        ASTLiteral*              create_literal(const tools::TypeDescriptor *_type, ASTScope*);
        ASTFunctionCall*         create_function(const tools::FunctionDescriptor& desc) { return create_function(desc, this->root_scope()); }
        ASTFunctionCall*         create_function(const tools::FunctionDescriptor&, ASTScope*);
        ASTFunctionCall*         create_operator(const tools::FunctionDescriptor& desc) { return create_operator(desc, this->root_scope()); }
        ASTFunctionCall*         create_operator(const tools::FunctionDescriptor&, ASTScope*);
        ASTIf*                   create_cond_struct() { return create_cond_struct(root_scope()); }
        ASTIf*                   create_cond_struct(ASTScope*);
        ASTForLoop*              create_for_loop() { return create_for_loop(root_scope()); }
        ASTForLoop*              create_for_loop(ASTScope*);
        ASTWhileLoop*            create_while_loop() { return create_while_loop(root_scope()); }
        ASTWhileLoop*            create_while_loop(ASTScope*);
        ASTNode*                 create_empty_instruction() { return create_empty_instruction(root_scope()); }
        ASTNode*                 create_empty_instruction(ASTScope*);
        ASTNode*                 create_scope(ASTScope* scope);
        void                     find_and_destroy(ASTNode* node);
        std::vector<ASTScope *>  scopes();
        std::set<ASTScope *>     root_scopes();
        NodeRegistry&            nodes() {return m_node_registry;}
        const NodeRegistry&      nodes()const {return m_node_registry;}
        void                     flag_node_to_delete(ASTNode* node, GraphFlags = GraphFlag_NONE);
        bool                     contains(ASTNode*) const;

        template<typename T> ASTVariable* create_variable_decl(const char* name = "var")          { return create_variable_decl( tools::type::get<T>(), name, this->root_scope()); }
        template<typename T> ASTVariable* create_variable_decl(const char* name, ASTScope* scope ){ return create_variable_decl( tools::type::get<T>(), name, scope); }
        template<typename T> ASTLiteral*  create_literal()                 { return create_literal( tools::type::get<T>(), this->root_scope() ); }
        template<typename T> ASTLiteral*  create_literal(ASTScope* scope ) { return create_literal( tools::type::get<T>(), scope ); }
    private:
        void                    _insert(ASTNode*, ASTScope*);
        NodeRegistry::iterator  _erase(NodeRegistry::iterator);
        void                    _clean_node(ASTNode* node);

        void                    _reset_scope(ASTNode* scoped_node);
        void                    _transfer_children(ASTScope* from, ASTScope* to);
        void                    _change_scope(ASTNode *node, ASTScope* desired_scope);
//====== Edge(s) Related ===============================================================================================
    public:
        ASTSlotLink             connect(ASTNodeSlot* tail, ASTNodeSlot* head, GraphFlags = GraphFlag_NONE );
        void                    connect(const std::set<ASTNodeSlot*>& tails, ASTNodeSlot* head, GraphFlags _flags);
        ASTSlotLink             connect_to_variable(ASTNodeSlot* output_slot, ASTVariable* variable );
        ASTSlotLink             connect_or_merge(ASTNodeSlot* tail, ASTNodeSlot* head);
        EdgeRegistry::iterator  disconnect(const ASTSlotLink&, GraphFlags = GraphFlag_NONE);
        const EdgeRegistry&     edges() const { return m_edge_registry; }
    private:
        void                    _handle_disconnect_value_side_effects(const ASTSlotLink&);
        void                    _handle_disconnect_flow_side_effects(const ASTSlotLink&);
        void                    _handle_connect_value_side_effects(const ASTSlotLink&);
        void                    _handle_connect_flow_side_effects(const ASTSlotLink&);
        EdgeRegistry::iterator  _disconnect(EdgeRegistry::iterator, GraphFlags = GraphFlag_NONE );
    };
}