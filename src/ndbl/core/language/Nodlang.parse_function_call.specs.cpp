#include "../fixtures/core.h"
#include <gtest/gtest.h>
#include "tools/core/log.h"
#include "ndbl/core/ASTFunctionCall.h"

using namespace ndbl;

typedef ::testing::Core Language_parse_function_call;

TEST_F(Language_parse_function_call, dna_to_protein)
{
    auto* language = app.get_language();

    // tokenize
    language->tokenize("dna_to_protein(\"GATACA\")");

    // check
    ASTTokenRibbon& ribbon = language->_state.tokens();
    EXPECT_EQ(ribbon.size(), 4);
    EXPECT_EQ(ribbon.at(0).m_type, ASTToken_t::identifier);
    EXPECT_EQ(ribbon.at(1).m_type, ASTToken_t::parenthesis_open);
    EXPECT_EQ(ribbon.at(2).m_type, ASTToken_t::literal_string);
    EXPECT_EQ(ribbon.at(3).m_type, ASTToken_t::parenthesis_close);

    // parse
    ASTNodeSlot* function_out = language->parse_function_call( app.graph()->root_scope() );

    // check
    EXPECT_TRUE(function_out!= nullptr);
    EXPECT_TRUE(function_out->node->type() == ASTNodeType_FUNCTION);
}

TEST_F(Language_parse_function_call, operator_add)
{
    auto* language = app.get_language();

    // tokenize
    language->tokenize("42+42");

    // check
    ASTTokenRibbon& ribbon = language->_state.tokens();
    EXPECT_EQ(ribbon.size(), 3);
    EXPECT_EQ(ribbon.at(0).m_type, ASTToken_t::literal_int);
    EXPECT_EQ(ribbon.at(1).m_type, ASTToken_t::operator_);
    EXPECT_EQ(ribbon.at(2).m_type, ASTToken_t::literal_int);

    // parse
    ASTNodeSlot* result = language->parse_expression( app.graph()->root_scope() );

    // check
    EXPECT_TRUE(result != nullptr );
    EXPECT_TRUE(result->node->type() == ASTNodeType_OPERATOR);
}