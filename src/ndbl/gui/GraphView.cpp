#include "GraphView.h"

#include <algorithm>
#include <ranges>
#include "tools/core/types.h"
#include "tools/core/log.h"
#include "tools/gui/ImGuiEx.h"
#include "tools/core/math.h"
#include "tools/core/StateMachine.h"
#include "tools/gui/Color.h"

#include "ndbl/core/Graph.h"
#include "ndbl/core/ASTLiteral.h"
#include "ndbl/core/ASTUtils.h"
#include "ndbl/core/ASTNodeSlot.h"

#include "ndbl/core/Interpreter.h"
#include "Config.h"
#include "Event.h"
#include "Nodable.h"
#include "ASTNodeView.h"
#include "PhysicsComponent.h"
#include "ASTNodeSlotView.h"
#include "ASTScopeView.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(GraphView).extends<tools::Component<Graph>>();
)

// Popup name
constexpr const char* CONTEXT_POPUP    = "GraphView.ContextMenuPopup";
// Tool names
constexpr const char* CURSOR_STATE     = "Cursor Tool";
constexpr const char* ROI_STATE        = "Selection Tool";
constexpr const char* DRAG_STATE       = "Drag Node Tool";
constexpr const char* VIEW_PAN_STATE   = "Grab View Tool";
constexpr const char* LINE_STATE       = "Line Tool";

GraphView::GraphView()
: Component<Graph>("View")
, _m_state_machine(this)
, _m_spatial_data()
, _m_shape( Vec2{100.f, 100.f} ) // non null area
{
    Component::signal_init.connect<&GraphView::_handle_init>(this);
    Component::signal_shutdown.connect<&GraphView::_handle_shutdown>(this);
}

GraphView::~GraphView()
{
    assert(Component::signal_init.disconnect<&GraphView::_handle_init>(this));
    assert(Component::signal_shutdown.disconnect<&GraphView::_handle_shutdown>(this));
}

void GraphView::_handle_init()
{
    _m_selection.on_change.connect<&GraphView::_on_selection_change>(this);
    graph()->signal_add_node.connect<&GraphView::_handle_add_node>(this);
    graph()->signal_remove_node.connect<&GraphView::_handle_remove_node>(this);
    graph()->signal_change.connect<&GraphView::_on_graph_change>(this);
    graph()->signal_reset.connect<&GraphView::reset>(this);
    graph()->signal_is_complete.connect<&GraphView::reset>(this);

    _m_state_machine.add_state(CURSOR_STATE);
    _m_state_machine.bind<&GraphView::cursor_state_tick>(CURSOR_STATE, When::OnTick);
    _m_state_machine.set_default_state(CURSOR_STATE);

    _m_state_machine.add_state(ROI_STATE);
    _m_state_machine.bind<&GraphView::roi_state_enter>(ROI_STATE, When::OnEnter);
    _m_state_machine.bind<&GraphView::roi_state_tick>(ROI_STATE, When::OnTick);

    _m_state_machine.add_state(DRAG_STATE);
    _m_state_machine.bind<&GraphView::drag_state_enter>(DRAG_STATE, When::OnEnter);
    _m_state_machine.bind<&GraphView::drag_state_tick>(DRAG_STATE, When::OnTick);

    _m_state_machine.add_state(VIEW_PAN_STATE);
    _m_state_machine.bind<&GraphView::view_pan_state_tick>(VIEW_PAN_STATE, When::OnTick);

    _m_state_machine.add_state(LINE_STATE);
    _m_state_machine.bind<&GraphView::line_state_enter>(LINE_STATE, When::OnEnter);
    _m_state_machine.bind<&GraphView::line_state_tick>(LINE_STATE, When::OnTick);
    _m_state_machine.bind<&GraphView::line_state_leave>(LINE_STATE, When::OnLeave);

    _m_state_machine.start();
}

void GraphView::_handle_shutdown()
{
    assert(_m_selection.on_change.disconnect<&GraphView::_on_selection_change>(this));
    assert( graph()->signal_add_node.disconnect<&GraphView::_handle_add_node>(this) );
    assert( graph()->signal_remove_node.disconnect<&GraphView::_handle_remove_node>(this) );
    assert( graph()->signal_change.disconnect<&GraphView::_on_graph_change>(this) );
    assert( graph()->signal_reset.disconnect<&GraphView::reset>(this) );
    assert( graph()->signal_is_complete.disconnect<&GraphView::reset>(this) );
}

void GraphView::_handle_add_node(ASTNode* node)
{
   // view state component
    auto* nodeview = node->components()->create<ASTNodeView>();
    nodeview->set_size({20.f, 35.f});

    // physics component
    node->components()->create<PhysicsComponent>();

    // add a ScopeView for the inner scope and any child that is owned by this node too
    if ( node->has_internal_scope() )
    {
        ASTScope*  internal_scope = node->internal_scope();
        auto*      scope_view     = node->components()->create<ASTScopeView>(internal_scope);
        scope_view->on_hover.connect<&GraphView::_set_hovered>(this);
        scope_view->add_child( nodeview );

        for ( ASTScope* sub_scope : internal_scope->partition() )
        {
            auto* sub_scope_view = node->components()->create<ASTScopeView>(sub_scope);
            sub_scope_view->on_hover.connect<&GraphView::_set_hovered>(this);
            nodeview->_add_child(sub_scope_view);
        }
    }
}

void GraphView::_handle_remove_node(ASTNode* node)
{

}

ImGuiID make_wire_id(const ASTNodeSlot *ptr1, const ASTNodeSlot *ptr2)
{
    string128 id;
    id.append_fmt("wire %zu->%zu", ptr1, ptr2);
    return ImGui::GetID(id.c_str());
}

void GraphView::draw_wire_from_slot_to_pos(ASTNodeSlotView *from, const Vec2 &end_pos)
{
    VERIFY(from != nullptr, "from slot can't be nullptr");

    Config* cfg = get_config();

    // Style

    ImGuiEx::WireStyle style;
    style.shadow_color = cfg->ui_codeflow_shadowColor,
    style.roundness    = 0.f;

    if (from->slot->type() == SlotFlag_TYPE_FLOW) {
        style.color = cfg->ui_codeflow_color,
                style.thickness = cfg->ui_slot_rectangle_size.x * cfg->ui_codeflow_thickness_ratio;
    } else {
        style.color = cfg->ui_node_borderHighlightedColor;
        style.thickness = cfg->ui_wire_bezier_thickness;
    }

    // Draw

    ImGuiID id = make_wire_id(from->slot, nullptr);
    Vec2 start_pos = from->spatial_node()->position(WORLD_SPACE);

    BezierCurveSegment2D segment{
            start_pos, start_pos,
            end_pos, end_pos
    }; // straight line

    ImGuiEx::DrawWire(id, ImGui::GetWindowDrawList(), segment, style);
}

bool GraphView::draw(float dt)
{
    bool changed = false;

    // Ensure view state fit with content region
    // (n.b. we could also implement a struct RootViewState wrapping ViewState)
    Rect region = ImGuiEx::GetContentRegion(WORLD_SPACE );
    _m_shape.set_size( region.size() );
    _m_shape.set_position(region.center()); // children will be relative to the center
    _m_shape.draw_debug_info();

    _m_hovered = {};

    Config*         cfg                    = get_config();
    Interpreter*    interpreter            = get_interpreter();

    ImDrawList*     draw_list              = ImGui::GetWindowDrawList();
    const bool      enable_edition         = interpreter->is_program_stopped();

    // Draw Scopes
    std::vector<ASTScope*> scopes_to_draw = graph()->scopes();
    auto low_to_high_depth = [](ASTScope* s1, ASTScope* s2) { return s1->depth() < s2->depth(); };
    std::sort(scopes_to_draw.begin(), scopes_to_draw.end(), low_to_high_depth);

    for( ASTScope* scope : scopes_to_draw )
    {
        if (ASTScopeView* view = scope->view())
        {
            view->draw(dt);
        }
    }

    // Draw Grid
    const Rect window_content_region = {
        ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin(),
        ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax()
    };
    ImGuiEx::Grid(
            window_content_region,
            cfg->ui_grid_size,
            cfg->ui_grid_subdiv_count,
            ImGui::GetColorU32(cfg->ui_graph_grid_color_major),
            ImGui::GetColorU32(cfg->ui_graph_grid_color_minor));

    // Draw Wires (code flow ONLY)
    const ImGuiEx::WireStyle code_flow_style{
            cfg->ui_codeflow_color,
            cfg->ui_codeflow_color, // hover
            cfg->ui_codeflow_shadowColor,
            cfg->ui_codeflow_thickness(),
            0.0f
    };
    for (ASTNode* each_node: graph()->nodes() )
    {
        ASTNodeView *each_view = ASTNodeView::substitute_with_parent_if_not_visible(each_node->component<ASTNodeView>() );

        if (!each_view) {
            continue;
        }

        std::vector<ASTNodeSlot *> slots = each_node->filter_slots(SlotFlag_FLOW_OUT);
        for (size_t slot_index = 0; slot_index < slots.size(); ++slot_index)
        {
            ASTNodeSlot *slot = slots[slot_index];

            if (slot->empty())
            {
                continue;
            }

            for (const auto &adjacent_slot: slot->adjacent())
            {
                ASTNode*     each_successor_node  = adjacent_slot->node;
                ASTNodeView* possibly_hidden_view = each_successor_node->component<ASTNodeView>();
                ASTNodeView* each_successor_view  = ASTNodeView::substitute_with_parent_if_not_visible(possibly_hidden_view);

                if ( each_successor_view == nullptr )
                    continue;
                if ( !each_view->state()->visible() )
                    continue;
                if ( !each_successor_view->state()->visible() )
                    continue;

                ASTNodeSlotView* tail = slot->view;
                ASTNodeSlotView* head = adjacent_slot->view;

                ImGuiID id = make_wire_id(slot, adjacent_slot);
                Vec2 tail_pos = tail->spatial_node()->position(WORLD_SPACE);
                Vec2 head_pos = head->spatial_node()->position(WORLD_SPACE);
                BezierCurveSegment2D segment{
                        tail_pos,
                        tail_pos,
                        head_pos,
                        head_pos,
                };
                ImGuiEx::DrawWire(id, draw_list, segment, code_flow_style);
                if (ImGui::GetHoveredID() == id )
                {
                    _m_hovered = EdgeView{tail, head};
                }
            }
        }
    }

    // Draw Wires (regular)
    const ImGuiEx::WireStyle default_wire_style{
            cfg->ui_wire_color,
            cfg->ui_wire_color, // hover
            cfg->ui_wire_shadowColor,
            cfg->ui_wire_bezier_thickness,
            cfg->ui_wire_bezier_roundness.x // roundness min
    };
    for (ASTNode* node_out: graph()->nodes() )
    {
        for (const ASTNodeSlot* slot_out: node_out->filter_slots(SlotFlag_OUTPUT))
        {
            for(const ASTNodeSlot* slot_in : slot_out->adjacent())
            {
                if (slot_in == nullptr)
                    continue;

                auto *node_view_out = slot_out->node->component<ASTNodeView>();
                auto *node_view_in  = slot_in->node->component<ASTNodeView>();

                if ( !node_view_out->state()->visible() )
                    continue;
                if ( !node_view_in->state()->visible() )
                    continue;

                Vec2 p1, cp1, cp2, p2; // BezierCurveSegment's points

                ASTNodeSlotView* slot_view_out = slot_out->view;
                ASTNodeSlotView* slot_view_in  = slot_in->view;

                p1 = slot_view_out->spatial_node()->position(WORLD_SPACE);
                p2 = slot_view_in->spatial_node()->position(WORLD_SPACE);

                const Vec2  signed_dist = Vec2::distance(p1, p2);
                const float lensqr_dist = signed_dist.lensqr();

                // Animate style
                ImGuiEx::WireStyle style = default_wire_style;
                if ( _m_selection.contains( node_view_out ) || _m_selection.contains( node_view_in ) )
                {
                    style.color.w *= wave(0.5f, 1.f, (float) App::get_time(), 10.f);
                }
                else if (lensqr_dist > cfg->ui_wire_bezier_fade_lensqr_range.x)
                {
                    // transparent depending on wire length
                    float factor = (lensqr_dist - cfg->ui_wire_bezier_fade_lensqr_range.x) /
                                   (cfg->ui_wire_bezier_fade_lensqr_range.y - cfg->ui_wire_bezier_fade_lensqr_range.x);
                    style.color        = Vec4::lerp(style.color,        Vec4(0, 0, 0, 0), factor);
                    style.shadow_color = Vec4::lerp(style.shadow_color, Vec4(0, 0, 0, 0), factor);
                }

                // Draw transparent some "variable--->ref" wires in certain cases
                if (node_out->type() == ASTNodeType_VARIABLE ) // from a variable
                {
                    auto variable = static_cast<ASTVariable*>( node_out );
                    if (slot_out == variable->ref_out() ) // from a reference slot (can't be a declaration link)
                        if (!node_view_out->state()->selected() && !node_view_in->state()->selected() )
                            style.color.w *= 0.25f;
                }

                // draw the wire if necessary
                if (style.color.w != 0.f)
                {
                    // Determine control points
                    float roundness = tools::clamped_lerp(0.f, 10.f, lensqr_dist / 100.f);
                    cp1 = p1;
                    cp2 = p2 + slot_view_in->direction * roundness;
                    if ( slot_view_out->direction.y > 0.f ) // round out when direction is bottom
                        cp1 += slot_view_out->direction * roundness;

                    BezierCurveSegment2D segment{p1, cp1, cp2, p2};

                    ImGuiID id = make_wire_id(slot_view_out->slot, slot_in);
                    ImGuiEx::DrawWire(id, draw_list, segment, style);
                    if (ImGui::GetHoveredID() == id)
                    {
                        _m_hovered = EdgeView{slot_view_out, slot_view_in};
                    }
                }
            }
        }
    }

    // Draw NodeViews
    for (ASTNode* node : graph()->nodes()  )
    {
        ASTNodeView* nodeview = node->component<ASTNodeView>();

        if ( !nodeview)
            continue;
        if ( !nodeview->state()->visible() )
            continue;

        changed |= nodeview->draw();

        if ( nodeview->state()->hovered() ) // no check if something else is hovered, last node always win against an edge
        {
            if ( nodeview->m_hovered_slotview != nullptr)
            {
                _m_hovered = nodeview->m_hovered_slotview;
            }
            else
                _m_hovered = {nodeview};
        }

        // VM Cursor (scroll to the next node when VM is debugging)
        if (interpreter->is_debugging())
            if (interpreter->is_next_node( nodeview->node() ))
                ImGui::SetScrollHereY();
    }

    // Virtual Machine cursor
    if (interpreter->is_program_running())
    {
        const ASTNode* node = interpreter->get_next_node();
        if (const ASTNodeView* view = node->component<ASTNodeView>())
        {
            Vec2 left = view->get_rect().left();
            Vec2 interpreter_cursor_pos = Vec2::round(left);
            draw_list->AddCircleFilled(interpreter_cursor_pos, 5.0f, ImColor(255, 0, 0));

            Vec2 linePos = interpreter_cursor_pos + Vec2(-10.0f, 0.5f);
            linePos += Vec2( glm::sin(float(App::get_time()) * 12.0f) * 4.0f, 0.f); // wave
            float size = 20.0f;
            float width = 2.0f;
            ImColor color = ImColor(255, 255, 255);

            // arrow ->
            draw_list->AddLine(linePos - Vec2(1.f, 0.0f), linePos - Vec2(size, 0.0f), color, width);
            draw_list->AddLine(linePos, linePos - Vec2(size * 0.5f, -size * 0.5f), color, width);
            draw_list->AddLine(linePos, linePos - Vec2(size * 0.5f, size * 0.5f), color, width);
        }
    }

    _m_state_machine.tick();

    // Debug Infos
    if (cfg->tools_cfg->runtime_debug)
    {
        if (ImGui::Begin("GraphViewToolStateMachine"))
        {
            ImGui::Text("current_tool:         %s" , _m_state_machine.get_current_state_name());
            ImGui::Text("_m_focused.type:       %zu", _m_focused.index() );
            ImGui::Text("_m_hovered.type:       %zu", _m_hovered.index() );
            Vec2 mouse_pos = ImGui::GetMousePos();
            ImGui::Text("_m_mouse_pos:          (%f, %f)", mouse_pos.x, mouse_pos.y);
        }
        ImGui::End();
    }

    // add some empty space
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 100.0f);

    if ( changed )
        on_change.emit();

	return changed;
}

void GraphView::_update(float dt, u16_t count)
{
    // Guards
    ASSERT(dt > 0.f );
    ASSERT(count != 0 );

    for(u16_t i = 0; i < count; i++)
        _update( dt );
}

void GraphView::_create_constraints__align_down(ASTNode* follower, const  std::vector<ASTNode*>& leader )
{
    if( leader.empty() )
        return;

    std::vector<ASTNodeView*> leader_view;
    for ( ASTNode* _leader : leader )
        leader_view.push_back(_leader->component<ASTNodeView>() );

    ASTNodeView* follower_view = follower->component<ASTNodeView>();
    ViewConstraint constraint;
    constraint.name           = "Position below previous";
    constraint.rule           = &ViewConstraint::rule_1_to_N_as_row;
    constraint.leader         = leader_view;
    constraint.follower       = {follower_view};
    constraint.follower_flags = NodeViewFlag_WITH_RECURSION;
    const Vec2 halignment     = constraint.leader.size() == 1 ? LEFT : CENTER;
    constraint.leader_pivot   = halignment + BOTTOM;
    constraint.follower_pivot = halignment + TOP;

    // vertical gap
    constraint.gap_size      = Size_MD;
    constraint.gap_direction = BOTTOM;

    follower->component<PhysicsComponent>()->add_constraint(constraint);
};

void GraphView::_create_constraints__align_top_recursively(const std::vector<ASTNode*>& unfiltered_follower, ndbl::ASTNode* leader )
{
    if ( unfiltered_follower.empty() )
        return;

    ASSERT(leader);
    ASTNodeView* leader_view = leader->component<ASTNodeView>();
    // nodeview's inputs must be aligned on center-top
    // It's a one to many constrain.
    //
    std::vector<ASTNodeView*> follower;
    for (auto* _follower : unfiltered_follower )
        if (ASTUtils::is_output_node_in_expression(_follower, leader))
            follower.push_back(_follower->component<ASTNodeView>() );

    if ( follower.empty() )
        return;

    ViewConstraint constraint;
    constraint.name           = "Align many inputs above";
    constraint.rule           = &ViewConstraint::rule_N_to_1_as_a_row;
    constraint.leader         = { leader_view };
    constraint.leader_pivot   = TOP;
    constraint.follower       = follower;
    constraint.follower_pivot = BOTTOM;
    constraint.gap_size       = Size_SM;
    constraint.gap_direction  = TOP;

    if (follower.size() > 1 )
    {
        constraint.follower_flags = NodeViewFlag_WITH_RECURSION;
    }

    if ( leader->has_flow_adjacent() )
    {
        constraint.follower_pivot = BOTTOM_LEFT;
        constraint.leader_pivot   = TOP_RIGHT;
        constraint.row_direction  = RIGHT;
    }

    leader->component<PhysicsComponent>()->add_constraint(constraint);

    for( ASTNodeView* _leader : follower )
        _create_constraints__align_top_recursively(_leader->node()->inputs(), _leader->node());
};

void GraphView::_create_constraints(ASTScope* scope )
{
    // distribute child scopes
    if ( scope->is_partitioned() )
    {
        ViewConstraint constraint;
        constraint.name          = "Align ScopeView partitions";
        constraint.rule          = &ViewConstraint::rule_distribute_sub_scope_views;
        constraint.leader        = {scope->entity()->component<ASTNodeView>()};
        constraint.leader_pivot  = BOTTOM;
        constraint.gap_size      = Size_XL;
        constraint.gap_direction = BOTTOM;
        scope->entity()->component<PhysicsComponent>()->add_constraint(constraint);
    }

    for ( ASTScope* sub_scope : scope->partition() )
    {
        _create_constraints(sub_scope);
    }

    std::vector<ASTNode*> backbone = scope->backbone();
    for ( ASTNode* child_node : backbone )
    {
        // align child below flow_inputs
        if ( child_node != backbone.front() || scope->is_orphan() )
            _create_constraints__align_down(child_node, child_node->flow_inputs());

        // align child's inputs above
        _create_constraints__align_top_recursively(child_node->inputs(), child_node );

        // child's internal scope
        if ( child_node->has_internal_scope() )
            _create_constraints( child_node->internal_scope() );
    }
};

void GraphView::_update(float dt)
{
    ASSERT( graph() );

    // Physics Components
    LOG_VERBOSE("GraphView", "Updating constraints ...\n");

    if ( _m_physics_dirty )
    {
        // clear all constraints, and THEN create them again

        for (ASTNode* node : graph()->nodes())
            if ( auto* physics = node->component<PhysicsComponent>())
                physics->clear_constraints();

        _create_constraints(graph()->root_scope());

        _m_physics_dirty = false;
    }

    // Apply all constraints, and THEN apply all forces
    // TODO: store PhysicsComponent contiguously, and iterate over all of them.
    //       this requires to store the components on a per Graph basis.
    for ( ASTNode* node : graph()->nodes() )
        if ( auto* _physics = node->component<PhysicsComponent>() )
            _physics->apply_constraints(dt);
    for ( ASTNode* node : graph()->nodes() )
        if ( auto* _physics = node->component<PhysicsComponent>() )
            _physics->apply_forces(dt);

    LOG_VERBOSE("GraphView", "Constraints updated.\n");

    // NodeViews
    for (ASTNode* node : graph()->nodes() )
        if ( auto* view = node->component<ASTNodeView>() )
            view->update(dt);

    // ScopeViews
    if( ASTNode* root = graph()->root_node() )
        if ( auto* view = root->component<ASTScopeView>())
            view->update( dt, ScopeViewFlags_RECURSE );
}

void GraphView::_frame_views(const std::vector<ASTNodeView*>& _views, const Vec2& pivot)
{
    if (_views.empty())
    {
        LOG_VERBOSE("GraphView", "Unable to frame views vector. Reason: is empty.\n");
        return;
    }

    BoxShape2D views_bbox{ASTNodeView::bounding_rect(_views, WORLD_SPACE) };
    const Vec2 desired_pivot_pos = _m_shape.pivot( pivot * 0.95f, WORLD_SPACE); // 5%  margin
    const Vec2 pivot_pos         = views_bbox.pivot(pivot, WORLD_SPACE);
    const Vec2 delta             = desired_pivot_pos - pivot_pos;

    // apply the translation
    // TODO: Instead of applying a translation to all views, apply it to all scope views
    for (ASTNode* node : graph()->nodes() )
        if ( ASTNodeView* view = node->component<ASTNodeView>() )
            view->spatial_node()->translate( delta );
}

void GraphView::_unfold()
{
    const Config* cfg = get_config();

    // Compute the number of update necessary to simulate unfolding for dt seconds
    const u32_t dt      = cfg->graph_view_unfold_duration;
    const u32_t samples = 1000 * dt / cfg->tools_cfg->dt_cap;

    // Run the updates
    _update( float(dt) / samples, samples);
}

void GraphView::add_action_to_node_menu(Action_CreateNode* _action )
{
    _m_create_node_menu.add_action(_action);
}

void GraphView::frame_nodes(FrameMode mode )
{
    switch (mode)
    {
        case FRAME_ALL:
        {
            if( graph()->is_empty() )
                return;

            std::vector<ASTNodeView*> views;
            for(ASTNode* node : graph()->nodes())
                views.push_back(node->component<ASTNodeView>() );
            _frame_views( views, CENTER );
            break;
        }

        case FRAME_SELECTION_ONLY:
        {
            if ( _m_selection.contains<ASTNodeView*>())
            {
                _frame_views(_m_selection.collect<ASTNodeView*>(), CENTER );
            }
            break;
        }
        default:
            VERIFY(false, "unhandled case!");
    }
}

void GraphView::_on_graph_change()
{
    _m_physics_dirty = true;
}

void GraphView::_on_selection_change(Selection::EventType type, Selection::Element elem)
{
    bool selected = type == Selection::EventType::Append;

    switch ( elem.index() )
    {
        case Selectable::index_of<ASTScopeView*>():
        {
            elem.get<ASTScopeView*>()->state()->set_selected(selected );
            break;
        }
        case Selectable::index_of<ASTNodeView*>():
        {
            elem.get<ASTNodeView*>()->state()->set_selected(selected );
            break;
        }
        case Selectable::index_of<EdgeView>():
        {
            break;
        }
        default:
        {
            ASSERT(false); // unhandled case
        }
    }
}

void GraphView::reset()
{
    if ( graph()->is_empty() )
        return;

    // unfold the graph (does not work great when nodes are rendered for the first time)
    _unfold();

    // make sure views are outside viewable rectangle (to avoid flickering)
    Vec2 far_outside = Vec2(-1000.f, -1000.0f);

    for( ASTNode* node : graph()->nodes() )
        if ( auto* view = node->component<ASTNodeView>() )
            view->spatial_node()->translate( far_outside );

    // physics
    _m_physics_dirty = true;

    // frame all (100ms delayed to ensure layout is correct)
    get_event_manager()->dispatch_delayed<Event_FrameSelection>( 100, { FRAME_ALL } );
}

bool GraphView::has_an_active_tool() const
{
    return !_m_state_machine.has_default_state();
}

void GraphView::reset_all_properties()
{
    for( ASTNode* node : graph()->nodes() )
        if ( ASTNodeView* v = node->component<ASTNodeView>() )
            v->reset_all_properties();
}

//-----------------------------------------------------------------------------

void GraphView::_draw_create_node_context_menu(ASTNodeViewContextualMenu& menu, ASTNodeSlotView* dragged_slotview)
{
    if (Action_CreateNode* triggered_action = menu.draw_search_input( dragged_slotview, 10))
    {
        // Generate an event from this action, add some info to the state and dispatch it.
        auto event                     = triggered_action->make_event();
        event->data.graph              = graph();
        event->data.active_slotview    = dragged_slotview;
        event->data.desired_screen_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
        get_event_manager()->dispatch(event);
        ImGui::CloseCurrentPopup();
    }
}

//-----------------------------------------------------------------------------

void GraphView::drag_state_enter()
{
    for( const Selectable& elem : _m_selection )
    {
        if ( auto* nodeview = elem.get_if<ASTNodeView*>() )
            nodeview->state()->set_pinned();
        else if ( auto* scopeview = elem.get_if<ASTScopeView*>() )
            scopeview->state()->set_pinned();
    }
}

void GraphView::drag_state_tick()
{
    const Vec2 delta = ImGui::GetMouseDragDelta();
    ImGui::ResetMouseDragDelta();

    for ( const Selectable& elem : _m_selection )
    {
        if ( auto* nodeview = elem.get_if<ASTNodeView*>() )
            nodeview->translate(delta);
        else if ( auto* scopeview = elem.get_if<ASTScopeView*>() )
            scopeview->translate(delta);
    }

    if ( ImGui::IsMouseReleased(0) )
        _m_state_machine.exit_state();
}


//-----------------------------------------------------------------------------

void GraphView::view_pan_state_tick()
{
    // The code is very similar to drag_state_tick, however it should not be. Indeed, we hack a little here
    // by translating all the nodes instead of translating the graphview content...

    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    Vec2 delta = ImGui::GetMouseDragDelta();
    for( ASTNode* node : graph()->nodes() )
        if ( auto v = node->component<ASTNodeView>() )
            v->spatial_node()->translate(delta);

    ImGui::ResetMouseDragDelta();

    if ( ImGui::IsMouseReleased(0) )
        _m_state_machine.exit_state();
}

//-----------------------------------------------------------------------------

void GraphView::cursor_state_tick()
{
    if ( ImGui::BeginPopup(CONTEXT_POPUP) )
    {
        if ( ImGui::IsWindowAppearing())
            _m_create_node_menu.flag_to_be_reset();

        switch ( _m_focused.index() )
        {
            case Selectable::index_null:
            {
                _draw_create_node_context_menu(_m_create_node_menu);
                break;
            }

            case Selectable::index_of<ASTScopeView*>():
            {
                auto      scopeview = _m_focused.get<ASTScopeView*>();
                ASTNode*     node     = scopeview->entity();
                ASTNodeView* nodeview = node->component<ASTNodeView>();
                if ( ImGui::MenuItem( nodeview->expanded() ? "Collapse Scope" : "Expand Scope" ) )
                {
                    nodeview->expand_toggle_rec();
                }

                if ( ImGui::MenuItem("Delete Scope") )
                {
                    auto event = new Event_DeleteSelection({scopeview->entity()});
                    get_event_manager()->dispatch(event);
                }

                if ( ImGui::MenuItem("Select Scope") )
                {
                    // Get descendent scopes
                    std::set<ASTScope*> children;
                    ASTScope::get_descendent(children, scopeview->scope(), ScopeFlags_INCLUDE_SELF );

                    // Extract node views from each descendent
                    std::vector<ASTNodeView*> views;
                    for(ASTScope* child : children)
                    {
                        // Include scope owner's view too
                        if ( auto* view = child->entity()->component<ASTNodeView>())
                            views.push_back( view );

                        // and every other child's
                        for( ASTNode* child_node : child->backbone() )
                            if ( auto* view = child_node->component<ASTNodeView>())
                                views.push_back(view);
                    }
                    // Replace selection
                    _m_selection.clear();
                    _m_selection.append( views.begin(), views.end() ) ;
                }

                ImGui::Separator();
                _draw_create_node_context_menu(_m_create_node_menu);

                break;
            }

            case Selectable::index_of<EdgeView>():
            {
                auto edge = _m_focused.get<EdgeView>();
                if ( ImGui::MenuItem(ICON_FA_TRASH " Delete Edge") )
                {
                    auto* event = new Event_DeleteEdge();
                    event->data.first  = edge.head->slot;
                    event->data.second = edge.tail->slot;
                    get_event_manager()->dispatch( event );
                }

                break;
            }

            case Selectable::index_of<ASTNodeSlotView*>():
            {
                if ( ImGui::MenuItem(ICON_FA_TRASH " Disconnect Edges") )
                {
                    auto* event = new Event_SlotDisconnectAll();
                    event->data.first = _m_focused.get<ASTNodeSlotView*>()->slot;
                    get_event_manager()->dispatch( event );
                }

                break;
            }

            case Selectable::index_of<ASTNodeView*>():
            {
                auto nodeview = _m_focused.get<ASTNodeView*>();

                if ( ImGui::MenuItem(ICON_FA_TRASH " Delete Node") )
                {
                    auto* event = new Event_DeleteSelection ();
                    event->data.node = nodeview->node();
                    get_event_manager()->dispatch( event );
                }

                if ( ImGui::MenuItem(ICON_FA_MAP_PIN " Pin/Unpin Node") )
                {
                    const bool pinned = nodeview->state()->pinned();
                    nodeview->state()->set_pinned( !pinned );
                }

                if ( ImGui::MenuItem(ICON_FA_WINDOW_RESTORE " Arrange Node") )
                {
                    nodeview->arrange_recursively();
                }

                break;
            }
        }

        ImGui::EndPopup();

        // When we're focused on something with popup open, we don't want to do things based on _m_hovered.type (see below)
        if ( !_m_focused.empty() )
            return;
    }

    switch ( _m_hovered.index() )
    {
        case Selectable::index_of<ASTNodeSlotView*>():
        {
            if ( ImGui::IsMouseClicked(1) )
            {
                _m_focused = _m_hovered;
                ImGui::OpenPopup(CONTEXT_POPUP);
            }
            else if (ImGui::IsMouseDragging(0, 0.f))
            {
                _m_focused = _m_hovered;
                _m_state_machine.change_state(LINE_STATE);
            }
            break;
        }

        case Selectable::index_of<EdgeView>():
        {
            if (ImGui::IsMouseDragging(0, 0.1f))
            {
                _m_focused = _m_hovered;
            }
            else if (ImGui::IsMouseClicked(1))
            {
                _m_focused = _m_hovered;
                ImGui::OpenPopup(CONTEXT_POPUP);
            }
            break;
        }

        case Selectable::index_of<ASTNodeView*>():
        case Selectable::index_of<ASTScopeView*>():
        {
            const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

            if ( ImGui::IsMouseReleased(0) )
            {
                if ( ctrl_pressed )
                {
                    if ( !_m_selection.contains( _m_hovered ) )
                    {
                        _m_selection.append( _m_hovered );
                        _m_focused = _m_hovered;
                    }
                    else
                    {
                        _m_selection.remove( _m_hovered );
                    }
                }
                else
                {
                    _m_selection.clear();
                    _m_selection.append( _m_hovered );
                    _m_focused = _m_hovered;
                }
            }
            else if (ImGui::IsMouseClicked(1))
            {
                _m_focused = _m_hovered;
                ImGui::OpenPopup(CONTEXT_POPUP);
            }
            else if ( ImGui::IsMouseDragging(0) )
            {
                if ( !_m_selection.contains( _m_hovered) )
                {
                    if ( !ctrl_pressed )
                        _m_selection.clear();
                    _m_selection.append( _m_hovered );
                }
                _m_state_machine.change_state(DRAG_STATE);
            }
            break;
        }

        case Selectable::index_null:
        {
            if ( ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows) )
            {
                if (ImGui::IsMouseClicked(0))
                    _m_selection.clear(); // Deselect All (Click on the background)
                else if (ImGui::IsMouseReleased(0))
                    _m_focused = {};
                else if (ImGui::IsMouseClicked(1))
                    ImGui::OpenPopup(CONTEXT_POPUP);
                else if (ImGui::IsMouseDragging(0))
                {
                    if (ImGui::IsKeyDown(ImGuiKey_Space))
                        _m_state_machine.change_state(VIEW_PAN_STATE);
                    else
                        _m_state_machine.change_state(ROI_STATE);
                }
            }

            break;
        }

        default:
            VERIFY(false, "Unhandled case, must be implemented!");
    }
}

//-----------------------------------------------------------------------------

void GraphView::line_state_enter()
{
    ASSERT( _m_focused.holds_alternative<ASTNodeSlotView*>() );
}

void GraphView::line_state_tick()
{
    Vec2 mouse_pos_snapped = Vec2{ImGui::GetMousePos()};
    if ( auto slotview = _m_hovered.get_if<ASTNodeSlotView*>() )
    {
        mouse_pos_snapped = slotview->spatial_node()->position(WORLD_SPACE);
    }

    // Contextual menu
    if ( ImGui::BeginPopup(CONTEXT_POPUP) )
    {
        mouse_pos_snapped = ImGui::GetMousePosOnOpeningCurrentPopup();

        if ( ImGui::IsWindowAppearing() )
            _m_create_node_menu.flag_to_be_reset();

        if ( _m_hovered.empty() )
            _draw_create_node_context_menu(_m_create_node_menu, _m_focused.get<ASTNodeSlotView*>() );

        if ( ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) )
            _m_state_machine.exit_state();

        ImGui::EndPopup();
    }
    else if ( ImGui::IsMouseReleased(0) )
    {
        if ( _m_hovered.holds_alternative<ASTNodeSlotView*>() )
        {
            if ( _m_focused != _m_hovered )
            {
                auto event = new Event_SlotDropped();
                event->data.first  = _m_focused.get<ASTNodeSlotView*>()->slot;
                event->data.second = _m_hovered.get<ASTNodeSlotView*>()->slot;
                get_event_manager()->dispatch(event);
                _m_state_machine.exit_state();
            }
        }
        else
        {
            ImGui::OpenPopup(CONTEXT_POPUP);
        }
    }

    // Draw a temporary wire from focused/dragged slotview to the mouse cursor
    GraphView::draw_wire_from_slot_to_pos(_m_focused.get<ASTNodeSlotView*>(), mouse_pos_snapped );
}

void GraphView::line_state_leave()
{
    _m_focused = {};
}

//-----------------------------------------------------------------------------

void GraphView::roi_state_enter()
{
    _m_roi_state_start_pos = ImGui::GetMousePos();
    _m_roi_state_end_pos   = ImGui::GetMousePos();;
}

void GraphView::roi_state_tick()
{
    _m_roi_state_end_pos = ImGui::GetMousePos();

    // Get normalized ROI rectangle
    Rect roi = Rect::normalize({_m_roi_state_start_pos, _m_roi_state_end_pos});

    // Expand to avoid null area
    const int roi_border_width = 2;
    roi.expand(Vec2{roi_border_width*0.5f});

    // Draw the ROI rectangle
    float alpha = wave(0.5f, 0.75f, (float) App::get_time(), 10.f);
    auto* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(roi.min, roi.max, ImColor(1.f, 1.f, 1.f, alpha), roi_border_width, ImDrawFlags_RoundCornersAll , roi_border_width );

    if (ImGui::IsMouseReleased(0))
    {
        // Get the views included in the ROI
        std::set<ASTNodeView*> nodeviews_inside_roi;
        for ( ASTNode* node : graph()->nodes() )
            if ( auto view = node->component<ASTNodeView>() )
                if ( Rect::contains(roi, view->get_rect()) )
                    nodeviews_inside_roi.insert( view );

        // Select them
        const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
        if ( !ctrl_pressed )
            _m_selection.clear();
        _m_selection.append(nodeviews_inside_roi.begin(), nodeviews_inside_roi.end() );

        _m_state_machine.exit_state();
    }
}

void GraphView::add_child(ASTNodeView* view)
{
    SpatialNode* _spatial_node = view->spatial_node();
    VERIFY(_spatial_node != nullptr, "A SpatialNode is required to _add_child");
    _m_spatial_data.add_child( _spatial_node );
}

void GraphView::update(float dt)
{
    // Determines how many times update should be called
    ASSERT( dt >= 0.f);
    u16_t sample_count = (u16_t)(dt * get_config()->ui_node_physics_frequency);
    if ( sample_count == 0 ) // When frame rate is too slow
        sample_count = 1;
    const float sample_dt = dt / float(sample_count);

    // Do the update(s)
    for(size_t i = 0; i < sample_count; ++i)
        _update(sample_dt);
}

void GraphView::_set_hovered(ASTScopeView* scope_view)
{
    if ( !_m_hovered.holds_alternative<ASTScopeView*>() )
        _m_hovered = scope_view;
    else if ( _m_hovered.empty() )
        _m_hovered = scope_view;
    else if ( scope_view->depth() >= _m_hovered.get<ASTScopeView*>()->depth() )
        _m_hovered = scope_view;
}

