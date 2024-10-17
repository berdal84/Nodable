#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <cmath> // round()
#include <algorithm>
#include <observe/observer.h>

#include "PropertyView.h"
#include "SlotView.h"
#include "ndbl/core/NodeComponent.h"// base class
#include "ndbl/core/Property.h"
#include "tools/gui/geometry/Box.h"
#include "tools/gui/ImGuiEx.h"
#include "tools/gui/ViewState.h"
#include "types.h"
#include "ViewDetail.h"

namespace ndbl
{
    // forward declaration
    class Node;
    class Graph;
    class Slot;
    class SlotView;
    class NodeViewConstraint;
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
        friend class GraphView;
		NodeView();
		~NodeView();

        Node*                   get_node() const { return m_owner; }
        bool                    selected() const { return  m_state.selected; };
        inline bool             pinned() const { return m_pinned; }
        bool                    visible() const { return m_state.visible; };
        void                    set_pinned(bool b = true ) { m_pinned = b; }
        std::vector<NodeView*>  get_adjacent(SlotFlags) const;
        bool                    draw();
        void                    set_owner(Node*)override;
        bool                    update(float);
        void                    translate( const tools::Vec2&);
        void                    translate_ex(const tools::Vec2&, NodeViewFlags flags);
        void                    arrange_recursively(bool _smoothly = true);
        std::string             get_label();
        tools::Rect             get_rect(tools::Space space = tools::SCREEN_SPACE) const;
        tools::Rect             get_rect_ex(tools::Space, NodeViewFlags) const;
        bool                    expanded()const { return m_expanded; }
        void                    set_expanded_rec(bool _expanded);
        void                    set_expanded(bool _expanded);
        void                    set_inputs_visible(bool _visible, bool _recursive = false);
        void                    set_children_visible(bool _visible, bool _recursive = false);
        void                    expand_toggle();
        void                    expand_toggle_rec();
        void                    set_color( const tools::Vec4* _color, ColorType _type = Color_FILL );
        tools::Vec4             get_color(ColorType _type) const;
        GraphView*              get_graph() const;
        void                    set_pos(tools::Vec2 pos, tools::Space space = tools::SCREEN_SPACE) { return m_state.set_pos(pos, space); }
        tools::Vec2             get_pos(tools::Space space = tools::SCREEN_SPACE) const { return m_state.get_pos(space); }
        tools::ViewState*            base_view() { return &m_state; }
        bool                    hovered() const { return m_state.hovered; }
        void                    set_selected(bool b = true) { m_state.selected = b; };

        static tools::Rect      get_rect(const std::vector<NodeView *> &_views, tools::Space = tools::SCREEN_SPACE, NodeViewFlags = NodeViewFlag_NONE);
        static std::vector<tools::Rect>   get_rects( const std::vector<NodeView*>& _in_views, tools::Space space = tools::SCREEN_SPACE, NodeViewFlags flags = NodeViewFlag_NONE);
        static bool             is_inside(NodeView*, const tools::Rect&, tools::Space = tools::SCREEN_SPACE);
        static void             constraint_to_rect(NodeView*, const tools::Rect& );
        static bool             draw_property_view(PropertyView*, bool compact_mode = false, const char* _override_label = nullptr);
        static void             draw_as_properties_panel(NodeView* _view, bool *_nodes );
        static NodeView*        substitute_with_parent_if_not_visible(NodeView* _view, bool _recursive = true);
        static std::vector<NodeView*> substitute_with_parent_if_not_visible(const std::vector<NodeView*>& _in, bool _recurse = true );
        static void             translate(const std::vector<NodeView*>&, const tools::Vec2& delta);
    private:
        void                    draw_slot(SlotView*);
        void                    set_adjacent_visible(SlotFlags flags, bool _visible, bool _recursive);
        bool                    _draw_property_view(PropertyView* _view, ViewDetail detail);
        void                    update_labels_from_name(const Node *_node);

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
        static float calc_input_width(const char* text);

        tools::ViewState     m_state; // uses View by Composition
        std::string     m_label;
        std::string     m_short_label;
        bool            m_expanded;
        bool            m_pinned;
        float           m_opacity;
        SlotView*       m_hovered_slotview;
        SlotView*       m_last_clicked_slotview;
        std::array<const tools::Vec4*, Color_COUNT> m_colors;
        std::vector<SlotView*>     m_slot_views;
        std::unordered_map<const Property*, PropertyView*> m_property_views__all;
        PropertyView*              m_property_view_this;
        std::vector<PropertyView*> m_property_views__in_strictly;
        std::vector<PropertyView*> m_property_views__out_strictly;
        std::vector<PropertyView*> m_property_views__inout_strictly;
        std::vector<PropertyView*> m_property_views__out;
        std::vector<PropertyView*> m_property_views__in;

        REFLECT_DERIVED_CLASS()

    };
}
