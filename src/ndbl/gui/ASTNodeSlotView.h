#pragma once

#include "tools/core/reflection/Type.h"
#include "tools/gui/ImGuiEx.h"

#include "ndbl/core/ASTNodeSlot.h"
#include "ndbl/core/ASTNodeProperty.h"
#include "types.h"
#include "tools/gui/geometry/Vec2.h"
#include "tools/gui/ViewState.h"


namespace ndbl
{
    class ASTNodeView;

    enum ShapeType
    {
        ShapeType_NONE = 0,
        ShapeType_CIRCLE,
        ShapeType_RECTANGLE
    };

    struct ASTNodeSlotView
    {
        ASTNodeSlotView(
                ASTNodeSlot*,
                const tools::Vec2& /* alignment */,
                ShapeType,
                size_t /* index */,
                const tools::BoxShape2D* /* alignment_ref */
            );

        const size_t    index;
        const ShapeType shape_type;
        tools::Vec2            direction; // cached
        ASTNodeSlot* const slot;
        tools::Vec2               alignment;
        const tools::BoxShape2D*  alignment_ref;

        void                  update(float dt);
        bool                  draw();
        tools::string64       compute_tooltip() const;
        ASTNode*              node()const { return slot->node; }
        bool                  allows(ASTNodeSlotFlag flags) const { return slot->has_flags(flags); }
        tools::SpatialNode*          spatial_node() { return _shape.spatial_node(); }
        const tools::SpatialNode*    spatial_node() const { return _shape.spatial_node(); }
        tools::BoxShape2D*           shape() { return &_shape; }
        const tools::BoxShape2D*     shape() const { return &_shape; }
        tools::ViewState*        state() { return &_state; }
        const tools::ViewState*  state() const { return &_state; }
        const ASTNodeProperty*   property()const { return slot->property; }
        const tools::TypeDescriptor* property_type()const { return property() ? property()->get_type() : nullptr; }
        void                  update_direction_from_alignment();

    private:
        tools::ViewState  _state;
        tools::BoxShape2D _shape;
    };
}