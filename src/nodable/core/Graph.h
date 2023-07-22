#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory> // std::shared_ptr

#include "nodable/core/NodeFactory.h"
#include "nodable/core/WhileLoopNode.h"
#include "fw/core/reflection/class.h"
#include "fw/core/reflection/reflection"
#include "fw/core/types.h"

#include "IScope.h"

namespace ndbl
{
    // forward declarations
    class Nodlang;
    class NodeFactory;
    enum class Edge_t;

    /**
     * @brief To manage a graph (nodes and edges)
     */
	class Graph
	{
	public:
        using EdgeRegistry_t = std::multimap<Edge_t, const DirectedEdge*>; // edge storage (by edge type)

 		explicit Graph(const Nodlang*, const NodeFactory*, const bool* _autocompletion);
		~Graph();

        UpdateResult                update();                                                   // Delete/Update nodes if necessary.

        // node related

        Node*                       create_root();                                              // Create a new root instruction in this graph.
        InstructionNode*            create_instr();                                             // Create a new instruction in this graph.
		VariableNode*				create_variable(const fw::type *_type, const std::string &_name, IScope *_scope);     // Create a new variable of a given type, identifier and scope (use create_scope()) in this graph.
		LiteralNode*                create_literal(const fw::type *_type);                                   // Create a new literal of a given type.
        template<typename T>
        LiteralNode*                create_literal() { return create_literal(fw::type::get<T>()); }
		Node*                       create_abstract_function(const fw::func_type*, bool _is_operator = false); // Create and append a new abstract (without known implementation)  function of a given type.
		Node*                       create_function(const fw::iinvokable*, bool _is_operator = false); // Create a new (invokable) function.
        Node*                       create_abstract_operator(const fw::func_type*);             // Create a new abstract (without known implementation) operator.
        Node*                       create_operator(const fw::iinvokable*);                     // Create a new operator.
        Node*                       create_scope();                                             // Create a new scope.
        ConditionalStructNode*      create_cond_struct();                                       // Create a new conditional structure.
        ForLoopNode*                create_for_loop();                                          // Create a new for loop (iterative structure).
        WhileLoopNode*              create_while_loop();
        Node*                       create_node();                                              // Create a new node (simplest possible).
        void                        destroy(Node*);                                             // Delete a given node after disconnecting all its edges/wires.
        void                        ensure_has_root();                                          // If necessary, create a root node.
        Node*                       get_root()const { return m_root; }                          // Get root node.
        bool                        is_empty() const;                                           // Check if graph has a root (orphan nodes are ignored).
        bool                        is_dirty() const { return m_is_dirty; }
        void                        set_dirty(bool value = true) { m_is_dirty = value; }
        void                        clear();                                                     // Delete all nodes, wires, edges and reset scope.
        const std::vector<Node*>&   get_node_registry()const {return m_node_registry;}           // Get the node registry.
        EdgeRegistry_t&             get_edge_registry() {return m_edge_registry;}   

        // edge related

        const DirectedEdge*         connect(DirectedEdge _edge, bool _side_effects = true);     // Create a new directed edge, side effects can be turned off.
        void                        disconnect(const DirectedEdge*, bool _side_effects = true); // Disconnect a directed edge, side effects can be turned off.
        const DirectedEdge*         connect(Property* _source_property, Property * _target_property );                  // Create an edge between a source and a destination property (source and destination must be different).
        const DirectedEdge*         connect(Node* _src, InstructionNode* _dst);                 // Connect a given node to an instruction node. The node will be the root expression of that instruction.
        const DirectedEdge*         connect(Property * _src, VariableNode* _dst);               // Connect a node's property to a given variable. This property will be the initial value of the variable.
        std::vector<const DirectedEdge *> filter_edges(Property *_property, Way _way) const;    // Filter all the edges connected to a given property in certain way.

    private:
        // register management
        void                        add(Node*);                                                  // Add a given node to the registry.
        void                        remove(Node*);                                               // Remove a given node from the registry.
        void                        add(DirectedEdge*);                                          // Add a given edge to the registry.
        void                        remove(DirectedEdge*);                                       // Remove a given edge from the registry.

        bool                     m_is_dirty;
		std::vector<Node*>       m_node_registry;       // registry to store all the nodes from this graph.
        EdgeRegistry_t           m_edge_registry;       // registry ot all the edges (directed edges) between the registered nodes' properties.
		const Nodlang*           m_language;
		Node*                    m_root;                // Graph root (main scope), without it a graph cannot be compiled.
		const NodeFactory*       m_factory;             // Node factory (can be headless or not depending on the context: app, unit tests, cli).
        const bool*              m_autocompletion;      // Abandoned idea about autocompleting graph.
    };
}