#pragma once

namespace ndbl
{
    typedef int ASTNodeType;
    enum ASTNodeType_
    {
        // enum is used to index arrays, must start at 0 with no gaps

        ASTNodeType_DEFAULT = 0,
        ASTNodeType_SCOPE,
        ASTNodeType_IF_ELSE,
        ASTNodeType_FOR_LOOP,
        ASTNodeType_WHILE_LOOP,
        ASTNodeType_VARIABLE,
        ASTNodeType_VARIABLE_REF,
        ASTNodeType_LITERAL,
        ASTNodeType_FUNCTION,
        ASTNodeType_OPERATOR,
        ASTNodeType_EMPTY_INSTRUCTION,

        ASTNodeType_COUNT,
    };
}