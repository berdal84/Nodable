#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>

#include "PropertyView.h"
#include "SlotView.h"
#include "ndbl/core/NodeComponent.h"// base class
#include "ndbl/core/Property.h"
#include "tools/gui/geometry/BoxShape2D.h"
#include "tools/gui/ImGuiEx.h"
#include "tools/gui/ViewState.h"
#include "types.h"
#include "ViewDetail.h"

namespace ndbl
{
    // forward declaration
    class Node;
    class Graph;
    struct Slot;
    struct SlotView;
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
        NodeViewFlag_WITH_MULTICONSTRAINED  = 1 << 2,
        NodeViewFlag_EXCLUDE_UNSELECTED     = 1 << 3
    };

	/**
	 * This class implement a view for Nodes using ImGui.
	 */
    class NodeView : public NodeComponent
	{
    public:
        DECLARE_REFLECT_override
        friend class GraphView;
		NodeView();
		~NodeView();

        std::vector<NodeView*>  get_adjacent(SlotFlags) const;
        bool                    draw();
        void                    update(float);
        void                    arrange_recursively(bool _smoothly = true);
        std::string             get_label();
        tools::Rect             get_rect(tools::Space space = tools::WORLD_SPACE) const;
        tools::Rect             get_rect_ex(tools::Space, NodeViewFlags) const;
        bool                    expanded()const { return m_expanded; }
        void                    set_expanded_rec(bool _expanded);
        void                    set_expanded(bool _expanded);
        void                    set_inputs_visible(bool _visible, bool _recursive = false);
        void                    set_children_visible(bool visible, bool recursively = false);
        void                    expand_toggle() { set_expanded(!m_expanded); }
        void                    expand_toggle_rec() { return set_expanded_rec(!m_expanded); };
        void                    set_color( const tools::Vec4* _color, ColorType _type = Color_FILL );
        tools::Vec4             get_color(ColorType _type) const;
        GraphView*              graph_view() const;
        tools::BoxShape2D*      shape() { return &m_state.shape(); }
        const tools::SpatialNode2D& spatial_node() const { return m_state.spatial_node(); }
        tools::SpatialNode2D&   spatial_node() { return m_state.spatial_node(); }
        tools::ViewState&       state() { return m_state; }
        void                    reset_all_properties();

        static tools::Rect      get_rect(const std::vector<NodeView *> &_views, tools::Space = tools::WORLD_SPACE, NodeViewFlags = NodeViewFlag_NONE);
        static std::vector<tools::Rect>   get_rects(const std::vector<NodeView*>& _in_views, tools::Space space = tools::WORLD_SPACE, NodeViewFlags flags = NodeViewFlag_NONE);
        static bool             is_inside(NodeView*, const tools::Rect&, tools::Space = tools::WORLD_SPACE);
        static void             constraint_to_rect(NodeView*, const tools::Rect& );
        static bool             draw_as_properties_panel(NodeView* _view, bool* _show_advanced );
        static NodeView*        substitute_with_parent_if_not_visible(NodeView* _view, bool _recursive = true);
        static std::vector<NodeView*> substitute_with_parent_if_not_visible(const std::vector<NodeView*>& _in, bool _recurse = true );

    private:
        PropertyView*           find_property_view(const Property *pProperty);
        void                    add_child(PropertyView*);
        void                    add_child(SlotView*);
        void                    draw_slot(SlotView*);
        void                    set_adjacent_visible(SlotFlags, bool _visible, NodeViewFlags = NodeViewFlag_NONE);
        void                    reset();

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


        tools::ViewState m_state;
        bool            m_expanded;
        float           m_opacity;
        SlotView*       m_hovered_slotview;
        SlotView*       m_last_clicked_slotview;
        std::array<const tools::Vec4*, Color_COUNT> m_colors;
        std::vector<SlotView*>     m_slot_views;
        std::unordered_map<const Property*, PropertyView*> m_view_by_property;
        PropertyView*              m_value_view;

        enum PropType
        {
            PropType_IN_STRICTLY = 0,
            PropType_OUT_STRICTLY,
            PropType_INOUT_STRICTLY,
            PropType_IN,
            PropType_OUT,
            PropType_COUNT
        };

        std::array<std::vector<PropertyView*>, PropType::PropType_COUNT> m_view_by_property_type;
    };
}
