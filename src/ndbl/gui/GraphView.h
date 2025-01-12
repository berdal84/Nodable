#pragma once

#include <map>
#include <string>
#include <functional>
#include <vector>

#include "tools/core/Component.h"  // base class
#include "tools/core/Variant.h"
#include "tools/core/UniqueVariantList.h"
#include "tools/gui/ViewState.h"
#include "tools/gui/geometry/Pivots.h"

#include "ndbl/core/ASTScope.h"

#include "Action.h"
#include "ASTNodeView.h"
#include "ASTNodeSlotView.h"
#include "types.h"
#include "tools/core/StateMachine.h"
#include "ASTNodeViewContextualMenu.h"
#include "ASTScopeView.h"

namespace ndbl
{
    // forward declarations
    class Nodable;
    class Graph;
    using tools::Vec2;

    struct EdgeView
    {
        ASTNodeSlotView* tail = nullptr;
        ASTNodeSlotView* head = nullptr;
        bool operator==(const EdgeView& other) const // required to compare tools::Variant<..., EdgeView>
        { return tail == other.tail && head == other.head; }
    };

    using Selectable = tools::Variant<ASTNodeView*, ASTScopeView*, ASTNodeSlotView*, EdgeView> ;
    using Selection  = tools::UniqueVariantList<Selectable> ;

    class GraphView : public tools::Component<Graph>
    {
    public:
	    GraphView();
		~GraphView() override;

        tools::SimpleSignal signal_change;

        void                   update(float dt);
        bool                   draw(float dt);
        void                   add_action_to_node_menu(Action_CreateNode* _action);
        void                   frame_content(FrameMode mode );
        void                   reset(); // unfold and frame the whole graph
        bool                   has_an_active_tool() const;
        Selection&             selection() { return _m_selection; }
        const Selection&       selection() const { return _m_selection; }
        void                   reset_all_properties();
        Graph*                 graph() const { return entity(); } // alias for entity
        static void            draw_wire_from_slot_to_pos(ASTNodeSlotView *from, const Vec2 &end_pos);
    private:
        tools::SpatialNode*    spatial_node() { return _m_shape.spatial_node(); }
        ASTNodeViewContextualMenu      _m_create_node_menu;
        Selectable             _m_hovered;
        Selectable             _m_focused;
        Selection              _m_selection;
        tools::BoxShape2D      _m_shape;
        bool                   _m_physics_dirty = false;

        void                   _handle_init();
        void                   _handle_shutdown();
        void                   _handle_add_node(ASTNode* node);
        void                   _handle_remove_node(ASTNode* node);
        void                   _handle_change_scope(ASTNode* node, ASTScope* old_scope, ASTScope* new_scope);
        void                   _handle_hover(ASTScopeView *scope_view);
        void                   _unfold(); // unfold the graph until it is stabilized
        void                   _update(float dt, u16_t iterations);
        void                   _update(float dt);
        void                   _on_graph_change();
        void                   _on_selection_change(Selection::EventType, Selection::Element );
        void                   _draw_create_node_context_menu(ASTNodeViewContextualMenu&, ASTNodeSlotView* dragged_slotview = nullptr );
        void                   _create_constraints__align_top_recursively(const std::vector<ASTNode*>& follower, ndbl::ASTNode *leader);
        void                   _create_constraints__align_down(ASTNode* follower, const std::vector<ASTNode*>& leader);
        void                   _create_constraints(ASTScope *scope);

        // Tools State Machine
        //--------------------

        // The data (for some states)

        tools::StateMachine    _m_state_machine;
        tools::Vec2            _m_roi_state_start_pos;
        tools::Vec2            _m_roi_state_end_pos;

        // The behavior

        void cursor_state_tick();
        void roi_state_enter();
        void roi_state_tick();
        void drag_state_enter();
        void drag_state_tick();
        void view_pan_state_tick();
        void line_state_enter();
        void line_state_tick();
        void line_state_leave();

    };
}

// Custom hash provided to work in std::hash<std::variant<EdgeView, ...>>
template<>
struct std::hash<ndbl::EdgeView>
{
    std::size_t operator()(const ndbl::EdgeView& edge) const noexcept
    { return tools::Hash::hash(edge); }
};
