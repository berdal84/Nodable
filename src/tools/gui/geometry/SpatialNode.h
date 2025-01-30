#pragma once
#include "glm/glm/mat3x3.hpp"
#include <unordered_set>
#include "tools/core/assertions.h"
#include "tools/core/Component.h"
#include "TRSTransform2D.h"
#include "Space.h"
#include "Rect.h"
#include "Vec2.h"

namespace tools
{

    typedef int SpatialNodeFlags;
    enum SpatialNodeFlag_
    {
        SpatialNodeFlag_NONE                    = 0,
        SpatialNodeFlag_PRESERVE_WORLD_POSITION = 1 << 0,
    };

    /**
     * Very simple spatial node in 2D.
     * A scene graph can be created by linking parent and primary_child nodes.
     * Currently, we can only set and get the position (not implemented in TRSTransform2D)
     */
    class SpatialNode
    {
    public:
        typedef std::unordered_set<SpatialNode*> Children;

        ~SpatialNode();

        void                  set_position(const Vec2&);
        void                  set_position(const Vec2&, Space);
        Vec2                  position() const;
        Vec2                  position(Space) const;
        void                  translate(const tools::Vec2& delta);
        const glm::mat3&      world_matrix() const     { const_cast<SpatialNode*>(this)->update_world_matrix(); return _world_matrix; }
        const glm::mat3&      world_matrix_inv() const { const_cast<SpatialNode*>(this)->update_world_matrix(); return _world_matrix_inv; }
        void                  set_world_transform_dirty();
        bool                  add_child(SpatialNode*, SpatialNodeFlags = SpatialNodeFlag_PRESERVE_WORLD_POSITION);
        bool                  remove_child(SpatialNode*, SpatialNodeFlags = SpatialNodeFlag_PRESERVE_WORLD_POSITION);
        bool                  has_parent() const { return _parent != nullptr; }
        SpatialNode*          parent() const { return _parent; }
        void                  update_world_matrix();
        const Children&       children() const { return _children; }
        void                  clear();

    private:
        SpatialNode*          _parent = nullptr;
        Children              _children;
        TRSTransform2D        _transform; // local transform, relative to the parent
        glm::mat3             _world_matrix = {1.f}; // update only on-demand when m_world_matrix_dirty is set.
        glm::mat3             _world_matrix_inv = {1.f}; // update only on-demand when m_world_matrix_dirty is set.
        bool                  _world_matrix_dirty = true;
    };
}