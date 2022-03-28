#include <nodable/core/HeadlessNodeFactory.h>
#include <nodable/core/InstructionNode.h>
#include <nodable/core/VariableNode.h>
#include <nodable/core/LiteralNode.h>
#include <nodable/core/Language.h>
#include <nodable/core/InvokableComponent.h>
#include <nodable/core/Scope.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

using namespace Nodable;

InstructionNode* HeadlessNodeFactory::new_instr() const
{
    InstructionNode* instr_node = new InstructionNode(ICON_FA_CODE " Instr.");
    instr_node->set_short_label(ICON_FA_CODE " I.");
    m_post_process(instr_node);
    return instr_node;
}

VariableNode* HeadlessNodeFactory::new_variable(std::shared_ptr<const R::MetaType> _type, const std::string& _name, IScope *_scope) const
{
    // create
    auto* node = new VariableNode(_type);
    node->set_name(_name.c_str());

    if( _scope)
    {
        _scope->add_variable(node);
    }
    else
    {
        LOG_WARNING("HeadlessNodeFactory", "Variable %s has been created without defining its scope.\n", _name.c_str())
    }

    m_post_process(node);

    return node;
}

Node* HeadlessNodeFactory::new_operator(const InvokableOperator* _operator) const
{
    switch (_operator->get_operator_type() )
    {
        case InvokableOperator::Type::Binary:
            return new_binary_op(_operator);
        case InvokableOperator::Type::Unary:
            return new_unary_op(_operator);
        default:
            return nullptr;
    }
}

Node* HeadlessNodeFactory::new_binary_op(const InvokableOperator* _operator) const
{
    // Create a node with 2 inputs and 1 output
    auto node = new Node();

    setup_node_labels(node, _operator);

    const FunctionSignature* signature = _operator->get_signature();
    const auto args = signature->get_args();
    auto props   = node->props();
    Member* left   = props->add(k_lh_value_member_name, Visibility::Default, args[0].m_type, Way_In);
    Member* right  = props->add(k_rh_value_member_name, Visibility::Default, args[1].m_type, Way_In);
    Member* result = props->add(k_value_member_name, Visibility::Default, signature->get_return_type(), Way_Out);

    // Create ComputeBinaryOperation component and link values.
    auto binOpComponent = new InvokableComponent( _operator );
    binOpComponent->set_result(result);
    binOpComponent->set_l_handed_val(left);
    binOpComponent->set_r_handed_val(right);
    node->add_component(binOpComponent);

    m_post_process(node);

    return node;
}

void HeadlessNodeFactory::setup_node_labels(Node *_node, const InvokableOperator *_operator)
{
    _node->set_label(_operator->get_signature()->get_label());
    _node->set_short_label(_operator->get_short_identifier().c_str());
}

Node* HeadlessNodeFactory::new_unary_op(const InvokableOperator* _operator) const
{
    // Create a node with 2 inputs and 1 output
    auto node = new Node();

    setup_node_labels(node, _operator);

    const FunctionSignature* signature = _operator->get_signature();
    const auto args = signature->get_args();
    Properties* props = node->props();
    Member* left = props->add(k_lh_value_member_name, Visibility::Default, args[0].m_type, Way_In);
    Member* result = props->add(k_value_member_name, Visibility::Default, signature->get_return_type(), Way_Out);

    // Create ComputeBinaryOperation binOpComponent and link values.
    auto unaryOperationComponent = new InvokableComponent( _operator );
    unaryOperationComponent->set_result(result);
    unaryOperationComponent->set_l_handed_val(left);
    node->add_component(unaryOperationComponent);

    m_post_process(node);

    return node;
}

Node* HeadlessNodeFactory::new_function(const FunctionSignature* _signature) const
{
    // Create a node with 2 inputs and 1 output
    auto node = new Node();
    node->set_label(_signature->get_identifier() + "()");
    std::string str = _signature->get_label().substr(0, 2) + "..()";
    node->set_short_label(str.c_str());

    auto props = node->props();
    Member* result = props->add(k_value_member_name, Visibility::Default, _signature->get_return_type(), Way_Out);

    m_post_process(node);

    return node;
}

Node* HeadlessNodeFactory::new_function(const IInvokable* _function) const
{
    // Create a node with 2 inputs and 1 output
    auto node = new Node();
    node->set_label(_function->get_signature()->get_identifier() + "()");
    std::string str = _function->get_signature()->get_label().substr(0, 2) + "..()";
    node->set_short_label(str.c_str());

    auto props = node->props();
    Member* result = props->add(k_value_member_name, Visibility::Default, _function->get_signature()->get_return_type(), Way_Out);

    // Create ComputeBase binOpComponent and link values.
    auto functionComponent = new InvokableComponent( _function );
    functionComponent->set_result(result);

    // Arguments
    auto args = _function->get_signature()->get_args();
    for (size_t argIndex = 0; argIndex < args.size(); argIndex++)
    {
        std::string memberName = args[argIndex].m_name;
        auto member = props->add(memberName.c_str(), Visibility::Default, args[argIndex].m_type, Way_In); // create node input
        functionComponent->set_arg(argIndex, member); // link input to binOpComponent
    }

    node->add_component(functionComponent);
    m_post_process(node);
    return node;
}

Node* HeadlessNodeFactory::new_scope() const
{
    auto scope_node = new Node();
    std::string label = ICON_FA_CODE_BRANCH " Scope";
    scope_node->set_label(label);
    scope_node->set_short_label(ICON_FA_CODE_BRANCH " Sc.");

    scope_node->predecessor_slots().set_limit(std::numeric_limits<int>::max());
    scope_node->successor_slots().set_limit(1);

    auto* scope = new Scope();
    scope_node->add_component(scope);

    m_post_process(scope_node);

    return scope_node;
}

ConditionalStructNode* HeadlessNodeFactory::new_cond_struct() const
{
    auto cond_struct_node = new ConditionalStructNode();
    std::string label = ICON_FA_QUESTION " Condition";
    cond_struct_node->set_label(label);
    cond_struct_node->set_short_label(ICON_FA_QUESTION" Cond.");

    cond_struct_node->predecessor_slots().set_limit(std::numeric_limits<int>::max());
    cond_struct_node->successor_slots().set_limit(2); // true/false branches

    auto* scope = new Scope();
    cond_struct_node->add_component(scope);

    m_post_process(cond_struct_node);

    return cond_struct_node;
}

ForLoopNode* HeadlessNodeFactory::new_for_loop_node() const
{
    auto for_loop = new ForLoopNode();
    std::string label = ICON_FA_RECYCLE " For loop";
    for_loop->set_label(label);
    for_loop->set_short_label(ICON_FA_RECYCLE" For");

    for_loop->predecessor_slots().set_limit(std::numeric_limits<int>::max());
    for_loop->successor_slots().set_limit(1);

    auto* scope = new Scope();
    for_loop->add_component(scope);

    m_post_process(for_loop);

    return for_loop;
}

Node* HeadlessNodeFactory::new_program() const
{
    Node* prog = new_scope();
    prog->set_label(ICON_FA_FILE_CODE " Program");
    prog->set_short_label(ICON_FA_FILE_CODE " Prog.");

    m_post_process(prog);
    return prog;
}

Node* HeadlessNodeFactory::new_node() const
{
    auto node = new Node();
    m_post_process(node);
    return node;
}

LiteralNode* HeadlessNodeFactory::new_literal(std::shared_ptr<const R::MetaType> _type) const
{
    LiteralNode* node = new LiteralNode(_type);
    node->set_label("Literal");
    node->set_short_label("Lit.");
    m_post_process(node);
    return node;
}
