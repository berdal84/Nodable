#include <gtest/gtest.h>
#include "tools/core/string.h"

using namespace tools;

TEST(string, constructor_no_args)
{
    string str;
    EXPECT_STREQ(str.c_str(), "");
    EXPECT_EQ(str.length(), 0);
    EXPECT_EQ(str.is_empty(), true);
    EXPECT_EQ(str.capacity(), 0);
}

TEST(string16, constructor_no_args)
{
    string16 str;
    EXPECT_STREQ(str.c_str(), "");
    EXPECT_EQ(str.length(), 0);
    EXPECT_EQ(str.is_empty(), true);
    EXPECT_EQ(str.capacity(), 15);
}

TEST(string16, constructor_with_args)
{
    string16 str("Super");

    EXPECT_STREQ(str.c_str(), "Super");
    EXPECT_EQ(str.length(), 5);
    EXPECT_EQ(str.is_empty(), false);
    EXPECT_EQ(str.capacity(), 15);
}

TEST(string16, append_char)
{
    string16 str("Super");

    EXPECT_STREQ(str.c_str(), "Super");
    EXPECT_EQ(str.length(), 5);

    str.push_back('!');

    EXPECT_EQ(str.length(), 6);
}

TEST(string16, append_strn)
{
    string16 str("Super");

    EXPECT_STREQ(str.c_str(), "Super");
    EXPECT_STREQ(str.c_str(), "Super");
    EXPECT_EQ(str.length(), 5);

    str.append("man!", 4);

    EXPECT_EQ(str.length(), 9);
}

TEST(string16, append_const_char_ptr)
{
    string16 str("Super");

    EXPECT_STREQ(str.c_str(), "Super");
    EXPECT_EQ(str.length(), 5);

    str.append("man!");

    EXPECT_EQ(str.length(), 9);
}

TEST(string8, append_overflow)
{
    string8 str("Super");

    EXPECT_FALSE(str.heap_allocated());

    str.append(" long");

    EXPECT_STREQ(str.c_str(), "Super long");
    EXPECT_TRUE(str.heap_allocated());
}

TEST(string, append_overflow)
{
    string str("Super");

    EXPECT_TRUE(str.heap_allocated());
    EXPECT_EQ(str.capacity(), 8-1);

    str.append(" long");

    EXPECT_STREQ(str.c_str(), "Super long");
    EXPECT_EQ(str.capacity(), 16-1);

    str.append(", mais vraiment beaucoup ça race");

    EXPECT_STREQ(str.c_str(), "Super long, mais vraiment beaucoup ça race");
    EXPECT_EQ(str.capacity(), 64-1);
}

TEST(string8, copy )
{
    string8 str("Super");
    string8 copy = str;
    EXPECT_STREQ(str.c_str(), copy.c_str());
    str.append(" bien!");
    EXPECT_STRNE(str.c_str(), copy.c_str());
}

TEST(string8, copy_constructor )
{
    string8 str("Super");
    string8 copy(str);
    EXPECT_STREQ(str.c_str(), copy.c_str());
    str.append(" bien!");
    EXPECT_STRNE(str.c_str(), copy.c_str());
}

TEST(string, string_copy )
{
    string str("Super");
    string copy = str;
    EXPECT_STREQ(str.c_str(), copy.c_str());
    str.append(" bien!");
    EXPECT_STRNE(str.c_str(), copy.c_str());
}

TEST(string, copy_string8 )
{
    string8 str("Super");
    string copy = str;
    EXPECT_STREQ(str.c_str(), copy.c_str());
    str.append(" bien!");
    EXPECT_STRNE(str.c_str(), copy.c_str());
}

TEST(string8, copy_string )
{
    string str("Super");
    string8 copy = str;
    EXPECT_STREQ(str.c_str(), copy.c_str());
    EXPECT_FALSE(copy.heap_allocated());
}

TEST(string8, move_ctor )
{
    string str("My string is very long");
    string copy(std::move(str));
    EXPECT_STREQ(copy.c_str(), "My string is very long");
    EXPECT_STREQ(str.c_str(), "");
}

TEST(string8, move_assignment)
{
    string str("My string is very long");
    string copy = std::move(str);
    EXPECT_STREQ(copy.c_str(), "My string is very long");
    EXPECT_STREQ(str.c_str(), "");
}