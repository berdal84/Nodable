#include "Graph.h"

#include <algorithm>    // std::find_if
#include "language/Nodlang.h"
#include "imgui_internal.h"
#include "ASTForLoop.h"
#include "ASTFunctionCall.h"
#include "ASTIf.h"
#include "ASTLiteral.h"
#include "ASTNode.h"
#include "ASTScope.h"
#include "ASTSlotLink.h"
#include "ASTUtils.h"
#include "ASTVariable.h"
#include "ASTVariableRef.h"
#include "ASTWhileLoop.h"

using namespace ndbl;
using namespace tools;

Graph::Graph()
: m_components(this)
{
    _init();
}

Graph::~Graph()
{
    _clear();
    m_components.shutdown();
    assert(m_node_registry.empty());
    assert(m_edge_registry.empty());
}

void Graph::_init()
{
    TOOLS_LOG(TOOLS_VERBOSE, "Graph", "Initializing ...\n");
    ASSERT( m_node_registry.empty() ); // Root must be first, registry should be empty

    // create and _insert root
    ASTNode* root  = ASTUtils::create_root_scope();
    this->_insert(root, nullptr);

    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "-- add root node %p (name: %s, class: %s)\n", root, root->name().c_str(), root->get_class()->name());
    ASSERT( root_node() == root );
    TOOLS_LOG(TOOLS_VERBOSE, "Graph", "Initialized " TOOLS_OK "\n");
}

void Graph::_clear()
{
    TOOLS_LOG(TOOLS_VERBOSE, "Graph", "Clearing ...\n");

    // delete from last to first (which is the root)
    while ( !m_node_registry.empty() )
    {
        auto last = m_node_registry.end() - 1;
        ASTNode* node = *last;
        _erase(last);
        _clean_node(node);
        node->shutdown();
        delete node;
    }

#ifdef NDBL_DEBUG
    if ( !m_edge_registry.empty() )
    {
        TOOLS_LOG(TOOLS_ERROR, "Graph", "m_edge_registry should be empty.\n" );
        TOOLS_LOG(TOOLS_MESSAGE, "Graph", "Dumping %zu edge(s) for debugging purpose ...\n", m_edge_registry.size() );
        for ( auto& edge : m_edge_registry)
        {
            TOOLS_LOG(TOOLS_MESSAGE, "Graph", "   %s\n", to_string(edge.second).c_str() );
        }
        m_edge_registry.clear();
    }
#endif

    assert(m_node_registry.empty());
    assert(m_edge_registry.empty());
    TOOLS_LOG(TOOLS_VERBOSE, "Graph", "Clear " TOOLS_OK "\n");
}

void Graph::reset()
{
	TOOLS_LOG(TOOLS_VERBOSE,  "Graph", "Resetting ...\n");

    this->_clear();
    this->_init();
    signal_reset.emit();

    TOOLS_LOG(TOOLS_VERBOSE, "Graph", "Reset " TOOLS_OK "\n");
}

bool Graph::update()
{
    bool _changed = false;

    std::stack<ASTNode*> node_to_delete;

    for(ASTNode* node : m_node_registry)
    {
        if ( node->has_flags(ASTNodeFlag_MUST_BE_DELETED))
        {
            node_to_delete.push(node);
        }
        else if ( node->has_flags(ASTNodeFlag_IS_DIRTY) )
        {
            _changed |= node->update();
        }
    }

    while( !node_to_delete.empty() )
    {
        _changed |= true;
        ASTNode* node = node_to_delete.top();
        _clean_node(node);
        node->shutdown();
        delete node;
        node_to_delete.pop();
    }

    if ( _changed )
    {
        signal_change.broadcast(); // TODO: rather be signal_update, signal_change is already emitted within function calls from this method
    }

    return _changed;
}

void Graph::_insert(ASTNode* node, ASTScope* scope)
{
    // do the inverse of Graph::_erase(ASTNode* node)

    if ( scope != nullptr )
    {
        VERIFY( !m_node_registry.empty(), "can't insert a scoped node first, a root node (with a nullptr scope) should be inserted before." );
        VERIFY( node->scope() == nullptr, "node must be unscoped, use scope argument instead" );
        VERIFY( scope->node()->graph() == this, "the provided scope belong to another graph" );
        assert(!node->has_flags(ASTNodeFlag_WAS_IN_A_SCOPE_ONCE)); // double-check
        scope->append(node);
    }
    else
    {
        VERIFY( m_node_registry.empty(), "you didn't provided a scope argument, which is only valid for the first insert (root node).");
    }

    node->m_graph = this;
	m_node_registry.push_back( node );

    signal_add_node.emit(node);
    signal_change.broadcast();

    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "-- add node %p (name: %s, class: %s)\n", node, node->name().c_str(), node->get_class()->name());
}

NodeRegistry::iterator Graph::_erase(NodeRegistry::iterator it)
{
    ASTNode* node = *it;
    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "-- node %p (name: \"%s\"): erasing ...\n", node, node->name().c_str() );
    const auto& next = m_node_registry.erase( it );
    signal_remove_node.emit(node);
    signal_change.broadcast();
    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "-- node %p (name: \"%s\"): _erased\n", node, node->name().c_str() );
    return next;
}

void Graph::_clean_node(ASTNode* node)
{
    ASSERT( node );
    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "-- node %p (name: \"%s\"): pre_erasing ...\n", node, node->name().c_str() );

    // Identify each edge connected to this node
    auto concerns_node = [&](const std::pair<SlotFlags, ASTSlotLink>& pair )
    {
        const ASTSlotLink& edge = pair.second;
        return edge.tail->node == node
               || edge.head->node == node;
    };
    auto edge_it = m_edge_registry.begin();
    while( edge_it != m_edge_registry.end() )
    {
        edge_it = std::find_if(edge_it, m_edge_registry.end(), concerns_node);
        if ( edge_it != m_edge_registry.end() )
            edge_it = _disconnect(edge_it);
    }

    // unset scope
    if ( node->scope() )
    {
        _reset_scope(node);
    }

    // transfer children to default scope
    if (ASTScope* _internal_scope = node->internal_scope() )
    {
        _transfer_children( _internal_scope, root_scope());
    }

    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "-- node %p (name: \"%s\"): pre__erased\n", node, node->name().c_str() );
}

ASTNode* Graph::create_scope(ASTScope* scope)
{
    auto* node = ASTUtils::create_scope();
    _insert(node, scope);
    return node;
}

ASTVariable* Graph::create_variable(const TypeDescriptor *_type, const std::string& _name, ASTScope* scope)
{
    ASTVariable* node = ASTUtils::create_variable(_type, _name);
    _insert(node, scope);
	return node;
}

ASTFunctionCall* Graph::create_function(const FunctionDescriptor& _type, ASTScope* scope)
{
    ASTFunctionCall* node = ASTUtils::create_function(_type, ASTNodeType_FUNCTION);
    _insert(node, scope);
    return node;
}

ASTFunctionCall* Graph::create_operator(const FunctionDescriptor& _type, ASTScope* scope)
{
    ASTFunctionCall* node = ASTUtils::create_function(_type, ASTNodeType_OPERATOR);
    _insert(node, scope);
    return node;
}

void Graph::find_and_destroy(ASTNode* node)
{
    if (!node)
        return;

    auto it = std::find(m_node_registry.begin(), m_node_registry.end(), node);
    ASSERT( it != m_node_registry.end() );

    // backup slots
    const ASTNodeSlot* flow_in  = node->flow_in();
    const ASTNodeSlot* flow_out = node->flow_out();
    const bool flow_can_be_maintained = flow_in->adjacent_count() == 1
                                     && flow_out->adjacent_count() == 1;
    ASTNodeSlot* prev_adjacent_slot = flow_in->first_adjacent();
    ASTNodeSlot* next_adjacent_slot = flow_out->first_adjacent();

    _clean_node(node); // flow_in/out will be cleared

    // try to maintain flow
    if ( flow_can_be_maintained )
    {
        connect(prev_adjacent_slot, next_adjacent_slot, GraphFlag_ALLOW_SIDE_EFFECTS );
    }

    _erase(it);
    node->shutdown();

    delete node;
}

ASTSlotLink Graph::connect_or_merge(ASTNodeSlot* tail, ASTNodeSlot* head )
{
    // Guards
    ASSERT(head->has_flags(SlotFlag_INPUT ) );
    ASSERT(head->has_flags(SlotFlag_NOT_FULL ) );
    ASSERT(tail->has_flags(SlotFlag_OUTPUT ) );
    ASSERT(tail->has_flags(SlotFlag_NOT_FULL ) );
    VERIFY(head->property, "tail property must be defined" );
    VERIFY(tail->property, "head property must be defined" );
    VERIFY(head->node != tail->node, "Can't connect same primary_child!" );

    // now graph is abstract
//    const type* out_type = __out->property->get_type();
//    const type* in_type  = _in->property->get_type();
//    EXPECT( type::is_implicitly_convertible( out_type, in_type ), "dependency type should be implicitly convertible to dependent type");

    // case 1: merge orphan slot
    if (tail->node == nullptr ) // if dependent is orphan
    {
        head->property->digest(tail->property );
        delete head->property;
        // set_dirty(); // no changes on edges/nodes
        return {};
    }

    // case 2: merge literals when not connected to a variable
    if (tail->node->type() == ASTNodeType_LITERAL && tail->property->token().word_len() < 16 )
        if (head->node->type() != ASTNodeType_VARIABLE )
        {
            head->property->digest(tail->property );
            find_and_destroy(tail->node);
            return {};
        }

    // Connect (case 4)
    return connect(tail, head, GraphFlag_ALLOW_SIDE_EFFECTS );
}

ASTSlotLink Graph::connect_to_variable(ASTNodeSlot* output_slot, ASTVariable* _variable )
{
    // Guards
    ASSERT( output_slot->has_flags(SlotFlag_OUTPUT | SlotFlag_NOT_FULL ) );
    return connect_or_merge( output_slot, _variable->value_in() );
}

void Graph::connect(const std::set<ASTNodeSlot*>& tails, ASTNodeSlot* head, GraphFlags _flags)
{
    if ( !tails.empty() )
        for (ASTNodeSlot* _tail : tails )
            connect(_tail, head, GraphFlag_ALLOW_SIDE_EFFECTS );
}

ASTSlotLink Graph::connect(ASTNodeSlot* tail, ASTNodeSlot* head, GraphFlags _flags)
{
    // Create and _insert edge
    auto it = m_edge_registry.emplace(tail->type(), ASTSlotLink{tail, head});
    ASTSlotLink& edge = it->second;


    // DirectedEdge is just data, we must add manually cross-references to each end of the edge
    edge.tail->add_adjacent( edge.head );
    edge.head->add_adjacent( edge.tail );

    // Handle side effects
    if (_flags & GraphFlag_ALLOW_SIDE_EFFECTS )
    {
        switch ( edge.type() )
        {
            case SlotFlag_TYPE_FLOW:
                _handle_connect_flow_side_effects(edge);  break;
            case SlotFlag_TYPE_VALUE:
                _handle_connect_value_side_effects(edge); break;
            default:
                ASSERT(false);// This connection type is not yet implemented
        }
    }

    signal_change.broadcast();

    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "Graph", "New edge added\n");

    return edge;
}

void Graph::_handle_connect_value_side_effects(const ASTSlotLink& edge )
{
    // ensure the tail node has the right scope
    // must be:
    // - unchanged in case of a node already part of the code flow
    // - or: head node's scope / internal scope if any
    if ( !edge.tail->node->has_flow_adjacent() )
    {
        ASTNode*  tail_node    = edge.tail->node;
        ASTNode*  head_node    = edge.head->node;
        ASTScope* target_scope = head_node->scope();

        if ( head_node->has_internal_scope() )
        {
            target_scope = head_node->internal_scope();
        }

        _change_scope(tail_node, target_scope);
    }

    // make sure head property type matches with tail, update head when needed.
    if ( edge.head->node->type() != ASTNodeType_VARIABLE )
    {
        const ASTNodeProperty* tail_prop = edge.tail->property;
        ASTNodeProperty* head_prop = edge.head->property;
        head_prop->set_type( tail_prop->get_type() );
    }
}

void Graph::_handle_disconnect_value_side_effects(const ASTSlotLink& edge )
{
    ASSERT( edge.tail->type_and_order() == SlotFlag_OUTPUT );

    // reset token to a default value to preserve a correct serialization
    if (edge.head->node->type() != ASTNodeType_VARIABLE )
    {
        ASTToken& tok = edge.head->property->token();
        std::string buf;
        get_language()->serialize_default_buffer(buf, tok.m_type);
        tok.word_replace( buf.c_str() );
    }
}

void Graph::_handle_disconnect_flow_side_effects(const ASTSlotLink& edge )
{
    ASSERT( edge.tail->type_and_order() == SlotFlag_FLOW_OUT );

    // Ensure disconnected node gets in the right scope
    //
    ASTScope* target_scope = root_scope();
    switch ( edge.head->adjacent_count())
    {
        case 0:
            break;
        case 1:
        {
            target_scope = edge.head->first_adjacent_node()->scope();
            break;
        }
        default: // 2+
        {
            // Find the lowest common ancestor of adjacent node(s)
            std::set<ASTScope*> scopes;
            for(ASTNodeSlot* _adjacent_slot : edge.head->adjacent() )
                scopes.insert(_adjacent_slot->node->scope());
            ASTScope* ancestor = ASTScope::lowest_common_ancestor(scopes);

            if ( ancestor != nullptr )
            {
                ASSERT( ancestor->parent() != nullptr );
                target_scope = ancestor->parent();
                ASSERT(false); // TODO: here we must create a flow edge from the ancestor's node to edge.head->node
            }
        }
    }
    _change_scope(edge.head->node, target_scope);
}

void Graph::_handle_connect_flow_side_effects(const ASTSlotLink& edge )
{
    ASSERT( edge.tail->type_and_order() == SlotFlag_FLOW_OUT );

    ASTNode*  previous_node      = edge.tail->node;
    ASTNode*  next_node          = edge.head->node;
    size_t    flow_in_edge_count = edge.head->adjacent_count();

    if ( flow_in_edge_count == 1)
    {
        if ( edge.tail->has_flags(SlotFlag_IS_INTERNAL) )
        {
            ASTScope* target_scope = previous_node->internal_scope();
            if ( ASTUtils::is_conditional(previous_node) )
            {
                if( !next_node->has_internal_scope() && !ASTUtils::is_connected_to_codeflow(next_node) )
                {
                    // insert a scope between target_scope and next_node
                    ASTNode* intermediate_node = create_scope(target_scope);
                    target_scope->reset_head(intermediate_node);
                    target_scope = intermediate_node->internal_scope();
                }
            }
            _change_scope(next_node, target_scope);
            target_scope->reset_head(next_node); // since slot has IS_BRANCH, this node must become the head
        }
        else
        {
            _change_scope(next_node, previous_node->scope());
        }
    }
    else if ( flow_in_edge_count > 1 )
    {
        // gather adjacent scopes
        std::set<ASTScope*> scopes;
        for(ASTNodeSlot* adjacent : edge.head->adjacent() )
            scopes.insert(adjacent->node->scope() );

        if (scopes.size() == 1 )
        {
            _change_scope(next_node, *scopes.begin());
        }
        else
        {
            ASTScope* target_scope = ASTScope::lowest_common_ancestor(scopes );
            if( ASTUtils::is_conditional(target_scope->node()) )
            {
                // We don't want to add a node in a conditional scope, we must pick the parent
                target_scope = target_scope->parent();
            }
            _change_scope(next_node, target_scope);
            // node: no need to branch_scope->reset_head(next_node) here, since when we have 2 flow in or more, we can't be the head
        }
    }
    else
    {
        VERIFY(false, "Unexpected edge count");
    }
}

EdgeRegistry::iterator Graph::disconnect(const ASTSlotLink& edge, GraphFlags flags)
{
    auto [range_begin, range_end] = m_edge_registry.equal_range( edge.type() & ~SlotFlag_TYPE_MASK);
    auto it = std::find_if(
            range_begin,
            range_end,
            [&](const auto& _pair) -> bool
            {
                return edge == _pair.second;
            });
    return _disconnect( it, flags );
}

EdgeRegistry::iterator Graph::_disconnect(EdgeRegistry::iterator it, GraphFlags flags)
{
    // find the edge to disconnect
    ASTSlotLink& _edge = it->second;
    SlotFlags    type  = it->first;

    // _erase it from the registry
    it = m_edge_registry.erase(it);

    // disconnect the slots
    _edge.tail->remove_adjacent(_edge.head);
    _edge.head->remove_adjacent(_edge.tail);

    // handle side effects
    if ( flags & GraphFlag_ALLOW_SIDE_EFFECTS )
    {
        switch ( type )
        {
            case SlotFlag_TYPE_FLOW:
            {
                _handle_disconnect_flow_side_effects(_edge);
                break;
            }
            case SlotFlag_TYPE_VALUE:
            {
                _handle_disconnect_value_side_effects(_edge);
                break;
            }
            default:
                VERIFY(!type, "Not yet implemented yet");
        }
    }

    signal_change.broadcast();
    return it;
}

ASTIf* Graph::create_cond_struct(ASTScope* scope)
{
    ASTIf* node = ASTUtils::create_cond_struct();
    _insert(node, scope);
    return node;
}

ASTForLoop* Graph::create_for_loop(ASTScope* scope)
{
    ASTForLoop* node = ASTUtils::create_for_loop();
    _insert(node, scope);
    return node;
}

ASTWhileLoop* Graph::create_while_loop(ASTScope* scope)
{
    ASTWhileLoop* ast_node = ASTUtils::create_while_loop();
    _insert(ast_node, scope);
    return ast_node;
}

ASTNode* Graph::create_node(ASTScope* scope)
{
    ASTNode* node = ASTUtils::create_node();
    _insert(node, scope);
    return node;
}

ASTLiteral* Graph::create_literal(const TypeDescriptor* _type, ASTScope* scope)
{
    ASTLiteral* node = ASTUtils::create_literal(_type);
    _insert(node, scope);
    return node;
}

ASTNode* Graph::create_node(CreateNodeType _type, const FunctionDescriptor* _signature, ASTScope* scope)
{
    switch ( _type )
    {
        /*
         * TODO: We could consider narowing the enum to few cases (BLOCK, VARIABLE, LITERAL, OPERATOR, FUNCTION)
         *       and rely more on _signature (ex: a bool variable could be simply "bool" or "bool bool(bool)")
         */
        case CreateNodeType_BLOCK_CONDITION:  return create_cond_struct(scope);
        case CreateNodeType_BLOCK_FOR_LOOP:   return create_for_loop(scope);
        case CreateNodeType_BLOCK_WHILE_LOOP: return create_while_loop(scope);
        case CreateNodeType_ROOT:
            reset(); return root_node();

        case CreateNodeType_VARIABLE_BOOLEAN: return create_variable_decl<bool>("b", scope);
        case CreateNodeType_VARIABLE_DOUBLE:  return create_variable_decl<double>("d", scope);
        case CreateNodeType_VARIABLE_INTEGER: return create_variable_decl<int>("i", scope);
        case CreateNodeType_VARIABLE_STRING:  return create_variable_decl<std::string>("str", scope);

        case CreateNodeType_LITERAL_BOOLEAN:  return create_literal<bool>(scope);
        case CreateNodeType_LITERAL_DOUBLE:   return create_literal<double>(scope);
        case CreateNodeType_LITERAL_INTEGER:  return create_literal<int>(scope);
        case CreateNodeType_LITERAL_STRING:   return create_literal<std::string>(scope);

        case CreateNodeType_FUNCTION:
        {
            VERIFY(_signature != nullptr, "_signature is expected when dealing with functions or operators");
            if ( get_language()->is_operator( _signature ) )
                return create_operator( *_signature, scope );
            return create_function( *_signature, scope );
        }
        default:
            VERIFY(false, "Unhandled CreateNodeType.");
            return nullptr;
    }
}

ASTVariableRef* Graph::create_variable_ref(ASTScope* scope)
{
    ASTVariableRef* node = ASTUtils::create_variable_ref();
    _insert(node, scope);
    return node;
}

ASTVariable* Graph::create_variable_decl(const TypeDescriptor* type, const char*  name, ASTScope* scope)
{
    // Create variable
    ASTVariable* var_node = create_variable(type, name, scope);
    var_node->set_flags(VariableFlag_DECLARED); // yes, when created from the graph view, variables can be undeclared (== no scope).
    ASTToken token(ASTToken_t::keyword_operator, " = ");
    token.word_move_begin(1);
    token.word_move_end(-1);
    var_node->set_operator_token( token );

    return var_node;
}

ASTNode *Graph::create_empty_instruction(ASTScope* scope)
{
    ASTNode* node = ASTUtils::create_empty_instruction();
    _insert(node, scope);
    return node;
}

std::set<ASTScope *> Graph::root_scopes()
{
    std::set<ASTScope*> result;
    for ( ASTNode* node : m_node_registry )
        if ( node->has_internal_scope() )
            if ( node->internal_scope()->depth() == 0 )
                result.insert( node->internal_scope() );
    return result;
}

std::vector<ASTScope *> Graph::scopes()
{
    std::vector<ASTScope *> result;
    for(ASTNode* node : m_node_registry)
        if ( node->scope() )
            result.push_back( node->scope() );
    return result;
}

void Graph::flag_node_to_delete(ASTNode *node, GraphFlags flags)
{
    ASSERT(node->graph() == this);

    if ( flags & GraphFlag_ALLOW_SIDE_EFFECTS )
    {
        // delete inputs when they share the same scope
        for ( auto input : node->inputs() )
            if ( node->scope() == input->scope() )
                flag_node_to_delete(input, flags);

        // delete children
        if ( ASTScope* scope = node->internal_scope() )
            for ( ASTNode* _child : scope->child() )
                flag_node_to_delete(_child, flags);
    }

    node->set_flags(ASTNodeFlag_MUST_BE_DELETED);
}

ASTScope* Graph::root_scope() const
{
    return root_node()->internal_scope();
}

bool Graph::contains(ASTNode* node) const
{
    return std::find( m_node_registry.begin(), m_node_registry.end(), node ) != m_node_registry.end();
}


void Graph::_change_scope(ASTNode* node, ASTScope* desired_scope)
{
    ASTScope* current_scope = node->scope();

    VERIFY( current_scope != nullptr, "node must be scoped to be changed for another scope");
    VERIFY( desired_scope != nullptr, "a non null desired scope is expected");

    if ( desired_scope == current_scope )
    {
        return;
    }

    current_scope->remove(node);
    desired_scope->append(node);

    signal_change_scope.emit(node, current_scope, desired_scope);
}

void Graph::_transfer_children(ASTScope* source, ASTScope* target)
{
    ASSERT(source);
    ASSERT(target);

    std::set<ASTNode*> child_copy{source->child()};
    for(ASTNode* _child : child_copy)
    {
        _change_scope(_child, target);
        ASSERT(_child->scope() == target);
    }

    ASSERT(source->empty());
}

void Graph::_reset_scope(ASTNode* node)
{
    ASSERT(node->scope());
    node->scope()->remove(node);
}
