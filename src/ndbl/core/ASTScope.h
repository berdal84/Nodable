#pragma once

#include <vector>
#include <set>
#include "tools/core/Component.h"
#include "ASTToken.h"
#include "ASTNode.h"

namespace ndbl
{
    // forward decl.
    class ASTNode;
    class ASTVariable;
    class ASTScopeView;

    typedef int ScopeFlags;
    enum ScopeFlags_
    {
        ScopeFlags_NONE                    = 0,
        ScopeFlags_RECURSE_PARENT_SCOPES   = 1 << 0,
        ScopeFlags_RECURSE_CHILD_PARTITION = 1 << 1,
        ScopeFlags_INCLUDE_SELF            = 1 << 2,
    };

    class ASTScope : public tools::Component<ASTNode>
    {
//== Data ==============================================================================================================
    public:
        ASTToken                  token_begin = {ASTToken_t::ignore};
        ASTToken                  token_end   = {ASTToken_t::ignore};
    private:
        ASTScopeView*             m_view = nullptr;
        std::set<ASTNode*>        m_child;
        ASTNode*                  m_head = nullptr; // backbone's start
        std::vector<ASTNode*>     m_cached_backbone;
        bool                      m_cached_backbone_dirty = true;
        size_t                    m_cached_depth = 0;
        bool                      m_cached_depth_dirty = true;
        std::set<ASTVariable*>    m_variable;
        std::vector<ASTScope*>    m_partition;
        ASTScope*                 m_parent = nullptr;
//== Methods ===========================================================================================================
    public:
        ASTScope();
        ~ASTScope() override;

        void                           append(ASTNode*);
        void                           remove(ASTNode*);
        bool                           contains(ASTNode* node) const;
        ASTScope*                      parent() { return m_parent; }
        const ASTScope*                parent() const { return m_parent; }
        std::vector<ASTNode*>          leaves();
        bool                           empty(ScopeFlags = ScopeFlags_NONE) const;
        ASTVariable*                   find_variable(const std::string& identifier, ScopeFlags = ScopeFlags_RECURSE_PARENT_SCOPES);
        ASTScopeView*                  view() const { return m_view; } // TODO: remove this, use components
        void                           set_view(ASTScopeView* view) { m_view = view; } // TODO: remove this, use components
        const std::set<ASTVariable*>&  variable()const { return m_variable; };
        const std::set<ASTNode*>&      child() const { return m_child; }
        ASTNode*                       head() const { return m_head; }
        void                           reset_head(ASTNode* node = nullptr);
        const std::vector<ASTNode*>&   backbone() const { const_cast<ASTScope*>(this)->_update_backbone_cache(); return m_cached_backbone; } // backbone is a vector of nodes starting from the scope's head including all flow_outputs in this scope (recursively)
        void                           reset_parent(ASTScope* new_parent = nullptr);
        bool                           is_orphan() const { return m_parent == nullptr; }
        size_t                         depth() const { const_cast<ASTScope*>(this)->_update_depth_cache(); return m_cached_depth; };
        ASTNode*                       node() const { return entity(); }; // alias for entity
        static ASTScope*               lowest_common_ancestor(const std::set<ASTScope*>& scopes);
        static ASTScope*               lowest_common_ancestor(ASTScope* s1, ASTScope* s2);
        static std::set<ASTScope*>&    get_descendent(std::set<ASTScope*>& out, ASTScope* scope, ScopeFlags flags = ScopeFlags_INCLUDE_SELF) { return get_descendent_ex(out, scope, -1, flags); }
        static std::set<ASTScope*>&    get_descendent_ex(std::set<ASTScope*>& out, ASTScope* scope, size_t level_max = -1, ScopeFlags = ScopeFlags_INCLUDE_SELF);
    private:
        void                           _on_shutdown();
        std::vector<ASTNode*>&         _leaves_ex(std::vector<ASTNode*>& out);
        void                           _update_backbone_cache();
        void                           _update_depth_cache();
        void                           _set_depth_cache_dirty();
    };
}
