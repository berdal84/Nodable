#include "PhysicsComponent.h"

#include <numeric>
#include <ranges> // for std::iota
#include "tools/core/math.h"
#include "tools/core/assertions.h"
#include "tools/core/Component.h"
#include "tools/gui/Config.h"
#include "ndbl/core/ASTUtils.h"
#include "ndbl/core/ASTNode.h"
#include "ndbl/core/ASTVariable.h"
#include "ndbl/gui/ASTNodeView.h"
#include "ndbl/gui/ASTScopeView.h"
#include "Config.h"
#include "tools/gui/geometry/Pivots.h"

using namespace ndbl;
using namespace tools;

#ifdef NDBL_DEBUG
#define DEBUG_DRAW 1
#endif

REFLECT_STATIC_INITIALIZER
(
   DEFINE_REFLECT(PhysicsComponent).extends<Component<ASTNode>>();
)


PhysicsComponent::PhysicsComponent()
: Component<ASTNode>("Physics")
{
    Component::signal_init.connect<&PhysicsComponent::_on_init>(this);
}

void PhysicsComponent::_on_init()
{
    _view      = entity()->component<ASTNodeView>();
    ASSERT(_view);
    _is_active = true;
}

void PhysicsComponent::clear_constraints()
{
    _constraints.clear();
}

void PhysicsComponent::apply_constraints(float _dt)
{
    if( !_is_active )
        return;

    for (auto& constraint : _constraints)
        constraint.update(_dt);
}

void PhysicsComponent::translate(const tools::Vec2& delta, float speed, bool recursive)
{
    const Vec2 force = delta * speed;
    add_force(force, recursive);
}

void PhysicsComponent::translate_to(const tools::Vec2& pos, float speed, bool recursive, tools::Space space)
{
    const Vec2 delta = pos - _view->spatial_node()->position(space);
    const Vec2 force = delta * speed;
    add_force(force, recursive);
}

void PhysicsComponent::add_force(const tools::Vec2& force, bool _recurse)
{
    _forces_sum += force;

    if ( !_recurse ) return;

    for (ASTNode* input_node: _view->node()->inputs() )
    {
        ASTNodeView* input_view = input_node->component<ASTNodeView>();

        if ( !input_view->state()->pinned())
            if (ASTUtils::is_output_node_in_expression(input_node, _view->node()))
                if(auto* physics_component = input_node->component<PhysicsComponent>())
                    physics_component->add_force(force, _recurse);
    }
}

void ndbl::PhysicsComponent::apply_forces(float _dt)
{
    float lensqr_max       = std::pow(100, 4);
    float friction_coef    = tools::clamped_lerp(0.0f, 0.5f, _forces_sum.lensqr() / lensqr_max);
    Vec2  soften_force_sum = Vec2::lerp(_last_frame_forces_sum, _forces_sum, 0.95f);
    Vec2  delta            = soften_force_sum * (1.0f - friction_coef) * _dt;

    _view->spatial_node()->translate( delta );

    _last_frame_forces_sum = soften_force_sum;
    _forces_sum            = Vec2();
}

void ViewConstraint::update(float dt)
{
    if ( enabled )
    {
        (this->*rule)(dt);
    }
}

void ViewConstraint::rule_1_to_N_as_row(float dt)
{
    // This type of constrain is designed to make a single NodeView to follow many others

    VERIFY(!leader.empty(), "No leader found!");
    VERIFY(follower.size() == 1, "This is a one to many relationship, a single follower only is allowed");

    std::vector<ASTNodeView*> clean_follower = ViewConstraint::clean(follower);
    if( clean_follower.empty() )
        return;

    Config* cfg = get_config();
    const ASTNodeView* _follower      = clean_follower[0];
    const BoxShape2D leaders_box{ASTNodeView::bounding_rect(leader, WORLD_SPACE, leader_flags) };
    const BoxShape2D follower_box{ _follower->get_rect_ex(WORLD_SPACE, follower_flags) };

    // Compute how much the follower box needs to be moved to snap the leader's box at a given pivots.
    Vec2 delta = BoxShape2D::diff(leaders_box, leader_pivot , follower_box, follower_pivot );
    delta += gap_direction * cfg->ui_node_gap(gap_size);

    // Apply a force to translate to the (single) follower
    Vec2 current_pos = _follower->spatial_node()->position(WORLD_SPACE);
    Vec2 desired_pos = current_pos + delta;
    auto* physics_component = _follower->node()->component<PhysicsComponent>();
    VERIFY(physics_component, "Component required");
    physics_component->translate_to(desired_pos, cfg->ui_node_speed, true, WORLD_SPACE);
}

void ViewConstraint::rule_N_to_1_as_a_row(float _dt)
{
    ASSERT(leader.size() == 1);
    ASSERT(follower.size() > 0);

    Config* cfg = get_config();
    std::vector<ASTNodeView*> clean_follower = clean(follower);
    if( clean_follower.empty() )
        return;

    // Form a row with each view box
    std::vector<BoxShape2D>  box(follower.size());
    std::vector<Vec2> delta(follower.size());
    const Vec2        gap = cfg->ui_node_gap(gap_size);

    for(size_t i = 0; i < clean_follower.size(); i++)
    {
        box[i] = BoxShape2D{ clean_follower[i]->get_rect_ex(WORLD_SPACE, follower_flags) };

        // Determine the delta required to snap the current follower with either the leaders or the previous follower.
        if ( i == 0 )
        {
            // First box is aligned with the leader
            const BoxShape2D leader_box{ leader[0]->get_rect_ex(WORLD_SPACE, leader_flags) };
            delta[i] = BoxShape2D::diff(leader_box, leader_pivot, box[i], follower_pivot);
            delta[i] += gap * gap_direction;
        }
        else
        {
            // i+1 box is aligned with the i
            delta[i] = BoxShape2D::diff(box[i - 1] , row_direction, box[i], -row_direction);
            delta[i] += gap * row_direction;
            delta[i] -= delta[i-1]; //
        }
    }

    for(size_t i = 0; i < clean_follower.size(); i++)
    {
        auto* physics_component = clean_follower[i]->node()->component<PhysicsComponent>();
        if( !physics_component )
            continue;
        Vec2 current_pos = clean_follower[i]->spatial_node()->position(WORLD_SPACE);
        Vec2 desired_pos = current_pos + delta[i];
        physics_component->translate_to(desired_pos, cfg->ui_node_speed, true, WORLD_SPACE);
    }
}

std::vector<ASTNodeView*> ViewConstraint::clean(std::vector<ASTNodeView *> &views)
{
    std::vector<ASTNodeView *> result;
    for(auto* view : views)
        if (view->state()->visible())
            if (!view->state()->pinned())
                result.push_back(view);
    return result;
}

void ViewConstraint::rule_distribute_sub_scope_views(float dt)
{
    // filter views to constrain
    //
    // TODO: there is an issue here, due to the specific case of Scope being partitions (sharing the same node with
    //       their parent scope), it is complicated to disable the constraints when the partition contains a single
    //       nested scope (e.g. in a while/if/for/etc.).
    //       The concept of partition should be removed. They must be either dynamically added/removed when user
    //       connects a node to a branch, or they must be attached to a separate node.
    //
    std::vector<ASTScopeView*> sub_scope_view;
    for( ASTNodeView* _follower : follower )
    {
        ASTScopeView* _follower_scopeview = _follower->internal_scopeview();
        ASSERT(_follower_scopeview);
        if ( !_follower_scopeview->pinned() )
            if ( _follower_scopeview->must_be_draw() )
                sub_scope_view.push_back( _follower_scopeview );
    }

    // get all content rects
    std::vector<Rect> new_content_rect;
    for(auto _view : sub_scope_view)
        new_content_rect.push_back( _view->content_rect() );

    // make a row
    const float gap = get_config()->ui_scope_gap( gap_size );
    Rect::make_row(new_content_rect, gap );

    // v align
    const Vec2 align_pos = leader[0]->shape()->pivot(leader_pivot, WORLD_SPACE )
                         + Vec2{0.f, gap} * gap_direction;
    Rect::align_top(new_content_rect, align_pos.y );

    // h align
    Rect::center(new_content_rect, align_pos.x );

    // translate each sub_scope
    for(size_t i = 0; i < sub_scope_view.size(); ++i)
    {
        const Vec2 cur_pos = sub_scope_view[i]->content_rect().center();
        const Vec2 new_pos = new_content_rect[i].center();
        const Vec2 delta = new_pos - cur_pos;

        // Apply force to translate head
        auto* physics = sub_scope_view[i]->scope()->head()->component<PhysicsComponent>();
        VERIFY(physics, "A PhysicsComponent is required on this entity to apply a force to");
        physics->translate(delta, get_config()->ui_node_speed, true );
    }
}
