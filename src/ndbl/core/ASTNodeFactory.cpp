#include "ASTNodeFactory.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include <utility>

#include "tools/core/reflection/reflection"

#include "ASTForLoop.h"
#include "ASTIf.h"
#include "ASTFunctionCall.h"
#include "ASTLiteral.h"
#include "ASTUtils.h"
#include "ASTScope.h"
#include "ASTVariable.h"
#include "ASTWhileLoop.h"

using namespace ndbl;
using namespace tools;

static ASTNodeFactory* g_node_factory{nullptr };

ASTNodeFactory::ASTNodeFactory()
: m_post_process([](ASTNode* _node){})
, m_post_process_is_overrided(false)
{}

ASTVariable* ASTNodeFactory::create_variable(const TypeDescriptor* _type, const std::string& _name) const
{
    // create
    auto ast_node = new ASTVariable();
    ast_node->init(_type, _name.c_str());
    m_post_process(ast_node);

    return ast_node;
}

ASTVariableRef* ASTNodeFactory::create_variable_ref() const
{
    auto  ast_node = new ASTVariableRef();
    ast_node->init();
    m_post_process(ast_node);
    return ast_node;
}

ASTFunctionCall* ASTNodeFactory::create_function(const FunctionDescriptor& _func_type, ASTNodeType _node_type) const
{
    auto* ast_node = new ASTFunctionCall();
    ASSERT(_node_type == ASTNodeType_OPERATOR || _node_type == ASTNodeType_FUNCTION );
    ast_node->init(_node_type, _func_type);
    m_post_process(ast_node);
    return ast_node;
}

ASTIf* ASTNodeFactory::create_cond_struct() const
{
    auto* ast_node = new ASTIf();
    ast_node->init("If");

    m_post_process(ast_node);

    return ast_node;
}

ASTForLoop* ASTNodeFactory::create_for_loop() const
{
    auto* ast_node = new ASTForLoop();
    ast_node->init("For");

    m_post_process(ast_node);

    return ast_node;
}

ASTWhileLoop* ASTNodeFactory::create_while_loop() const
{
    auto* ast_node = new ASTWhileLoop();
    ast_node->init("While");

    m_post_process(ast_node);

    return ast_node;
}

ASTNode* ASTNodeFactory::create_scope() const
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_SCOPE, "Scope");
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT | SlotFlag_IS_INTERNAL, 1);
    ast_node->init_internal_scope();
    m_post_process(ast_node);
    return ast_node;
}

ASTNode* ASTNodeFactory::create_root_scope() const
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_SCOPE, ICON_FA_ARROW_ALT_CIRCLE_DOWN " BEGIN");
    // ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY); nothing can be before...
    // ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1); nothing after either...
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT | SlotFlag_IS_INTERNAL, 1); // ...but something inside!
    ast_node->init_internal_scope();
    m_post_process(ast_node);
    return ast_node;
}

ASTNode* ASTNodeFactory::create_node() const
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_DEFAULT, "");
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY);
    m_post_process(ast_node);
    return ast_node;
}

ASTLiteral* ASTNodeFactory::create_literal(const TypeDescriptor *_type) const
{
    auto* ast_node = new ASTLiteral();
    ast_node->init(_type, "Literal");
    m_post_process(ast_node);
    return ast_node;
}

void ASTNodeFactory::override_post_process_fct(ASTNodeFactory::PostProcessFct _function)
{
    VERIFY(m_post_process_is_overrided == false, "Cannot override post process function more than once." );
    m_post_process_is_overrided = true;
    m_post_process              = std::move(_function);
}

ASTNode* ASTNodeFactory::create_empty_instruction()const
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_EMPTY_INSTRUCTION, ";");

    // Token will be/or not overriden as a Token_t::end_of_instruction by the parser
    ast_node->value()->set_token({ASTToken_t::ignore});

    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN , ASTNodeSlot::MAX_CAPACITY);
    ast_node->add_slot(ast_node->value(), SlotFlag_OUTPUT  , 1);

    m_post_process(ast_node);

    return ast_node;
}

ASTNodeFactory* ndbl::get_node_factory()
{
    return g_node_factory;
}

ASTNodeFactory* ndbl::init_node_factory()
{
    ASSERT(g_node_factory == nullptr); // singleton
    g_node_factory = new ASTNodeFactory();
    return g_node_factory;
}

void ndbl::shutdown_node_factory(ASTNodeFactory* _factor)
{
    ASSERT(g_node_factory == _factor);  // singleton
    ASSERT(g_node_factory != nullptr);
    delete g_node_factory;
    g_node_factory = nullptr;
}

