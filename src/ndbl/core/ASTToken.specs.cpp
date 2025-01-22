#include <gtest/gtest.h>
#include "ndbl/core/ASTNodeProperty.h"

using namespace ndbl;

TEST(Token, empty_constructor)
{
    ASTToken token;
    EXPECT_EQ(token.prefix_to_string(), "");
    EXPECT_EQ(token.word_to_string(), "");
    EXPECT_EQ(token.suffix_to_string(), "");
    EXPECT_EQ(token.string(), "");
 }

TEST(Token, constructor__with_const_char_ptr)
{
    ASTToken token(ASTToken_t::identifier, "toto");
    EXPECT_EQ(token.prefix_to_string(), "");
    EXPECT_EQ(token.word_to_string(), "toto");
    EXPECT_EQ(token.suffix_to_string(), "");
    EXPECT_EQ(token.string(), "toto");
    EXPECT_EQ(token.m_buffer.intern(), false);
}

TEST(Token, suffix_append_from_stack)
{
    //                          >|--|<
    const char* toto = "// test\ntoto";
    ASTToken token(ASTToken_t::identifier, const_cast<char*>(toto), 8, 4);
    EXPECT_EQ(token.prefix_to_string(), "");
    EXPECT_EQ(token.m_buffer.intern(), false);

    token.suffix_push_back(";\n");

    EXPECT_EQ(token.suffix_to_string(), ";\n");
    EXPECT_EQ(token.string(), "toto;\n");
    EXPECT_EQ(token.offset(), 0);
    EXPECT_EQ(token.m_buffer.intern(), true);
}

TEST(Token, constructor__with_not_owned_buffer)
{
    const char* buffer = "<prefix>toto<suffix>";
    ASTToken token(ASTToken_t::identifier, const_cast<char*>(buffer));
    token.word_move_begin(8);
    token.word_move_end(-8);

    EXPECT_EQ(token.prefix_to_string(), "<prefix>");
    EXPECT_EQ(token.word_to_string(), "toto");
    EXPECT_EQ(token.suffix_to_string(), "<suffix>");
    EXPECT_EQ(token.m_buffer.intern(), false);
}

TEST(Token, take_prefix_suffix_from)
{
    // prepare

    std::string tata{"<prefix>TATA<suffix>"};
    ASTToken source(ASTToken_t::identifier, const_cast<char*>(tata.data()));
    source.word_move_begin(8);
    source.word_move_end(-8);

    std::string toto{"TOTO"};
    ASTToken target(ASTToken_t::identifier, const_cast<char*>(toto.data()), 0, toto.length());

    // pre-check
    EXPECT_EQ(source.string(), "<prefix>TATA<suffix>");
    EXPECT_FALSE(source.m_buffer.intern());

    EXPECT_EQ(target.string(), "TOTO");
    EXPECT_FALSE(target.m_buffer.intern());

    // act
    target.take_prefix_suffix_from(&source);

    // post-check
    EXPECT_EQ(source.string(), "TATA");
    EXPECT_FALSE(source.m_buffer.intern());

    EXPECT_EQ(target.string(), "<prefix>TOTO<suffix>");
    EXPECT_TRUE(target.m_buffer.intern());
}

TEST(Token, replace_word__same_length)
{
    // prepare
    std::string tata{"<prefix>TATA<suffix>"};
    ASTToken source(ASTToken_t::identifier, const_cast<char*>(tata.data()), 0, tata.length());
    source.word_move_begin(8);
    source.word_move_end(-8);

    // pre-check
    EXPECT_EQ(source.string(), "<prefix>TATA<suffix>");
    EXPECT_FALSE(source.m_buffer.intern());

    // act
    source.word_replace("TOTO");

    // post-check
    EXPECT_EQ(source.string(), "<prefix>TOTO<suffix>");
    EXPECT_TRUE(source.m_buffer.intern());
}

TEST(Token, replace_word__larger)
{
    // prepare
    const char* tata = "<prefix>42<suffix>";
    ASTToken source(ASTToken_t::identifier, const_cast<char*>(tata));
    source.word_move_begin(8);
    source.word_move_end(-8);

    // pre-check
    EXPECT_EQ(source.string(), "<prefix>42<suffix>");
    EXPECT_EQ(source.word_to_string(), "42");
    EXPECT_FALSE(source.m_buffer.intern());

    // act
    source.word_replace("2048");

    // post-check
    EXPECT_EQ(source.string(), "<prefix>2048<suffix>");
    EXPECT_TRUE(source.m_buffer.intern());
}


TEST(Token, replace_word__smaller)
{
    // prepare
    const char* tata = "<prefix>42<suffix>";
    ASTToken source(ASTToken_t::identifier, const_cast<char*>(tata));
    source.word_move_begin(8);
    source.word_move_end(-8);

    // pre-check
    EXPECT_EQ(source.string(), "<prefix>42<suffix>");
    EXPECT_EQ(source.word_to_string(), "42");
    EXPECT_FALSE(source.m_buffer.intern());

    // act
    source.word_replace("0");

    // post-check
    EXPECT_EQ(source.string(), "<prefix>0<suffix>");
    EXPECT_TRUE(source.m_buffer.intern());
}