#include <gtest/gtest.h>

#include <nodable/core/InvokableFunction.h>

using namespace Nodable;
using namespace Nodable::R;

TEST( Function_Signature, no_arg_fct)
{
    auto no_arg_fct = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");

    EXPECT_EQ(no_arg_fct->get_arg_count(), 0);
    EXPECT_EQ(no_arg_fct->get_arg_count(), 0);
}

TEST( Function_Signature, push_single_arg)
{
    FuncSig* single_arg_fct = FuncSig
            ::new_instance<bool(double)>
            ::with_id(FuncSig::Type::Function, "fct");

    EXPECT_EQ(single_arg_fct->get_arg_count(), 1);
    EXPECT_EQ(single_arg_fct->get_return_type()->get_type(), Type::bool_t);
    EXPECT_EQ(single_arg_fct->get_args().at(0).m_type->get_type(), Type::double_t);
}

TEST( Function_Signature, push_two_args)
{
    auto two_arg_fct = FuncSig
            ::new_instance<bool(double, double)>
            ::with_id(FuncSig::Type::Function, "fct");

    EXPECT_EQ(two_arg_fct->get_arg_count(), 2);
    EXPECT_EQ(two_arg_fct->get_return_type()->get_type(), Type::bool_t);
    EXPECT_EQ(two_arg_fct->get_args().at(0).m_type->get_type(), Type::double_t);
    EXPECT_EQ(two_arg_fct->get_args().at(1).m_type->get_type(), Type::double_t);
}

TEST( Function_Signature, match_check_for_arg_count)
{
    FuncSig* single_arg_fct = FuncSig
            ::new_instance<bool(bool)>
            ::with_id(FuncSig::Type::Function, "fct");

    FuncSig* two_arg_fct = FuncSig
            ::new_instance<bool(bool, bool)>
            ::with_id(FuncSig::Type::Function, "fct");

    EXPECT_EQ(two_arg_fct->is_compatible(single_arg_fct), false);
    EXPECT_EQ(single_arg_fct->is_compatible(two_arg_fct), false);
}

TEST( Function_Signature, match_check_identifier)
{
    FuncSig* two_arg_fct = FuncSig
            ::new_instance<bool(bool, bool)>
            ::with_id(FuncSig::Type::Function, "fct");

    FuncSig* two_arg_fct_modified = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");

    two_arg_fct_modified->push_arg(R::get_meta_type<double>() );
    two_arg_fct_modified->push_arg(R::get_meta_type<double>() );

    EXPECT_EQ(two_arg_fct->is_compatible(two_arg_fct_modified), false);
    EXPECT_EQ(two_arg_fct_modified->is_compatible(two_arg_fct), false);
}

TEST( Function_Signature, match_check_absence_of_arg)
{
    FuncSig* two_arg_fct = FuncSig
            ::new_instance<bool(bool, bool)>
            ::with_id(FuncSig::Type::Function, "fct");

    FuncSig* two_arg_fct_without_args = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");

    EXPECT_EQ(two_arg_fct->is_compatible(two_arg_fct_without_args), false);
    EXPECT_EQ(two_arg_fct_without_args->is_compatible(two_arg_fct), false);
}

TEST( Function_Signature, push_args_template_0)
{
    auto ref = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");

    auto fct = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");

    using Args = std::tuple<>; // create arg tuple
    fct->push_args<Args>(); // push those args to signature

    EXPECT_EQ(ref->is_compatible(fct), true);
    EXPECT_EQ(fct->get_arg_count(), 0);
}

TEST( Function_Signature, push_args_template_1)
{
    auto ref = FuncSig
            ::new_instance<bool(double, double)>
            ::with_id(FuncSig::Type::Function, "fct");

    auto fct = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");

    fct->push_args< std::tuple<double, double> >();

    EXPECT_EQ(ref->is_compatible(fct), true);
    EXPECT_EQ(fct->get_arg_count(), 2);
}


TEST( Function_Signature, push_args_template_4)
{
    auto ref = FuncSig
            ::new_instance<bool(double, double, double, double)>
            ::with_id(FuncSig::Type::Function, "fct");

    auto fct = FuncSig
            ::new_instance<bool()>
            ::with_id(FuncSig::Type::Function, "fct");
    fct->push_args< std::tuple<double, double, double, double> >();

    EXPECT_EQ(ref->is_compatible(fct), true);
    EXPECT_EQ(fct->get_arg_count(), 4);
}
