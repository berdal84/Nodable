#include "SpatialNode.h"
#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm/gtx/matrix_transform_2d.hpp"
#undef GLM_ENABLE_EXPERIMENTAL

tools::SpatialNode::~SpatialNode()
{
    clear();
    assert( _parent == nullptr ); // remove this instance from its parent before to delete it.
    assert( _children.empty() ); // _parent is responsible for removing its children before they get delete.
}

void tools::SpatialNode::clear()
{
    while ( !_children.empty() )
    {
        remove_child( *_children.begin() );
    }
}

void tools::SpatialNode::set_position(const Vec2& _pos)
{
    _transform.set_position(_pos ); // PARENT_SPACE
    set_world_transform_dirty();
}

void tools::SpatialNode::set_position(const Vec2& _pos, Space desired_space )
{
    switch ( desired_space )
    {
        case PARENT_SPACE:
            set_position(_pos); // is default
            break;

        case WORLD_SPACE:
        {
            if (_parent == nullptr)
               return set_position(_pos, PARENT_SPACE); // the world is also the parent space in that case

            glm::vec2 new_pos = _parent->world_matrix_inv() * glm::vec3(_pos.x, _pos.y, 1.f);
            set_position(new_pos);
            break;
        }
        default:
            ASSERT( false ); // Not implemented yet
    }
}

void tools::SpatialNode::set_world_transform_dirty()
{
    _world_matrix_dirty = true;
    for (SpatialNode* child : _children)
    {
        child->set_world_transform_dirty();
    }
}

void tools::SpatialNode::update_world_matrix()
{
    if ( !_world_matrix_dirty )
        return;

    if ( _parent )
        _world_matrix = _transform.matrix() * _parent->world_matrix();
    else
        _world_matrix = _transform.matrix();

    _world_matrix_inv   = glm::inverse( _world_matrix );
    _world_matrix_dirty = false;
}

tools::Vec2 tools::SpatialNode::position() const
{
    return _transform.position();
}

tools::Vec2 tools::SpatialNode::position(Space space) const
{
    switch ( space )
    {
        default:
            VERIFY( false, "This space is not handled");

        case LOCAL_SPACE:
            return {0.f, 0.f};

        case PARENT_SPACE:
            return position(); // is default

        case WORLD_SPACE:
            if ( _parent == nullptr )
                return position();

            glm::vec2 result = _parent->world_matrix() * glm::vec3{position().x, position().y, 1.0f};
            return result;
    }
}

bool tools::SpatialNode::add_child(tools::SpatialNode* new_child, SpatialNodeFlags flags)
{
    ASSERT( new_child != nullptr );
    VERIFY( new_child->parent() == nullptr, "Child already has a parent, remove it first from parent" );
    VERIFY( new_child != this, "Adding itself as primary_child" );

    auto [it, inserted] = this->_children.insert(new_child);

    if ( !inserted )
        return false;

    if ( flags & SpatialNodeFlag_PRESERVE_WORLD_POSITION )
    {
        const Vec2 world_position = new_child->position(WORLD_SPACE);
        new_child->_parent = this;
        new_child->set_position(world_position, WORLD_SPACE);
    }
    else
    {
        new_child->_parent = this;
    }
    return true;
}

bool tools::SpatialNode::remove_child(tools::SpatialNode* child, SpatialNodeFlags flags)
{
    ASSERT(child);
    if (child->_parent == nullptr )
        return false;

    if ( !_children.erase(child) )
        return false;

    if ( flags & SpatialNodeFlag_PRESERVE_WORLD_POSITION )
    {
        const Vec2 pos = child->position(WORLD_SPACE);
        child->_parent = nullptr;
        child->set_position(pos, WORLD_SPACE);
    }
    else
    {
        child->_parent = nullptr;
    }

    return true;
}

void tools::SpatialNode::translate(const tools::Vec2& delta)
{
    // Since Transform2D cannot be rotated yet, we can apply the translation in parent space
    _transform.set_position( _transform.position() + delta );
    set_world_transform_dirty();
}
