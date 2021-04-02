#pragma once

#include "Nodable.h" // for constants and forward declarations
#include "View.h"    // base class
#include <imgui/imgui.h>   // for ImVec2
#include <string>
#include <map>
#include "Member.h"
#include <mirror.h>

namespace Nodable
{
	/** We use this enum to identify all NodeView detail modes */
	enum class NodeViewDetail: unsigned short int
	{
		Minimalist  = 0,
		Essential   = 1,
		Exhaustive  = 2,
		Default     = Essential
	};	

	// forward declaration
	class Node;
    class ScopedCodeBlockNode;
    class CodeBlockNode;
    class GraphNode;

	/**
	 * Simple struct to store a member view state
	 */
	struct MemberView
    {
	    Member* member;

	    /** determine if input should be visible or not */
	    bool showInput;

	    /** false by default, will be true if user toggle showInput on/off */
	    bool touched;

	    /** Position in screen space */
	    ImVec2 screenPos;

	    explicit MemberView(Member* _member):
	        member(_member),
	        showInput(false),
	        touched(false)
        {
	        assert(_member != nullptr); // Member must be defined
        }

        /**
         * Reset the view
         */
        void reset()
        {
            touched = false;
            showInput = false;
        }
    };

	/**
	 * A class to abstract a constraint between some NodeView
	 */
	class ViewConstraint {
	public:
	    enum Type {
	        AlignOnBBoxTop,
	        AlignOnBBoxLeft,
	        MakeRowAndAlignOnBBoxTop,
	        FollowWithChildren,
	        Follow,
        };

	    ViewConstraint(Type _type);
	    void apply(float _dt);
	    void addSlave(NodeView*);
	    void addMaster(NodeView*);
	    ImVec2 offset;
	private:
	    Type type;

        std::vector<NodeView*> masters;
	    std::vector<NodeView*> slaves;

        static const float s_viewSpacing;
        static const float s_viewSpeed;
	};

	/**
	 * This class implement a view for Nodes using ImGui.
	 */
	class NodeView : public View
	{
	public:
		NodeView();
		~NodeView();

		/** override Component::setOwner(Node*) to extract some information from owner before to actually set it */
		void setOwner(Node* _node)override;

		/** Expose a member in the NodeView
		 * Way can only be Way_In or Way_Out */
		void exposeMember(Member*, Way );

		/** Draw the view at its position into the current window
		   Returns true if nod has been edited, false either */
		bool draw ()override;

		/** Should be called once per frame to update the view */
		bool update()override;

        /** Get top-left corner vector position */
		ImVec2 getPosition()const;

		/** Get the connector position of the specified member (by name) for its Way way (In or Out ONLY !) */
		ImVec2 getConnectorPosition(const Member *_member /*_name*/, Way /*_connection*/_way)const;

		/** Set a new position (top-left corner relative) vector to this view */
		void  setPosition(ImVec2);

		/** Apply a translation vector to the view's position */
		void translate(ImVec2, bool _recurse = false);

		void translateTo(ImVec2 desiredPos, float _factor, bool _recurse = false);
		/** Arrange input nodes recursively while keeping this node position unchanged.
		 *  Note: Some input connected Nodes can stay fixed if they are pinned. */
		void arrangeRecursively(bool _smoothly = true);

		/** Draw advance properties (components, dirty state, etc.) */
		void drawAdvancedProperties();

		/** Get the node label
		 * Note: depends on s_viewDetail, can be just an ICON_FA (4 char) or an ICON_FA + a label */
        std::string getLabel();

        /** Compute the bounding rectangle of this view */
        ImRect getRect(bool _recursive = false, bool _ignorePinned = true, bool _ignoreMultiConstrained = true, bool _ignoreSelf = false);

        void addConstraint(ViewConstraint _constraint);
        void applyConstraints(float _dt);
        void clearConstraints();

        /** Arrange input nodes recursively while keeping this node position unchanged.
         *  Note: Some input connected Nodes can stay fixed if they are pinned. */
		static void ArrangeRecursively(NodeView* _view, bool _smoothly = true);

		/** Set a NodeView as selected.
		 * Note: Only a single view can be selected at the same time */
		static void SetSelected(NodeView*);

		/** Return a pointer to the selected view or nullptr if no view are selected */
		static NodeView* GetSelected();

		/** Return a pointer to the dragged Connector or nullptr if no connectors are currently dragged */
		static const Connector*  GetDraggedConnector() { return s_draggedConnector; }
		static void              ResetDraggedConnector() { s_draggedConnector = nullptr; }
		static void              StartDragConnector(const Connector* _connector) {
			if(s_draggedNode == nullptr)
				s_draggedConnector = _connector;
		};

		/** Return a pointer to the hovered member or nullptr if no member is dragged */
		static const Connector*  GetHoveredConnector() { return s_hoveredConnector; }

		/** Return true if the given NodeView is selected */
		static bool       IsSelected(NodeView*);

		/** Start to drag a NodeView. Only a single view can be dragged at the same time. */
		static void       StartDragNode(NodeView*);

		/** Return true if any NodeView is currently dragged */
		static bool		  IsAnyDragged();

		/** Return true if a given NodeView is contained by a given Rectangle */
		static bool       IsInsideRect(NodeView*, ImRect);

		/** Move instantly a given NodeView to be inside a given Rectangle */
		static void       ConstraintToRect(NodeView*, ImRect);

		/** Return a pointer to the dragged NodeView or nullptr if no view are dragged */
		static NodeView*  GetDragged();

		/**
		 * Draw the ImGui input to modify a given Member.
		 *
		 * @param _member
		 * @param _label by default nullptr, you can override it to change input label.
		 * @return
		 */
        static bool DrawMemberInput(Member *_member, const char* _label = nullptr);

        /**
         * Draw a NodeView as a properties panel.
         * All input, output, components, parent container and other information are draw in a column.
         * @param _view
         */
        static void DrawNodeViewAsPropertiesPanel(NodeView *_view);

        /** set globally the draw detail of nodes */
        static NodeViewDetail s_viewDetail;

        /** Change view detail globally */
        static void SetDetail(NodeViewDetail _viewDetail);

    private:
        /** Update function that takes a specific delta time (can be hacked by sending a custom value) */
        virtual bool update(float _deltaTime);

        /**	Draw a Node Member at cursor position.
			Returns true if Member's value has been modified, false either */
		bool drawMemberView(MemberView *_memberView);

		/** Draw all member connector(s). Can be 0, 1 or 2 depending on member's connectorWay (cf enum Way_) */
        void drawMemberConnectors(Member *_member, float _connectorRadius);

        /** Draw a single connector at a specific position into the IMGuiDrawList */
		void drawConnector(ImVec2& , const Connector* , ImDrawList*, float _connectorRadius);

		/** Check if a Member is exposed (as an input or output) */
        bool isMemberExposed(const Member *_member)const;

        /** Get a MemberView given a Member */
        const MemberView* getMemberView(const Member* _member)const;

        /** position in pixels (center of the NodeView) */
		ImVec2          position;

		/** Size in pixels */
		ImVec2          size;

        /** global transparency of this view */
		float           opacity;

		/* @deprecated */
		bool            forceMemberInputVisible;

		/** when true, the NodeView is pinned to the document and do not follow it's connected Node */
		bool            pinned;
		float           borderRadius;
		ImColor         borderColorSelected;
		std::vector<MemberView*> exposedInputsMembers;
		std::vector<MemberView*> exposedOutputMembers;
        std::map<const Member*, MemberView*> exposedMembers;

        /** The constraints this view is subject to */
        std::vector<ViewConstraint> constraints;

        /** pointer to the currently selected NodeView. */
		static NodeView* s_selected;

        /** pointer to the currently dragged NodeView. */
		static NodeView* s_draggedNode;

		static const Connector* s_draggedConnector;
		static const Connector* s_hoveredConnector;

		/** Minimum size for an input field */
        static const float s_memberInputSizeMin;

        /** Size of the small button to toggle input visibility on/off */
        static const ImVec2 s_memberInputToggleButtonSize;

        /** to store all instances */
        static std::vector<NodeView*> s_instances;

        // Reflect this class
        MIRROR_CLASS(NodeView)(
                MIRROR_PARENT(View)); // I only need to know the parent
        ImVec2 getScreenPos();

        bool isPinned() const;

        ImVec2 getSize() const;

        static ImRect GetRect(
                const std::vector<NodeView *>&,
                bool _recursive = false,
                bool _ignorePinned = true,
                bool _ignoreMultiConstrained = true);

        void addForceToTranslateTo(ImVec2 desiredPos, float _factor, bool _recurse = false);

        void addForce(ImVec2 force, bool _recurse = false);

        // sum of forces applied to this
        ImVec2 forces;

        void applyForces(bool _recurse = false);

        void setChildrenVisible(bool _visible, bool _recursive = false);
        bool childrenVisible = true;
        std::vector<NodeView *> getChildren();

        void setInputsVisible(bool _visible, bool _recursive = false);

        std::vector<NodeView *> getInputs();

        bool hasOnlyASingleOutputVisible();

        std::vector<NodeView *> getOutputs();
    };
}
