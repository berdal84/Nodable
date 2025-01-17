#include "ASTScope.h"

#include <stack>
#include <cstring>
#include <algorithm> // for std::find_if

#include "tools/core/log.h"

#include "ASTForLoop.h"
#include "ASTIf.h"
#include "ASTVariable.h"
#include "ASTUtils.h"
#include "Graph.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTScope).extends<Component<ASTNode>>();
)

ASTScope::ASTScope()
: Component<ASTNode>("ASTScope")
{
    // Component::signal_init.connect<&ASTScope::on_init>(this);
    Component::signal_shutdown.connect<&ASTScope::_on_shutdown>(this);
    // Component::signal_name_change.connect<&ASTScope::_on_name_change>(this);
}

ASTScope::~ASTScope()
{
    // assert(Component::signal_init.disconnect<&ASTScope::on_init>(this));
    assert(Component::signal_shutdown.disconnect<&ASTScope::_on_shutdown>(this));
    // assert(Component::signal_name_change.disconnect<&ASTScope::_on_name_change>(this));
    assert(m_parent == nullptr);
    assert(m_head == nullptr);
    assert(m_child.empty());
    assert(m_variable.empty());
    assert(m_partition.empty());
}

void ASTScope::_on_shutdown()
{
    ASSERT(m_parent == nullptr); // Remove this scope from parent first

    // reset partitions (they will be shutdown individually by the ComponentBag)
    for(ASTScope* partition : m_partition )
    {
        partition->reset_parent(nullptr);
    }
    m_partition.clear();

    VERIFY( m_child.empty(), "Scope must be empty to shutdown, since nodes can't have a nullptr scope, Graph is responsible for it");
    reset_head();
}

ASTVariable* ASTScope::find_variable(const std::string& _identifier, ScopeFlags flags )
{
    // Try first to find in this scope
    for(ASTVariable* node : m_variable)
        if ( node->get_identifier() == _identifier )
            return node;

    // not found? => recursive call in parent ...
    if ( m_parent && flags & ScopeFlags_RECURSE_PARENT_SCOPES )
        return m_parent->find_variable(_identifier, flags);

    return nullptr;
}

void ASTScope::append(ASTNode *node)
{
    m_cached_backbone_dirty = true;

    const ASTScope* previous_scope = node->scope();
    ASSERT(node);
    VERIFY(previous_scope == nullptr, "Node should have no scope");
    VERIFY(node != this->node(), "Can't add a node into its own internal scope" );

    // Insert
    const auto& [_, ok] = m_child.insert(node); ASSERT(ok);

    // insert as variable?
    if (node->type() == ASTNodeType_VARIABLE )
    {
        auto variable_node = reinterpret_cast<ASTVariable*>( node );
        if (find_variable(variable_node->get_identifier()) != nullptr )
        {
            TOOLS_LOG(TOOLS_ERROR, "Scope", "Unable to append variable '%s', already exists in the same internal_scopeview.\n", variable_node->get_identifier().c_str());
            // we do not return, graph is abstract, it just won't compile ...
        }
        else if (variable_node->scope() )
        {
            TOOLS_LOG(TOOLS_ERROR, "Scope", "Unable to append variable '%s', already declared in another internal_scopeview. Remove it first.\n", variable_node->get_identifier().c_str());
            // we do not return, graph is abstract, it just won't compile ...
        }
        else
        {
            TOOLS_LOG(TOOLS_VERBOSE, "Scope", "Add '%s' variable to the internal_scopeview\n", variable_node->get_identifier().c_str() );
            m_variable.insert(variable_node);
        }
    }

    // Insert inputs recursively
    for ( ASTNode* input : node->inputs() )
        if ( input->type() != ASTNodeType_VARIABLE ) // variables must be manually added
            if (input->scope() == previous_scope )
                append(input);

    node->reset_scope(this);
}

std::vector<ASTNode*> ASTScope::leaves()
{
    std::vector<ASTNode*> result;
    _leaves_ex(result);
    if ( result.empty() && node() != nullptr )
        result.push_back( node() );
    return result;
}

std::vector<ASTNode*>& ASTScope::_leaves_ex(std::vector<ASTNode*>& out)
{
    if ( !m_partition.empty() )
    {
        for( ASTScope* partition : m_partition )
            partition->_leaves_ex(out);
        return out; // when a scope as sub scopes, we do not consider its node as potential leaves since they are usually secondary nodes, so we return early.
    }

    ASTNode* node = m_head;
    while( node != nullptr )
    {
        if (node->has_internal_scope() )
        {
            node->internal_scope()->_leaves_ex(out);
        }

        auto outputs = node->flow_outputs();
        if ( outputs.empty() )
        {
            out.push_back( node );
            node = nullptr;
        }
        else
        {
            ASSERT(outputs.size() == 1); // Should happen?
            node = outputs.front();
        }
    }

    return out;
}

void ASTScope::remove(ndbl::ASTNode *node)
{
    ASSERT( node );
    ASSERT( node->scope() == this); // node can't be inside its own Scope

    m_cached_backbone_dirty = true;

    // inputs first
    for ( ASTNode* input : node->inputs() )
        if ( input->scope() == this )
            if ( input->type() != ASTNodeType_VARIABLE ) // variables must be manually removed
                remove(input);

    // erase node + side effects
    m_child.erase( node );
    if (m_head == node )
    {
        reset_head();
    }
    node->reset_scope(nullptr);

    if ( node->type() == ASTNodeType_VARIABLE )
    {
        m_variable.erase( reinterpret_cast<ASTVariable*>(node) );
    }

    ASSERT( node->scope() == nullptr);
}

bool ASTScope::empty(ScopeFlags flags) const
{
    bool is_empty = m_child.empty();

    if (flags & ScopeFlags_RECURSE_CHILD_PARTITION )
        for( const ASTScope* partition : m_partition )
            is_empty &= partition->empty(flags);

    return is_empty;
}

std::stack<ASTScope*> get_path(ASTScope* s)
{
    std::stack<ASTScope*> path;
    path.push(s);
    while( path.top() != nullptr )
    {
        path.push( path.top()->parent() );
    }
    return path;
}

ASTScope* ASTScope::lowest_common_ancestor(const std::set<ASTScope*>& scopes)
{
    ASTScope* lca_scope = nullptr;
    for( ASTScope* curr_scope : scopes )
    {
        lca_scope = lca_scope ? lowest_common_ancestor( lca_scope, curr_scope )
                              : curr_scope;
    }
    return lca_scope;
}

ASTScope* ASTScope::lowest_common_ancestor(ASTScope* s1, ASTScope* s2)
{
    if ( s1 == s2 )
    {
        return s1;
    }

    std::stack<ASTScope*> path1 = get_path(s1);
    std::stack<ASTScope*> path2 = get_path(s2);

    ASTScope* common = nullptr;
    while( !path1.empty() && !path2.empty() && path1.top() == path2.top() )
    {
        common = path1.top();
        path1.pop();
        path2.pop();
    }

    return common;
}

std::set<ASTScope*>& ASTScope::get_descendent_ex(std::set<ASTScope*>& out, ASTScope* scope, size_t level_max, ScopeFlags flags)
{
    if ( flags & ScopeFlags_INCLUDE_SELF )
    {
        out.insert( scope );
    }

    if ( level_max-1 == 0 )
        return out;

    for ( ASTScope* partition : scope->m_partition )
    {
        out.insert( partition );
        get_descendent_ex(out, partition, level_max - 1 );
    }

    ASTNode* node = scope->m_head;
    while( node != nullptr )
    {
        if ( ASTScope* internal_scope = node->internal_scope() )
        {
            out.insert( internal_scope );
            get_descendent_ex(out, internal_scope, level_max - 1, ScopeFlags_INCLUDE_SELF );
        }

        auto& outputs = node->flow_outputs();
        if ( !outputs.empty() )
        {
            ASSERT(outputs.size() == 1);
            node = outputs.front();
        }
        else
        {
            node = nullptr;
        }
    }

    return out;
}

void ASTScope::reset_parent(ASTScope* new_parent)
{
    VERIFY(new_parent != m_parent, "new_parent is expected to be different than the current one");
    m_parent = new_parent;
    _set_depth_cache_dirty();
}

void ASTScope::_set_depth_cache_dirty()
{
    m_cached_depth_dirty = true;

    // recurse
    for(ASTNode* child : m_child)
        if (ASTScope* child_scope = child->internal_scope() )
            child_scope->_set_depth_cache_dirty();
}

bool ASTScope::contains(ASTNode* node) const
{
    return m_child.contains( node );
}

void ASTScope::reset_head(ASTNode* node)
{
#ifdef TOOLS_DEBUG
    VERIFY( !node || node->scope() == this, "Node must be from this scope");
    VERIFY(!m_head || m_head->scope() == this, "node as backbone head should never be removed before to reset backbone head")
#endif
    m_head = node;
}

void ASTScope::_update_depth_cache()
{
    if ( !m_cached_depth_dirty )
        return;

    m_cached_depth = m_parent ? m_parent->depth() + 1 : 0;
    m_cached_depth_dirty = false;
}

void ASTScope::_update_backbone_cache()
{
    if ( !m_cached_backbone_dirty )
        return;

    m_cached_backbone.clear();
    ASTNode* curr_node = m_head;
    while( curr_node != nullptr && curr_node->scope() == this )
    {
        // add current
        m_cached_backbone.push_back(curr_node );

        // get next
        ASSERT( curr_node->flow_out()->capacity() == 1 );
        ASTNodeSlot* out = curr_node->flow_out();
        curr_node = out->first_adjacent_node();
    }
    m_cached_backbone_dirty = false;
}
