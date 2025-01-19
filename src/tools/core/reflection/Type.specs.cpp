#include <gtest/gtest.h>
#include "tools/core/reflection/Type.h"
using namespace tools;

TEST(FunctionDescriptor, no_arg_fct)
{
    FunctionDescriptor f;
    f.init<bool()>("fct");
    EXPECT_EQ( f.arg_count(), 0);
}

TEST(FunctionDescriptor, push_single_arg)
{
    FunctionDescriptor f;
    f.init<bool(double)>("fct");

    EXPECT_EQ(f.arg_count(), 1);
    EXPECT_TRUE(f.return_type()->is<bool>());
    EXPECT_TRUE(f.arg().at(0).type->is<double>() );
}

TEST(FunctionDescriptor, push_two_args)
{
    FunctionDescriptor f;
    f.init<bool(double)>("fct");

    EXPECT_EQ(f.arg_count(), 1);
    EXPECT_TRUE(f.return_type()->is<bool>());
    EXPECT_TRUE(f.arg().at(0).type->is<double>() );
    EXPECT_TRUE(f.arg().at(0).type->is<double>() );
}

TEST(FunctionDescriptor, match_check_for_arg_count)
{
    FunctionDescriptor f, g;
    f.init<bool(bool)>("fct");
    g.init<bool(bool, bool)>("fct");

    EXPECT_EQ(g.is_compatible(&f), false);
    EXPECT_EQ(f.is_compatible(&g), false);
}

TEST(FunctionDescriptor, push_args_template_0)
{
    FunctionDescriptor f, g;
    f.init<bool()>("fct");
    g.init<bool()>("fct");

    using ZeroArgs = std::tuple<>;
    g.push_args<ZeroArgs>();

    EXPECT_EQ(f.is_compatible(&g), true);
    EXPECT_EQ(g.arg_count(), 0);
}

TEST(FunctionDescriptor, push_args_template_1)
{
    FunctionDescriptor f, g;
    f.init<bool(double, double)>("fct");
    g.init<bool()>("fct");

    g.push_args< std::tuple<double, double> >();

    EXPECT_EQ(f.is_compatible(&g), true);
    EXPECT_EQ(g.arg_count(), 2);
}

TEST(FunctionDescriptor, push_args_template_4)
{
    FunctionDescriptor f, g;
    f.init<bool(double, double, double, double)>("fct");
    g.init<bool()>("fct");

    g.push_args< std::tuple<double, double, double, double> >();

    EXPECT_EQ(f.is_compatible(&g), true);
    EXPECT_EQ(g.arg_count(), 4);
}

TEST(TypeDescriptor, is_integer)
{
    EXPECT_TRUE( type::get<i8_t>()->is_integer() );
    EXPECT_TRUE( type::get<i16_t>()->is_integer() );
    EXPECT_TRUE( type::get<i32_t>()->is_integer() );
    EXPECT_TRUE( type::get<i64_t>()->is_integer() );
    EXPECT_TRUE( type::get<u8_t>()->is_integer() );
    EXPECT_TRUE( type::get<u16_t>()->is_integer() );
    EXPECT_TRUE( type::get<u32_t>()->is_integer() );
    EXPECT_TRUE( type::get<u64_t>()->is_integer() );

    EXPECT_FALSE( type::get<double>()->is_integer() );
    EXPECT_FALSE( type::get<float>()->is_integer() );
    EXPECT_FALSE( type::get<std::string>()->is_integer() );
}

TEST(TypeDescriptor, is_floating_point)
{
    EXPECT_TRUE( type::get<double>()->is_floating_point() );
    EXPECT_TRUE( type::get<float>()->is_floating_point() );

    EXPECT_FALSE( type::get<i8_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<i16_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<i32_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<i64_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<u8_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<u16_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<u32_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<u64_t>()->is_floating_point() );
    EXPECT_FALSE( type::get<std::string>()->is_integer() );
}

TEST(TypeDescriptor, pass_by_ref_argument )
{
    FunctionDescriptor f;
    f.init<double(double &, double)>("=");
    EXPECT_TRUE(f.arg_at(0).pass_by_ref);
    EXPECT_FALSE(f.arg_at(1).pass_by_ref);
}
