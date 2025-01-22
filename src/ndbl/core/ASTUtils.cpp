#include "ASTUtils.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "ASTForLoop.h"
#include "ASTFunctionCall.h"
#include "ASTIf.h"
#include "ASTLiteral.h"
#include "ASTNode.h"
#include "ASTScope.h"
#include "ASTSlotLink.h"
#include "ASTUtils.h"
#include "ASTVariable.h"
#include "ASTVariableRef.h"
#include "ASTWhileLoop.h"

using namespace ndbl;
using namespace tools;

ASTVariable* ASTUtils::create_variable(const TypeDescriptor* _type, const std::string& _name)
{
    // create
    auto ast_node = new ASTVariable();
    ast_node->init(_type, _name.c_str());
    return ast_node;
}

ASTVariableRef* ASTUtils::create_variable_ref()
{
    auto ast_node = new ASTVariableRef();
    ast_node->init();
    return ast_node;
}

ASTFunctionCall* ASTUtils::create_function(const FunctionDescriptor& _func_type, ASTNodeType _node_type)
{
    auto* ast_node = new ASTFunctionCall();
    ASSERT(_node_type == ASTNodeType_OPERATOR || _node_type == ASTNodeType_FUNCTION );
    ast_node->init(_node_type, _func_type);
    return ast_node;
}

ASTIf* ASTUtils::create_cond_struct()
{
    auto* ast_node = new ASTIf();
    ast_node->init("If");
    return ast_node;
}

ASTForLoop* ASTUtils::create_for_loop()
{
    auto* ast_node = new ASTForLoop();
    ast_node->init("For");
    return ast_node;
}

ASTWhileLoop* ASTUtils::create_while_loop()
{
    auto* ast_node = new ASTWhileLoop();
    ast_node->init("While");
    return ast_node;
}

ASTNode* ASTUtils::create_scope()
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_SCOPE, "Scope");
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT | SlotFlag_IS_INTERNAL, 1);
    ast_node->init_internal_scope();

    return ast_node;
}

ASTNode* ASTUtils::create_root_scope()
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_SCOPE, ICON_FA_ARROW_ALT_CIRCLE_DOWN " BEGIN");
    // ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY); nothing can be before...
    // ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1); nothing after either...
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT | SlotFlag_IS_INTERNAL, 1); // ...but something inside!
    ast_node->init_internal_scope();

    return ast_node;
}

ASTNode* ASTUtils::create_node()
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_DEFAULT, "");
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN, ASTNodeSlot::MAX_CAPACITY);
    return ast_node;
}

ASTLiteral* ASTUtils::create_literal(const TypeDescriptor *_type)
{
    auto* ast_node = new ASTLiteral();
    ast_node->init(_type, "Literal");
    return ast_node;
}

ASTNode* ASTUtils::create_empty_instruction()
{
    auto* ast_node = new ASTNode();
    ast_node->init(ASTNodeType_EMPTY_INSTRUCTION, ";");

    // Token will be/or not overriden as a Token_t::end_of_instruction by the parser
    ast_node->value()->set_token({ASTToken_t::ignore});

    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_OUT, 1);
    ast_node->add_slot(ast_node->value(), SlotFlag_FLOW_IN , ASTNodeSlot::MAX_CAPACITY);
    ast_node->add_slot(ast_node->value(), SlotFlag_OUTPUT  , 1);

    return ast_node;
}

std::vector<ASTNode*> ASTUtils::get_adjacent_nodes(const ASTNode* _node, SlotFlags _flags)
{
    std::vector<ASTNode*> result;
    for ( ASTNodeSlot* slot : _node->filter_slots(_flags ) )
    {
        for( const ASTNodeSlot* adjacent : slot->adjacent() )
        {
            result.emplace_back(adjacent->node );
        }
    }
    return result;
}

ASTNode* ASTUtils::adjacent_node_at(const ASTNode* _node, SlotFlags _flags, u8_t _pos)
{
    if ( ASTNodeSlot* adjacent_slot = _node->find_adjacent_at(_flags, _pos ) )
    {
        return adjacent_slot->node;
    }
    return {};
}

bool ASTUtils::is_instruction(const ASTNode* node)
{
    if ( is_connected_to_codeflow(node) )
        return true;
    if (node->type() == ASTNodeType_VARIABLE )
        return true;
    return false;
}

bool ASTUtils::is_connected_to_codeflow(const ASTNode *node)
{
    if (node->flow_inputs().size() )
        return true;
    if (node->flow_outputs().size() )
        return true;
    return false;
}

bool ASTUtils::can_be_instruction(const ASTNode* node)
{
    // TODO: handle case where a variable has inputs/outputs but not connected to the code flow
    return node->slot_count(SlotFlag_TYPE_FLOW) > 0 && node->inputs().empty() && node->outputs().empty();
}

bool ASTUtils::is_unary_operator(const ASTNode* node)
{
    if (node->type() == ASTNodeType_OPERATOR )
        if (static_cast<const ASTFunctionCall *>(node)->get_func_type().arg_count() == 1 )
            return true;
    return false;
}

bool ASTUtils::is_binary_operator(const ASTNode* node)
{
    if (node->type() == ASTNodeType_OPERATOR )
        if (static_cast<const ASTFunctionCall *>(node)->get_func_type().arg_count() == 2 )
            return true;
    return false;
}

bool ASTUtils::is_conditional(const ASTNode* node)
{
    switch ( node->type() )
    {
        case ASTNodeType_FOR_LOOP:
        case ASTNodeType_WHILE_LOOP:
        case ASTNodeType_IF_ELSE:
            return true;
        default:
            return false;
    };
}

bool ASTUtils::is_output_node_in_expression(const ASTNode* input_node, const ASTNode* output_node)
{
#ifdef NDBL_DEBUG
    ASSERT(input_node);
    ASSERT(output_node);
    const bool is_an_output = std::find(input_node->outputs().begin(), input_node->outputs().end(), output_node) != input_node->outputs().end();
    ASSERT(is_an_output);
#endif

    if ( ASTUtils::is_instruction(input_node ) )
    {
        if (input_node->type() == ASTNodeType_VARIABLE )
        {
            const ASTNodeSlot* declaration_out = static_cast<const ASTVariable*>(input_node)->decl_out();
            return declaration_out->first_adjacent_node() == output_node;
        }
        return false;
    }
    return input_node->outputs().front() == output_node;
}
