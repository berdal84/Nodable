#include "ASTUtils.h"
#include "ASTFunctionCall.h"
#include "ASTVariable.h"

using namespace ndbl;

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
