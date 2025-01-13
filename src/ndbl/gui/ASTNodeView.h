#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>

#include "tools/core/Component.h"// base class
#include "tools/gui/geometry/BoxShape2D.h"
#include "tools/gui/ImGuiEx.h"
#include "tools/gui/ViewState.h"
#include "ndbl/core/ASTNodeProperty.h"
#include "ndbl/gui/concepts/CView.h"
#include "ASTNodePropertyView.h"
#include "ASTNodeSlotView.h"
#include "types.h"
#include "ViewDetail.h"

namespace ndbl
{
    // forward declaration
    class ASTNode;
    class Graph;
    class ASTScopeView;
    struct ASTNodeSlot;
    struct ASTNodeSlotView;
    class GraphView;

    /**
     * Enum to define some color types
     */
    enum ColorType
    {
        Color_FILL,
        Color_COUNT
    };

    typedef int NodeViewFlags;
    enum NodeViewFlag_
    {
        // note: when adding a new value, remember we want NONE to be the most common case

        NodeViewFlag_NONE                   = 0,
        NodeViewFlag_WITH_RECURSION         = 1 << 0,
        NodeViewFlag_WITH_PINNED            = 1 << 1,
        NodeViewFlag_EXCLUDE_UNSELECTED     = 1 << 2
    };

	/**
	 * This class implement a view for Nodes using ImGui.
	 */
    class ASTNodeView : public tools::Component<ASTNode>
	{
        constexpr static tools::Vec4 DEFAULT_COLOR{1.f, 0.f, 0.f};
//== Data ==============================================================================================================
    private:
        enum PropType
        {
            PropType_IN_STRICTLY = 0,
            PropType_OUT_STRICTLY,
            PropType_INOUT_STRICTLY,
            PropType_IN,
            PropType_OUT,
            PropType_COUNT
        };

        tools::BoxShape2D          m_shape;
        tools::ViewState           m_view_state;
        bool                       m_expanded{true};
        float                      m_opacity{1.f};
        ASTNodeSlotView*           m_hovered_slotview{};
        ASTScopeView*              m_internal_scopeview{};
        std::array<const tools::Vec4*, Color_COUNT> m_colors {&DEFAULT_COLOR};
        std::vector<ASTNodeSlotView*>     m_slot_views;
        std::unordered_map<const ASTNodeProperty*, ASTNodePropertyView*> m_view_by_property;
        ASTNodePropertyView*              m_value_view{};
        std::array<std::vector<ASTNodePropertyView*>, PropType::PropType_COUNT> m_view_by_property_type;
//== Methods ===========================================================================================================
    public:
        friend class GraphView;
        ASTNodeView();
		~ASTNodeView() override;

        ASTNode*                node() const { return entity(); } // entity() alias
        std::vector<ASTNodeView*>  get_adjacent(SlotFlags) const;
        bool                    draw();
        void                    update(float);
        void                    arrange_recursively(bool smoothly = true);
        std::string             get_label();
        tools::Rect             get_rect(tools::Space space = tools::WORLD_SPACE) const { return m_shape.rect(space); }
        tools::Rect             get_rect_ex(tools::Space, NodeViewFlags) const;
        bool                    expanded()const { return m_expanded; }
        void                    set_expanded_rec(bool);
        void                    set_expanded(bool);
        void                    set_inputs_visible(bool visible, bool recursive = false);
        void                    set_children_visible(bool visible, bool recursive = false);
        void                    expand_toggle() { set_expanded(!m_expanded); }
        void                    expand_toggle_rec() { return set_expanded_rec(!m_expanded); };
        void                    set_color( const tools::Vec4*, ColorType = Color_FILL );
        tools::Vec4             get_color(ColorType) const;
        void                    set_size(const tools::Vec2& size) { m_shape.set_size(size); }
        tools::BoxShape2D*      shape() { return &m_shape; }
        const tools::BoxShape2D*shape() const { return &m_shape; }
        void                    translate(const tools::Vec2& delta) { m_shape.spatial_node()->translate(delta); }
        const tools::SpatialNode* spatial_node() const { return m_shape.spatial_node(); }
        tools::SpatialNode*     spatial_node() { return m_shape.spatial_node(); }
        tools::ViewState*       state() { return &m_view_state; }
        const tools::ViewState* state() const { return &m_view_state; }
        void                    reset_all_properties();
        ASTScopeView*           internal_scopeview() { return m_internal_scopeview; }
        const ASTScopeView*     internal_scopeview() const { return m_internal_scopeview; }
        static tools::Rect      bounding_rect(const std::vector<ASTNodeView *>&, tools::Space = tools::WORLD_SPACE, NodeViewFlags = NodeViewFlag_NONE);
        static bool             draw_as_properties_panel(ASTNodeView*, bool* show_advanced );
        static ASTNodeView*     substitute_with_parent_if_not_visible(ASTNodeView*, bool _recursive = true);

    private:
        void                    _handle_init();
        void                    _handle_shutdown();
        ASTNodePropertyView*    _find_property_view(const ASTNodeProperty*);
        void                    _add_child(CView auto* view);
        void                    _draw_slot(ASTNodeSlotView*);
        void                    _set_adjacent_visible(SlotFlags, bool _visible, NodeViewFlags = NodeViewFlag_NONE);

        static void DrawNodeRect(
                tools::Rect rect,
                tools::Vec4 color,
                tools::Vec4 border_highlight_col,
                tools::Vec4 shadow_col,
                tools::Vec4 border_col,
            bool selected,
            float border_radius,
            float border_width
        );
    };

    void ASTNodeView::_add_child(CView auto* view)
    {
        spatial_node()->add_child(view->spatial_node() );
        view->spatial_node()->set_position({0.f, 0.f}, tools::PARENT_SPACE);

        if constexpr ( std::is_same_v<decltype(view), ASTNodeSlotView*> )
        {
            m_slot_views.push_back(view);
        }
    }
}
