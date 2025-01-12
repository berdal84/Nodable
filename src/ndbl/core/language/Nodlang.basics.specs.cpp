#include <gtest/gtest.h>

#include "ndbl/core/fixtures/core.h"
#include "tools/core/reflection/reflection"

using namespace ndbl;
using namespace tools;

typedef testing::Core Language_basics;

TEST_F(Language_basics, can_get_add_operator_with_short_identifier )
{
    EXPECT_TRUE(get_language()->find_operator("+", Operator_t::Binary));
    EXPECT_TRUE(get_language()->find_operator("-", Operator_t::Unary));
}
TEST_F(Language_basics, token_t_to_type)
{
    EXPECT_EQ(get_language()->get_type(ASTToken_t::keyword_bool)  , type::get<bool>());
    EXPECT_EQ(get_language()->get_type(ASTToken_t::keyword_double), type::get<double>() );
    EXPECT_EQ(get_language()->get_type(ASTToken_t::keyword_i16)   , type::get<i16_t>() );
    EXPECT_EQ(get_language()->get_type(ASTToken_t::keyword_int)   , type::get<i32_t>() );
    EXPECT_EQ(get_language()->get_type(ASTToken_t::keyword_string), type::get<std::string>() );
    EXPECT_EQ(get_language()->get_type(ASTToken_t::keyword_any)   , type::get<any>() );

    EXPECT_EQ(get_language()->get_type(ASTToken_t::literal_bool), nullptr);
    EXPECT_EQ(get_language()->get_type(ASTToken_t::literal_double), nullptr);
    EXPECT_EQ(get_language()->get_type(ASTToken_t::literal_int), nullptr);
    EXPECT_EQ(get_language()->get_type(ASTToken_t::literal_string), nullptr);
    EXPECT_EQ(get_language()->get_type(ASTToken_t::literal_any), nullptr);
}

TEST_F(Language_basics, type_to_string)
{
    EXPECT_EQ(get_language()->serialize_type(type::get<bool>())        , "bool" );
    EXPECT_EQ(get_language()->serialize_type(type::get<double>())      , "double" );
    EXPECT_EQ(get_language()->serialize_type(type::get<i16_t>())       , "i16" );
    EXPECT_EQ(get_language()->serialize_type(type::get<int>())         , "int" );
    EXPECT_EQ(get_language()->serialize_type(type::get<i32_t>())       , "int" );
    EXPECT_EQ(get_language()->serialize_type(type::get<std::string>()) , "string" );
    EXPECT_EQ(get_language()->serialize_type(type::get<any>())         , "any" );
}
