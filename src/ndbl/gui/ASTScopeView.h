#pragma once
#include "tools/core/Component.h"
#include "tools/gui/geometry/Rect.h"
#include "tools/gui/geometry/BoxShape2D.h"
#include "tools/gui/ViewState.h"
#include "ndbl/core/ASTScope.h"
#include "ndbl/gui/concepts/CView.h"

namespace ndbl
{
    // forward decl.
    class ASTNodeView;

    typedef int ScopeViewFlags;
    enum ScopeViewFlags_
    {
        ScopeViewFlags_NONE          = 0,
        ScopeViewFlags_RECURSE       = 1 << 0,
        ScopeViewFlags_EXCLUDE_OWNER = 1 << 1,
    };

    typedef bool Theme;
    enum Theme_ : bool
    {
        Theme_DARK  = false,
        Theme_LIGHT = true
    };

    class ASTScopeView
    {
        using Rect = tools::Rect;
//== Data ==============================================================================================================
    public:
        tools::Signal<void(ASTScopeView*)> signal_hover;
    private:
        tools::ViewState          m_view_state{};
        tools::SpatialNode        m_spatial_node{};
        ASTScope*                 m_scope{};
        Rect                      m_content_rect{};
        std::vector<ASTNodeView*> m_wrapped_node_view{};
        Theme                     m_theme{};
//== Methods ===========================================================================================================
    public:
        ASTScopeView() = default;
        ASTScopeView(const ASTScopeView&) = default;

        void         init(ASTScope*);
        void         shutdown();
        void         update(float dt, ScopeViewFlags = ScopeViewFlags_NONE );
        void         draw(float dt);
        ASTNode*       node() { return scope()->node(); }
        const ASTNode* node() const { return scope()->node(); }
        tools::ViewState* state() { return &m_view_state; }
        bool         has_parent() const { return m_scope->parent() != nullptr; }
        ASTScopeView*parent() const;
        ASTScope*    scope() const { return m_scope; }
        size_t       depth() const { return m_scope->depth(); }
        tools::SpatialNode* spatial_node() { return &m_spatial_node; }
        void         set_position(const tools::Vec2& pos, tools::Space space ) { return m_spatial_node.set_position( pos, space );}
        void         translate(const tools::Vec2& delta) { m_spatial_node.translate(delta); }
        bool         must_be_draw() const;
        bool         pinned() const;
        void         set_pinned(bool b = true);
        const Rect&  content_rect() const { return m_content_rect; }
        void         arrange_content();
    public:
        static void  ImGuiTreeNode_ASTScope(const char* title, ASTScope*);
    private:
        static void  ImGuiTreeNode_ASTScopeContent(ASTScope*);
        static void  ImGuiTreeNode_ASTNode(ASTNode*);
    };
}