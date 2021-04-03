#include "GraphNode.h"

#include <cstring>      // for strcmp
#include <algorithm>    // for std::find_if
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "Core/Log.h"
#include "Core/Wire.h"
#include "Language/Common/Parser.h"
#include "Node/Node.h"
#include "Node/VariableNode.h"
#include "Component/ComputeBinaryOperation.h"
#include "Component/ComputeUnaryOperation.h"
#include "Component/WireView.h"
#include "Component/DataAccess.h"
#include "Component/NodeView.h"
#include "Component/GraphNodeView.h"
#include "Node/NodeTraversal.h"
#include "Node/InstructionNode.h"
#include "Node/CodeBlockNode.h"
#include "Node/ScopedCodeBlockNode.h"
#include "Node/ConditionalStructNode.h"

using namespace Nodable;

ImVec2 GraphNode::ScopeViewLastKnownPosition = ImVec2(-1, -1); // draft try to store node position

GraphNode::~GraphNode()
{
	clear();
}

void GraphNode::clear()
{

	// Store the Result node position to restore it later
	// TODO: handle multiple results
	if ( scope && scope->hasInstructions() )
	{
        auto view = scope->getComponent<NodeView>();
        GraphNode::ScopeViewLastKnownPosition = view->getPosition();
    }

	LOG_VERBOSE( "GraphNode", "=================== clear() ==================\n");

	if ( !nodeRegistry.empty() )
	{
        for ( auto i = nodeRegistry.size(); i > 0; i--)
        {
            Node* node = nodeRegistry[i-1];
            LOG_VERBOSE("GraphNode", "remove and delete: %s \n", node->getLabel() );
            deleteNode(node);
        }
	}

	wireRegistry.clear();
    nodeRegistry.clear();
	relationRegistry.clear();
    scope = nullptr;

    if ( auto view = this->getComponent<GraphNodeView>())
    {
        view->clearConstraints();
    }

    LOG_VERBOSE("GraphNode", "===================================================\n");
}

UpdateResult GraphNode::update()
{
    /*
        1 - Delete flagged Nodes
    */
    {
        auto nodeIndex = nodeRegistry.size();

        while (nodeIndex > 0)
        {
            nodeIndex--;
            auto node = nodeRegistry.at(nodeIndex);

            if (node->needsToBeDeleted())
            {
                this->deleteNode(node);
            }

        }
    }

    /*
       2 - update view constraints
     */
    if ( this->isDirty())
    {
        if (auto view = getComponent<GraphNodeView>() )
        {
            view->updateViewConstraints();
        }
    }

	/*
	    3 - Update all Nodes
    */
	UpdateResult result;
	if (this->scope)
    {
        NodeTraversal nodeTraversal;
        if ( nodeTraversal.update(this->scope) == Result::Success )
        {
            if ( nodeTraversal.getStats().traversed.empty() )
            {
                result = UpdateResult::SuccessWithoutChanges;
            } else {
                nodeTraversal.logStats();
                result = UpdateResult::Success;
            }
        }
        else
        {
            result =  UpdateResult::Failed;
        }
    }
	else
    {
        result = UpdateResult::SuccessWithoutChanges;
    }

    this->setDirty(false);
    return result;
}

void GraphNode::registerNode(Node* _node)
{
	this->nodeRegistry.push_back(_node);
    _node->setParentGraph(this);
    LOG_VERBOSE("GraphNode", "registerNode %s (%s)\n", _node->getLabel(), _node->getClass()->getName());
}

void GraphNode::unregisterNode(Node* _node)
{
    auto found = std::find(nodeRegistry.begin(), nodeRegistry.end(), _node);
    nodeRegistry.erase(found);

    // check if nothing if left
    auto relationFound = std::find_if(
            relationRegistry.begin(),
            relationRegistry.end(),
            [&_node](Relation& rel)->bool {
                return rel.second.first == _node || rel.second.second == _node;
            });

    if ( relationFound != relationRegistry.end())
    {
        NODABLE_ASSERT(false); // Check if you remove all relations before to destroy
    }
}

VariableNode* GraphNode::findVariable(std::string _name)
{
	return scope->findVariable(_name);
}

InstructionNode* GraphNode::newInstruction()
{
    // create
	auto instructionNode = new InstructionNode(ICON_FA_CODE " Instr.");
    instructionNode->addComponent(new NodeView);
    instructionNode->setShortLabel(ICON_FA_CODE);

    // register
    this->registerNode(instructionNode);

	return instructionNode;
}

InstructionNode* GraphNode::appendInstruction()
{
    std::string eol = language->getSerializer()->serialize(TokenType::EndOfLine);

    // add to code block
    if ( scope->getChildren().empty())
    {
        connect( newCodeBlock(), scope,RelationType::IS_CHILD_OF);
    }
    else
    {
        // insert an eol
        InstructionNode* lastInstruction = scope->getLastInstruction();
        lastInstruction->endOfInstructionToken->suffix += eol;
    }

    auto block = scope->getLastCodeBlock()->as<CodeBlockNode>();
    auto newInstructionNode = newInstruction();
    this->connect(newInstructionNode, block,  RelationType::IS_CHILD_OF);

    // Initialize (since it is a manual creation)
    Token* token = new Token(TokenType::EndOfInstruction);
    token->suffix = eol;
    newInstructionNode->endOfInstructionToken = token;

    return newInstructionNode;
}

VariableNode* GraphNode::newVariable(std::string _name, ScopedCodeBlockNode* _scope)
{
    // create
	auto node = new VariableNode();
	node->addComponent( new NodeView);
	node->setName(_name.c_str());

	// register
    this->registerNode(node);

    if( _scope)
    {
        _scope->addVariable(node);
    }
    else
    {
        LOG_WARNING("GraphNode", "You create a variable without defining its scope.");
    }

	return node;
}

VariableNode* GraphNode::newNumber(double _value)
{
	auto node = new VariableNode();
	node->addComponent( new NodeView);
	node->set(_value);
    this->registerNode(node);
	return node;
}

VariableNode* GraphNode::newNumber(const char* _value)
{
	auto node = new VariableNode();
	node->addComponent( new NodeView);
	node->set(std::stod(_value));
    this->registerNode(node);
	return node;
}

VariableNode* GraphNode::newString(const char* _value)
{
	auto node = new VariableNode();
	node->addComponent( new NodeView);
	node->set(_value);
    this->registerNode(node);
	return node;
}

Node* GraphNode::newOperator(const Operator* _operator)
{
    switch ( _operator->getType() )
    {
        case Operator::Type::Binary:
            return newBinOp(_operator);
        case Operator::Type::Unary:
            return newUnaryOp(_operator);
        default:
            return nullptr;
    }
}

Node* GraphNode::newBinOp(const Operator* _operator)
{
	// Create a node with 2 inputs and 1 output
	auto node = new Node();
	auto signature = _operator->signature;
	node->setLabel(signature.getLabel());
    node->setShortLabel(signature.getLabel().substr(0, 4).c_str());

    const auto args = signature.getArgs();
	const Semantic* semantic = language->getSemantic();
	auto left   = node->add("lvalue", Visibility::Default, semantic->tokenTypeToType(args[0].type), Way_In);
	auto right  = node->add("rvalue", Visibility::Default, semantic->tokenTypeToType(args[1].type), Way_In);
	auto result = node->add("result", Visibility::Default, semantic->tokenTypeToType(signature.getType()), Way_Out);

	// Create ComputeBinaryOperation component and link values.
	auto binOpComponent = new ComputeBinaryOperation(_operator, language);	
	binOpComponent->setResult(result);	
	binOpComponent->setLValue( left );	
	binOpComponent->setRValue(right);
	node->addComponent(binOpComponent);

	// Create a NodeView component
	node->addComponent(new NodeView());

	// Add to this container
    this->registerNode(node);
		
	return node;
}

Node* GraphNode::newUnaryOp(const Operator* _operator)
{
	// Create a node with 2 inputs and 1 output
	auto node = new Node();
	auto signature = _operator->signature;
	node->setLabel(signature.getLabel());
    node->setShortLabel(signature.getLabel().substr(0, 4).c_str());
	const auto args = signature.getArgs();
    const Semantic* semantic = language->getSemantic();
	auto left = node->add("lvalue", Visibility::Default, semantic->tokenTypeToType(args[0].type), Way_In);
	auto result = node->add("result", Visibility::Default, semantic->tokenTypeToType(signature.getType()), Way_Out);

	// Create ComputeBinaryOperation binOpComponent and link values.
	auto unaryOperationComponent = new ComputeUnaryOperation(_operator, language);
	unaryOperationComponent->setResult(result);
	unaryOperationComponent->setLValue(left);
	node->addComponent(unaryOperationComponent);

	// Create a NodeView Component
	node->addComponent(new NodeView());

	// Add to this container
    this->registerNode(node);

	return node;
}

Node* GraphNode::newFunction(const Function* _function)
{
	// Create a node with 2 inputs and 1 output
	auto node = new Node();
	node->setLabel(_function->signature.getIdentifier() + "()");
	node->setShortLabel("f(x)");
    const Semantic* semantic = language->getSemantic();
	node->add("result", Visibility::Default, semantic->tokenTypeToType(_function->signature.getType()), Way_Out);

	// Create ComputeBase binOpComponent and link values.
	auto functionComponent = new ComputeFunction(_function, language);
	functionComponent->setResult(node->get("result"));

	// Arguments
	auto args = _function->signature.getArgs();
	for (size_t argIndex = 0; argIndex < args.size(); argIndex++) {
		std::string memberName = args[argIndex].name;
		auto member = node->add(memberName.c_str(), Visibility::Default, semantic->tokenTypeToType(args[argIndex].type), Way_In); // create node input
		functionComponent->setArg(argIndex, member); // link input to binOpComponent
	}	
	
	node->addComponent(functionComponent);
	node->addComponent(new NodeView());

    this->registerNode(node);

	return node;
}


Wire* GraphNode::newWire()
{
	Wire* wire = new Wire();
	wire->addComponent(new WireView);	
	return wire;
}

void GraphNode::arrangeNodeViews()
{
    if ( scope ) {
        if (auto scopeView = scope->getComponent<NodeView>()) {
            bool hasKnownPosition = GraphNode::ScopeViewLastKnownPosition.x != -1 &&
                                    GraphNode::ScopeViewLastKnownPosition.y != -1;

            if ( this->hasComponent<View>()) {
                auto view = this->getComponent<View>();

                if (hasKnownPosition) {                                 /* if result node had a position stored, we restore it */
                    scopeView->setPosition(GraphNode::ScopeViewLastKnownPosition);
                }

                auto rect = view->getVisibleRect();
                if (!NodeView::IsInsideRect(scopeView, rect)) {
                    ImVec2 defaultPosition = rect.GetCenter();
                    defaultPosition.x += rect.GetWidth() * 1.0f / 6.0f;
                    scopeView->setPosition(defaultPosition);
                }
            }
        }

        scope->getComponent<NodeView>()->arrangeRecursively(false);
    }
}

GraphNode::GraphNode(const Language* _language)
    :
    language(_language),
    scope(nullptr)
{
	this->clear();
}

CodeBlockNode *GraphNode::newCodeBlock()
{
    auto codeBlockNode = new CodeBlockNode();
    std::string label = ICON_FA_CODE " Block " + std::to_string(this->scope->getChildren().size());
    codeBlockNode->setLabel(label);
    codeBlockNode->setShortLabel(ICON_FA_CODE "Bl");
    codeBlockNode->addComponent(new NodeView);

    this->registerNode(codeBlockNode);

    return codeBlockNode;
}

void GraphNode::deleteNode(Node* _node)
{
    // disconnect wires
    auto wires = _node->getWires();
    for (auto it = wires.rbegin(); it != wires.rend(); it++)
    {
        unregisterWire(*it);
        disconnect(*it);
     }

    // disconnect parent->node relation
    Node* parent = _node->getParent();
    if ( parent )
    {
        disconnect(_node, parent, RelationType::IS_CHILD_OF);
    }

    // disconnect children->node relations
    for(Node* eachChild : _node->getChildren() )
    {
        disconnect(eachChild, _node, RelationType::IS_CHILD_OF);
    }

    // unregister and delete
    unregisterNode(_node);
    delete _node;
}

bool GraphNode::hasInstructionNodes()
{
    return scope && scope->hasInstructions();
}

Wire *GraphNode::connect(Member* _from, Member* _to)
{
    Wire* wire = nullptr;

    /*
     * If _from has no owner _to can digest it, no Wire neede in that case.
     */
    if (_from->getOwner() == nullptr)
    {
        _to->digest(_from);

    }
    else
    {
        _to->setInputMember(_from);
        auto targetNode = _to->getOwner()->as<Node>();
        auto sourceNode = _from->getOwner()->as<Node>();

        // Link wire to members
        auto sourceContainer = sourceNode->getParentGraph();
        wire = sourceContainer->newWire();

        wire->setSource(_from);
        wire->setTarget(_to);

        targetNode->addWire(wire);
        sourceNode->addWire(wire);

        connect(sourceNode, targetNode, RelationType::IS_INPUT_OF);

        // TODO: move this somewhere else
        // (transfer prefix/suffix)
        auto fromToken = _from->getSourceToken();
        if (fromToken) {
            if (!_to->getSourceToken()) {
                _to->setSourceToken(new Token(fromToken->type, "", fromToken->charIndex));
            }

            auto toToken = _to->getSourceToken();
            toToken->suffix = fromToken->suffix;
            toToken->prefix = fromToken->prefix;
            fromToken->suffix = "";
            fromToken->prefix = "";
        }
    }

    if ( wire != nullptr )
    {
        registerWire(wire);
    }

    this->setDirty();

    return wire;
}

void GraphNode::disconnect(Wire *_wire)
{
    deleteWire(_wire);
}

void GraphNode::registerWire(Wire* _wire)
{
    wireRegistry.push_back(_wire);
}

void GraphNode::unregisterWire(Wire* _wire)
{
    auto found = std::find(wireRegistry.begin(), wireRegistry.end(), _wire);
    if (found != wireRegistry.end() )
    {
        wireRegistry.erase(found);
    }
    else
    {
        LOG_WARNING("GraphNode", "Unable to unregister wire\n");
    }
}

void GraphNode::connect(Member* _source, InstructionNode* _target)
{
    connect(_source, _target->getValue());
}


void GraphNode::connect(Node *_source, Node *_target, RelationType _relationType)
{
    switch ( _relationType )
    {
        case RelationType::IS_CHILD_OF:
            _target->addChild(_source);
            _source->setParent(_target);
            break;

        case RelationType::IS_INPUT_OF:
            _target->addInput(_source);
            _source->addOutput(_target);
            break;

        default:
            NODABLE_ASSERT(false); // This connection type is not yet implemented
    }

    this->relationRegistry.emplace(_relationType, std::pair(_source, _target));
    this->setDirty();
}

void GraphNode::disconnect(Node *_source, Node *_target, RelationType _relationType)
{
    NODABLE_ASSERT(_source && _target);

    // find relation
    Relation pair{_relationType, {_source, _target}};
    auto relation = std::find(relationRegistry.begin(), relationRegistry.end(), pair);

    if(relation == relationRegistry.end())
        return;

    // disconnect effectively
    switch ( _relationType )
    {
        case RelationType::IS_CHILD_OF:
            _target->removeChild(_source);
            _source->setParent(nullptr);
            break;

        case RelationType::IS_INPUT_OF:
            _target->removeInput(_source);
            _source->removeOutput(_target);
            break;

        default:
            NODABLE_ASSERT(false); // This connection type is not yet implemented
    }

    // remove relation
    relationRegistry.erase(relation);

    this->setDirty();
}

void GraphNode::deleteWire(Wire *_wire)
{
    _wire->getTarget()->setInputMember(nullptr);

    auto targetNode = _wire->getTarget()->getOwner()->as<Node>();
    auto sourceNode = _wire->getSource()->getOwner()->as<Node>();

    targetNode->removeWire(_wire);
    sourceNode->removeWire(_wire);

    disconnect(sourceNode, targetNode, RelationType::IS_INPUT_OF);

    NodeTraversal traversal;
    traversal.setDirty(targetNode);

    delete _wire;
}

ScopedCodeBlockNode *GraphNode::newScopedCodeBlock()
{
    auto scopeNode = new ScopedCodeBlockNode();
    std::string label = ICON_FA_CODE_BRANCH " Scope";
    scopeNode->setLabel(label);
    scopeNode->setShortLabel(ICON_FA_CODE_BRANCH " Scop.");
    scopeNode->addComponent(new NodeView());
    this->registerNode(scopeNode);
    return scopeNode;
}

ConditionalStructNode *GraphNode::newConditionalStructure()
{
    auto scopeNode = new ConditionalStructNode();
    std::string label = ICON_FA_QUESTION " Condition";
    scopeNode->setLabel(label);
    scopeNode->setShortLabel(ICON_FA_QUESTION" Cond.");
    scopeNode->addComponent(new NodeView());
    this->registerNode(scopeNode);
    return scopeNode;
}

ScopedCodeBlockNode *GraphNode::newProgram() {
    this->clear();
    this->scope = this->newScopedCodeBlock();
    return this->scope;
}
