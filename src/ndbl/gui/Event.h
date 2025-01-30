#pragma once
#include <utility>

#include "tools/core/EventManager.h"

#include "ndbl/core/Graph.h"

#include "Event.h"
#include "FrameMode.h"
#include "ASTNodeSlotView.h"

namespace ndbl
{
    // forward declaration
    class ASTNodeView;
    using tools::Vec2;
    using tools::FunctionDescriptor;

    enum EventID_ : tools::EventID
    {
        EventID_DELETE_NODE = tools::EventID_USER_DEFINED, // operation on nodes
        EventID_ARRANGE_NODE,
        EventID_SELECT_NEXT,
        EventID_TOGGLE_FOLDING,
        EventID_REQUEST_CREATE_NODE,
        EventID_REQUEST_CREATE_BLOCK,
        EventID_REQUEST_FRAME_SELECTION,
        EventID_MOVE_SELECTION,
        EventID_TOGGLE_ISOLATION_FLAGS,
        EventID_SLOT_DROPPED,
        EventID_SLOT_DISCONNECT_ALL,
        EventID_SELECTION_CHANGE,
        EventID_DELETE_EDGE,
        EventID_RESET_GRAPH,
    };

    using Event_ToggleIsolationFlags = tools::Event<EventID_TOGGLE_ISOLATION_FLAGS>;
    using Event_MoveSelection        = tools::Event<EventID_MOVE_SELECTION>;

    class GraphView;
    struct EventPayload_FrameNodeViews
    {
        FrameMode  mode;
        EventPayload_FrameNodeViews(FrameMode mode)
        : mode(mode)
        {}
    };
    using Event_FrameSelection = tools::Event<EventID_REQUEST_FRAME_SELECTION, EventPayload_FrameNodeViews>;

    struct EventPayload_SlotPair {
        ASTNodeSlot* first;
        ASTNodeSlot* second;
        EventPayload_SlotPair(ASTNodeSlot* first = {}, ASTNodeSlot* second = {})
        : first(first)
        , second(second)
        {}
    };
    using Event_SlotDisconnectAll = tools::Event<EventID_SLOT_DISCONNECT_ALL, EventPayload_SlotPair>;
    using Event_SlotDropped       = tools::Event<EventID_SLOT_DROPPED, EventPayload_SlotPair>;

    struct EventPayload_Node
    {
        ASTNode* node;
    };
    using Event_DeleteEdge  = tools::Event<EventID_DELETE_EDGE, EventPayload_SlotPair>;
    using Event_DeleteSelection  = tools::Event<EventID_DELETE_NODE, EventPayload_Node>;
    using Event_ArrangeSelection     = tools::Event<EventID_ARRANGE_NODE>;
    using Event_SelectNext  = tools::Event<EventID_SELECT_NEXT, EventPayload_Node>;

    enum ToggleFoldingMode
    {
        NON_RECURSIVELY = 0,
        RECURSIVELY     = 1,
    };
    struct EventPayload_ToggleFoldingEvent
    {
        ToggleFoldingMode mode;
    };
    using Event_ToggleFolding = tools::Event<EventID_TOGGLE_FOLDING, EventPayload_ToggleFoldingEvent>;

    struct EventPayload_CreateNode
    {
        CreateNodeType            node_type;          // The note type to create
        const FunctionDescriptor* node_signature;     // The signature of the node that must be created
        ASTNodeSlotView*          active_slotview;    // The slot view being dragged.
        Graph*                    graph;              // The graph to create the node into
        Vec2                      desired_screen_pos; // The desired position for the new node view

        explicit EventPayload_CreateNode(CreateNodeType node_type )
        : node_type(node_type)
        , node_signature(nullptr)
        , active_slotview(nullptr)
        , graph(nullptr)
        {}

        EventPayload_CreateNode(CreateNodeType node_type, const tools::FunctionDescriptor* signature )
        : node_type(node_type)
        , node_signature(signature)
        {}
    };
    using Event_CreateNode  = tools::Event<EventID_REQUEST_CREATE_NODE, EventPayload_CreateNode>;

}// namespace ndbl
