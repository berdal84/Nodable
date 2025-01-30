#pragma once

#include <unordered_map>
#include "tools/gui/ViewState.h"
#include "ViewDetail.h"

namespace ndbl
{
    // forward declarations
    class ASTNodeProperty;
    class ASTNodeView;
    class ASTNode;
    class ASTVariable;
    struct ASTNodeSlot;

    /**
     * Simple struct to store a get_value view state
     */
    class ASTNodePropertyView
    {
    public:

        bool        show;
        bool        touched;

        ASTNodePropertyView(ASTNodeProperty*);

        bool             draw(ViewDetail); // return true when changed
        void             reset();
        ASTNodeProperty* get_property() const;
        ASTNode*         get_node() const;
        ASTNodeSlot*     get_connected_slot() const;
        ASTVariable*     get_connected_variable() const;
        bool             has_input_connected() const;

        const tools::ViewState*     state() const { return &_state; };
        tools::ViewState*           state() { return &_state; };
        const tools::BoxShape2D*    shape() const { return &_shape; };
        tools::BoxShape2D*          shape() { return &_shape; };
        tools::SpatialNode*         spatial_node() { return _shape.spatial_node(); };
        const tools::SpatialNode*   spatial_node()const  { return _shape.spatial_node(); };

    private: static float calc_input_width(const char* text);
    public:  static bool  draw_input(ASTNodePropertyView*, bool _compact_mode, const char* _override_label);
    public:  static bool  draw_all(const std::vector<ASTNodePropertyView*>&, ViewDetail);
    private:
        ASTNodeProperty*  _property;
        tools::ViewState  _state;
        tools::BoxShape2D _shape;
    };
}