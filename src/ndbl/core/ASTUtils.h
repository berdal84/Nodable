#pragma once
#include <vector>
#include "tools/core/reflection/Type.h"
#include "ASTNodeType.h"
#include "ASTNodeSlotFlag.h"

namespace ndbl
{
    // Forward declarations
    class ASTForLoop;
    class ASTFunctionCall;
    class ASTIf;
    class ASTLiteral;
    class ASTNode;
    class ASTVariable;
    class ASTVariableRef;
    class ASTWhileLoop;
    class Nodlang;

    namespace ASTUtils
    {
        // Factory

        ASTNode*              create_root_scope();
        ASTNode*              create_scope();
        ASTVariable*          create_variable(const tools::TypeDescriptor*, const std::string& name);
        ASTVariableRef*       create_variable_ref();
        ASTLiteral*           create_literal(const tools::TypeDescriptor*);
        ASTFunctionCall*      create_function(const tools::FunctionDescriptor&, ASTNodeType = ASTNodeType_FUNCTION);
        ASTIf*                create_cond_struct();
        ASTForLoop*           create_for_loop();
        ASTWhileLoop*         create_while_loop();
        ASTNode*              create_node();
        ASTNode*              create_empty_instruction();

        // Misc.

        std::vector<ASTNode*> get_adjacent_nodes(const ASTNode*, SlotFlags);
        ASTNode*              adjacent_node_at(const ASTNode*, SlotFlags, u8_t pos);
        bool                  is_instruction(const ASTNode*);
        bool                  can_be_instruction(const ASTNode*);
        bool                  is_unary_operator(const ASTNode*);
        bool                  is_binary_operator(const ASTNode*);
        bool                  is_conditional(const ASTNode*);
        bool                  is_connected_to_codeflow(const ASTNode *node);
        bool                  is_output_node_in_expression(const ASTNode* input_node, const ASTNode* output_node);
    }
}
