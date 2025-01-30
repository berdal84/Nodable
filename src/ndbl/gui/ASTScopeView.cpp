#include "ASTScopeView.h"
#include "tools/core/assertions.h"
#include "ndbl/core/ASTScope.h"
#include "ASTNodeView.h"
#include "Config.h"
#include "ndbl/core/ASTFunctionCall.h"
#include "ndbl/core/language/Nodlang.h"

using namespace ndbl;
using namespace tools;

void ASTScopeView::init(ASTScope* scope)
{
    ASSERT(scope != nullptr);

    m_scope = scope;
    scope->set_view(this);
}

void ASTScopeView::shutdown()
{
    spatial_node()->clear();
    m_scope->set_view(nullptr);
    m_scope = nullptr;
}

ASTScopeView* ASTScopeView::parent() const
{
    return m_scope->parent() ? m_scope->parent()->view() : nullptr;
}

void ASTScopeView::update(float dt, ScopeViewFlags flags)
{
    const Config* config = get_config();

    // 1) update recursively
    //    any scope with higher depth in the same hierarchy will be up to date.
    for( ASTNode* child_node : m_scope->child() )
        if ( ASTScope* internal_scope = child_node->internal_scope() )
            internal_scope->view()->update(dt, flags);

    // 2) update content rectangle and wrapped node views
    //
    m_content_rect = {};
    m_wrapped_node_view.clear();
    auto wrap_nodeview = [&](ASTNodeView* nodeview )
    {
        ASSERT( nodeview );
        if ( !nodeview->state()->visible() )
            return;

        const Rect r = nodeview->get_rect(WORLD_SPACE);
        m_content_rect = Rect::bounding_rect(m_content_rect, r);
        m_wrapped_node_view.push_back(nodeview);
    };

    // sibling nodeview is always wrapped inside its own scopeview
    if ( auto sibling_nodeview = m_scope->node()->component<ASTNodeView>() )
        wrap_nodeview( sibling_nodeview );

    for( ASTNode* node : m_scope->child() )
        if ( auto nodeview = node->component<ASTNodeView>() )
            wrap_nodeview( nodeview );

    for( ASTNode* child_node : m_scope->child() )
    {
        if ( child_node->has_internal_scope() )
        {
            ASTScopeView* child_node_scope_view = child_node->internal_scope()->view();
            child_node_scope_view->update(dt, flags);
            m_content_rect = Rect::bounding_rect(m_content_rect, child_node_scope_view->m_content_rect);
        }
    }

    if ( must_be_draw() )
    {
        // Add margins to see clearly nested scopes
        m_content_rect.min -= config->ui_scope_content_rect_margin.min;
        m_content_rect.max += config->ui_scope_content_rect_margin.max;

        // pixel perfect
        m_content_rect.min.round();
        m_content_rect.max.round();
    }


    // 2) update theme
    //
    ASTScopeView* parent_view = parent();
    if ( parent_view )
    {
        m_theme = !parent_view->m_theme;
        if ( !parent_view->must_be_draw() )
            m_theme = !m_theme;
    }
    else
    {
        m_theme = Theme_DARK;
    }
}

bool ASTScopeView::must_be_draw() const
{
    if (!m_content_rect.has_area())
        return false;

    switch ( scope()->child().size() )
    {
        case 0:
            return false;
        case 1:
        {
            ASTNode* single_node = *scope()->child().begin();
            if ( single_node->has_internal_scope() && this->has_parent() )
                return false;
            return true;
        }
        default:
            return true;
    }
}

void ASTScopeView::draw(float dt)
{
    if ( must_be_draw() )
    {
        const Rect r = m_content_rect;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const Config* config = get_config();
        const Vec4& fill_col = m_theme == Theme_DARK ? config->ui_scope_fill_col_light
                                                     : config->ui_scope_fill_col_dark;
        draw_list->AddRectFilled(r.min, r.max, ImGui::GetColorU32(fill_col), config->ui_scope_border_radius );
        if ( m_view_state.selected() )
        {
            draw_list->AddRect(r.min, r.max, ImGui::GetColorU32( config->ui_scope_border_col ) , config->ui_scope_border_radius, 0, config->ui_scope_border_thickness );
        }

        if ( ImGui::IsMouseHoveringRect(r.min, r.max) )
        {
            signal_hover.emit(this);
        }
    }
}

void ASTScopeView::set_pinned(bool b)
{
    m_view_state.set_pinned(b);
}

bool ASTScopeView::pinned() const
{
    return m_view_state.pinned();
}

void ASTScopeView::ImGuiTreeNode_ASTScope(const char* title, ASTScope* scope)
{
    if ( ImGui::TreeNode( title ) )
    {
        if ( scope )
            ImGuiTreeNode_ASTScopeContent(scope);
        else
            ImGui::Text("nullptr");
        ImGui::TreePop();
    }
}

void ASTScopeView::ImGuiTreeNode_ASTNode(ASTNode* node)
{
    bool open = false;
    switch ( node->type() )
    {
        case ASTNodeType_OPERATOR:
        case ASTNodeType_FUNCTION:
        {
            auto* func_node = dynamic_cast<ASTFunctionCall*>( node );
            std::string signature;
            get_language()->serialize_func_sig(signature, &func_node->get_func_type());
            char str[255];
            open = ImGui::TreeNode(node, "[%p] \"%s\" (%s, %s)", func_node, func_node->name().c_str(), func_node->get_class()->name(), signature.c_str());
            break;
        }
        case ASTNodeType_VARIABLE:
        {
            auto* variable = dynamic_cast<ASTVariable*>( node );
            std::string value = variable->value()->token().word_to_string();
            char str[255];
            open = ImGui::TreeNode(node, "[%p] \"%s\" (%s)", variable, value.c_str(), variable->name().c_str());
            break;
        }
        default:
        {
            open = ImGui::TreeNode(node, "[%p] \"%s\" (%s)", node, node->name().c_str(), node->get_class()->name());
        }
    }

    if ( open )
    {
        if ( node->has_internal_scope() )
        {
            ImGuiTreeNode_ASTScopeContent(node->internal_scope());
        }

        ImGui::TreePop();
    }
};

void ASTScopeView::ImGuiTreeNode_ASTScopeContent(ASTScope *scope)
{
    ImGui::PushID( scope );
    std::vector<ASTNode*> backbone = scope->backbone();
    if ( ImGui::TreeNodeEx(&backbone, ImGuiTreeNodeFlags_DefaultOpen, "Children (backbone, ordered)" ) )
    {
        for ( ASTNode* _node : backbone )
        {
            ImGuiTreeNode_ASTNode(_node);
        }
        ImGui::TreePop();
    }

    if ( ImGui::TreeNode(&scope->variable(), "Children (vars only, unordered)") )
    {
        for ( ASTNode* child : scope->variable() )
        {
            ImGuiTreeNode_ASTNode(child);
        }
        ImGui::TreePop();
    }

    if ( ImGui::TreeNode(&scope->child(), "Children (all, unordered)") )
    {
        for ( ASTNode* child : scope->child() )
        {
            ImGuiTreeNode_ASTNode(child);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void ASTScopeView::arrange_content()
{
    for( ASTNodeView* view : m_wrapped_node_view )
    {
        view->arrange_recursively();
    }
}
