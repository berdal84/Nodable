#pragma once
#include "SpatialNode.h"
#include "imgui.h"
#include "Rect.h"
#include "Axis.h"

namespace tools
{
    class BoxShape2D
    {
    public:
        BoxShape2D() = default;
        BoxShape2D(const Vec2& size);
        explicit BoxShape2D(const Rect& rect); // will produce a centered box from rectangle size

        SpatialNode* spatial_node() { return &_spatial_node; }
        const SpatialNode* spatial_node() const { return &_spatial_node; }
        Vec2          position(Space space = PARENT_SPACE) const;
        void          set_position(Vec2 p, Space space = PARENT_SPACE ) { _spatial_node.set_position(p, space); }
        void          set_size(const Vec2& s);
        Vec2          size() const { return _half_size * 2.0f; }
        const Vec2&   half_size() const { return _half_size; }
        Rect          rect(Space = PARENT_SPACE) const;
        Vec2          pivot(const Vec2& pivot, Space = PARENT_SPACE) const;
        void          draw_debug_info();
        static Vec2   diff(const BoxShape2D&  leader, const Vec2& leader_pivot,
                           const BoxShape2D&  follower, const Vec2& follower_pivot,
                           const Vec2& axis = XY_AXIS
                           ); // Return the delta between two Box pivots on a given axis
    private:
        Vec2          _half_size    = {0.f, 0.f};
        SpatialNode   _spatial_node;
    };
}