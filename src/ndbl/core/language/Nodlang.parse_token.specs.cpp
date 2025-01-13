#include "../fixtures/core.h"
#include <gtest/gtest.h>
#include "tools/core/log.h"

using namespace ndbl;

typedef ::testing::Core Language_parse_token;

///////////////////////// Atomic expressions ///////////////////////////////////////////////////////////////////////////

TEST_F(Language_parse_token, atomic_expression_if)
{
    std::string buffer{"if"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::keyword_if);
    EXPECT_EQ(token.string(), "if");
}

TEST_F(Language_parse_token, atomic_expression_else)
{
    std::string buffer{"else"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::keyword_else);
    EXPECT_EQ(token.string(), "else");
}

TEST_F(Language_parse_token, atomic_expression_for)
{
    std::string buffer{"for"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::keyword_for);
    EXPECT_EQ(token.string(), "for");
}

TEST_F(Language_parse_token, atomic_expression_bool_true)
{
    std::string buffer{"true"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::literal_bool);
    EXPECT_EQ(token.string(), "true");
}

TEST_F(Language_parse_token, atomic_expression_bool_false)
{
    std::string buffer{"false"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::literal_bool);
    EXPECT_EQ(token.string(), "false");
}

TEST_F(Language_parse_token, atomic_expression_int_5)
{
    std::string buffer{"5"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::literal_int);
}

TEST_F(Language_parse_token, atomic_expression_double_5_0)
{
    std::string buffer{"5.0"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::literal_double);
    EXPECT_EQ(token.string(), "5.0");
}

TEST_F(Language_parse_token, atomic_expression_double_5_0001)
{
    std::string buffer{"5.0001"};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::literal_double);
    EXPECT_EQ(token.string(), "5.0001");
}

TEST_F(Language_parse_token, atomic_expression_string)
{
    std::string buffer{"\"Hello\""};
    ASTToken token = get_language()->parse_token(buffer);
    EXPECT_EQ(token.m_type, ASTToken_t::literal_string);
    EXPECT_EQ(token.string(), "\"Hello\"");
}