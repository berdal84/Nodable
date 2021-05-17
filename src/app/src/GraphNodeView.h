#pragma once

#include <map>
#include <string>
#include <functional>
#include <AbstractCodeBlockNode.h>
#include <Function.h>

#include "mirror.h"
#include "Nodable.h"     // forward declarations
#include "NodeView.h"     // base class

namespace Nodable::app
{
    using namespace core;

	typedef struct {
        std::string                 label;
        std::function<Node *(void)> create_node_fct;
        FunctionSignature           function_signature;
	} FunctionMenuItem;

	class GraphNodeView: public NodeView
    {
	public:
	    GraphNodeView() = default;
		~GraphNodeView() = default;

		void    setOwner(Node*) override;
		void    updateViewConstraints();
		bool    draw() override ;
		bool    update() override;
		void    addContextualMenuItem(const std::string& _category, std::string _label, std::function<Node*(void)> _lambda, const FunctionSignature& _signature);
	private:
	    std::vector<ViewConstraint> constraints;
        [[nodiscard]] GraphNode* getGraphNode() const;
		std::multimap<std::string, FunctionMenuItem> contextualMenus;

		MIRROR_CLASS(GraphNodeView)
		(
			MIRROR_PARENT(NodeView)
        );

    };
}