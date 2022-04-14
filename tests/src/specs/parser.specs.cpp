#include <gtest/gtest.h>
#include <nodable/core/Log.h>
#include "../test_tools.h"

using namespace Nodable;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, atomic_expression_int)
{
    EXPECT_EQ(eval<int>("5"), 5);
}

TEST_F(nodable_fixture, atomic_expression_double)
{
    EXPECT_EQ(eval<double>("10.0"), 10.0);
}

TEST_F(nodable_fixture, atomic_expression_string)
{
    EXPECT_EQ(eval<std::string>("\"hello world!\""), "hello world!");
}

TEST_F(nodable_fixture, atomic_expression_bool)
{
    EXPECT_EQ(eval<bool>("true"), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, unary_operators_minus_int)
{
    EXPECT_EQ(eval<int>("-5"), -5);
}

TEST_F(nodable_fixture, unary_operators_minus_double)
{
    EXPECT_EQ(eval<double>("-5.5"), -5.5);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, Simple_binary_expressions)
{
    EXPECT_EQ(eval<int>("2+3"), 5);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, Precedence_one_level)
{
    EXPECT_EQ(eval<int>("-5+4"), -1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, Precedence_two_levels)
{
    EXPECT_EQ(eval<double>("-1.0+2.0*5.0-3.0/6.0"), 8.5);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, parenthesis_01)
{
    EXPECT_EQ(eval<int>("(1+4)"), 5);
}

TEST_F(nodable_fixture, parenthesis_02)
{
    EXPECT_EQ(eval<int>("(1)+(2)"), 3);
}

TEST_F(nodable_fixture, parenthesis_03)
{
    EXPECT_EQ(eval<int>("(1+2)*3"), 9);
}

TEST_F(nodable_fixture, parenthesis_04)
{
    EXPECT_EQ(eval<int>("2*(5+3)"), 16);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, unary_binary_operator_mixed_01)
{
    EXPECT_EQ(eval<int>("-1*20"), -20);
}

TEST_F(nodable_fixture, unary_binary_operator_mixed_02)
{
    EXPECT_EQ(eval<int>("-(1+4)"), -5);
}

TEST_F(nodable_fixture, unary_binary_operator_mixed_03)
{
    EXPECT_EQ(eval<int>("(-1)+(-2)"), -3);
}
TEST_F(nodable_fixture, unary_binary_operator_mixed_04)
{
    EXPECT_EQ(eval<int>("-5*3"), -15);
}

TEST_F(nodable_fixture, unary_binary_operator_mixed_05)
{
    EXPECT_EQ(eval<int>("2-(5+3)"), -6);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, complex_parenthesis_01)
{
    EXPECT_EQ(eval<int>("2+(5*3)"), 2 + (5 * 3));
}

TEST_F(nodable_fixture, complex_parenthesis_02)
{
    EXPECT_EQ(eval<int>("2*(5+3)+2"), 2 * (5 + 3) + 2);
}

TEST_F(nodable_fixture, complex_parenthesis_03)
{
    EXPECT_EQ(eval<int>("(2-(5+3))-2+(1+1)"), (2 - (5 + 3)) - 2 + (1 + 1));
}

TEST_F(nodable_fixture, complex_parenthesis_04)
{
    EXPECT_EQ(eval<double>("(2.0 -(5.0+3.0 )-2.0)+9.0/(1.0- 0.54)"), (2.0 - (5.0 + 3.0) - 2.0) + 9.0 / (1.0 - 0.54));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, unexisting_function)
{
    EXPECT_ANY_THROW(eval<double>("pow_unexisting(5)") );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, function_call_return)
{
    EXPECT_EQ(eval<int>("return(5)"), 5);
}

TEST_F(nodable_fixture, function_call_sqrt_int)
{
    EXPECT_EQ(eval<int>("sqrt(81)"), 9);
}

TEST_F(nodable_fixture, function_call_pow_int_int)
{
    EXPECT_EQ(eval<int>("pow(2,2)"), 4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, functionlike_operator_call_multiply)
{
    EXPECT_EQ(eval<int>("operator*(2,2)"), 4);
}

TEST_F(nodable_fixture, functionlike_operator_call_superior)
{
    EXPECT_EQ(eval<bool>("operator>(2,2)"), false);
}

TEST_F(nodable_fixture, functionlike_operator_call_minus)
{
    EXPECT_EQ(eval<int>("operator-(3,2)"), 1);
}

TEST_F(nodable_fixture, functionlike_operator_call_plus)
{
    EXPECT_EQ(eval<int>("operator+(2,2)"), 4);
}

TEST_F(nodable_fixture, functionlike_operator_call_divide)
{
    EXPECT_EQ(eval<int>("operator/(4,2)"), 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, imbricated_operator_in_function)
{
    EXPECT_EQ(eval<int>("return(5+3)"), 8);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, imbricated_functions_01)
{
    EXPECT_EQ(eval<int>("return(return(1))"), 1);
}

TEST_F(nodable_fixture, imbricated_functions_02)
{
    EXPECT_EQ(eval<int>("return(return(1) + return(1))"), 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, Successive_assigns)
{
    EXPECT_EQ(eval<double>("double a; double b; a = b = 5.0;"), 5.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, string_var_assigned_with_literal_string)
{
    EXPECT_EQ(eval<std::string>("string a = \"coucou\"")       , "coucou");
}

TEST_F(nodable_fixture, string_var_assigned_with_15_to_string)
{
    EXPECT_EQ(eval<std::string>("string a = to_string(15)")    , "15");
}

TEST_F(nodable_fixture, string_var_assigned_with_minus15dot5_to_string)
{
    EXPECT_EQ(eval<std::string>("string a = to_string(15.5)")    , "15.5");
}

TEST_F(nodable_fixture, string_var_assigned_with_true_to_string)
{
    EXPECT_EQ(eval<std::string>("string a = to_string(true)")    , "true");
}

TEST_F(nodable_fixture, string_var_assigned_with_false_to_string)
{
    EXPECT_EQ(eval<std::string>("string a = to_string(false)")    , "false");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, precedence_1)
{
    EXPECT_TRUE(eval_serialize_and_compare("(1+1)*2"));
}

TEST_F(nodable_fixture, precedence_2)
{
    EXPECT_TRUE(eval_serialize_and_compare("1*1+2"));
}

TEST_F(nodable_fixture, precedence_3)
{
    EXPECT_TRUE(eval_serialize_and_compare("-(-1)"));
}

TEST_F(nodable_fixture, precedence_4)
{
    EXPECT_TRUE(eval_serialize_and_compare("-(2*5)"));
}

TEST_F(nodable_fixture, precedence_5)
{
    EXPECT_TRUE(eval_serialize_and_compare("-(2*5)"));
}

TEST_F(nodable_fixture, precedence_6)
{
    EXPECT_TRUE(eval_serialize_and_compare("(-2)*5"));
}

TEST_F(nodable_fixture, precedence_7)
{
    EXPECT_TRUE(eval_serialize_and_compare("-(2+5)"));
}

TEST_F(nodable_fixture, precedence_8)
{
    EXPECT_TRUE(eval_serialize_and_compare("5+(-1)*3"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, eval_serialize_and_compare_1)
{
    EXPECT_TRUE(eval_serialize_and_compare("1"));
}

TEST_F(nodable_fixture, eval_serialize_and_compare_2)
{
    EXPECT_TRUE(eval_serialize_and_compare("1+1"));
}

TEST_F(nodable_fixture, eval_serialize_and_compare_3)
{
    EXPECT_TRUE(eval_serialize_and_compare("1-1"));
}

TEST_F(nodable_fixture, eval_serialize_and_compare_4)
{
    EXPECT_TRUE(eval_serialize_and_compare("-1"));
}

TEST_F(nodable_fixture, eval_serialize_and_compare_5)
{
    EXPECT_TRUE(eval_serialize_and_compare("double a=5"));
}

TEST_F(nodable_fixture, eval_serialize_and_compare_6)
{
    EXPECT_TRUE(eval_serialize_and_compare("double a=1;double b=2;double c=3;double d=4;(a+b)*(c+d)"));
}

TEST_F(nodable_fixture, eval_serialize_and_compare_7)
{
    EXPECT_TRUE(eval_serialize_and_compare("string b = to_string(false)"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, decl_var_and_assign_string)
{
    std::string program = R"(string s = "coucou";)";
    EXPECT_EQ(parse_and_serialize(program), program);
}

TEST_F(nodable_fixture, decl_var_and_assign_double)
{
    std::string program = "double d = 15.0;";
    EXPECT_EQ(parse_and_serialize(program), program);
}

TEST_F(nodable_fixture, decl_var_and_assign_int)
{
    std::string program = "int s = 10;";
    EXPECT_EQ(parse_and_serialize(program), program);
}

TEST_F(nodable_fixture, decl_var_and_assign_bool)
{
    std::string program = "bool b = true;";
    EXPECT_EQ(parse_and_serialize(program), program);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, multi_instruction_single_line )
{
    EXPECT_EQ(eval<int>("int a = 5;int b = 2 * 5;"), 10 );
}

TEST_F(nodable_fixture, multi_instruction_multi_line_01 )
{
    EXPECT_TRUE(eval_serialize_and_compare("double a = 5.0;\ndouble b = 2.0 * a;"));
}

TEST_F(nodable_fixture, multi_instruction_multi_line_02 )
{
    EXPECT_TRUE(eval_serialize_and_compare("double a = 5.0;double b = 2.0 * a;\ndouble c = 33.0 + 5.0;"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, DNAtoProtein )
{
    EXPECT_EQ(eval<std::string>("DNAtoProtein(\"TAA\")"), "_");
    EXPECT_EQ(eval<std::string>("DNAtoProtein(\"TAG\")"), "_");
    EXPECT_EQ(eval<std::string>("DNAtoProtein(\"TGA\")"), "_");
    EXPECT_EQ(eval<std::string>("DNAtoProtein(\"ATG\")"), "M");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, code_formatting_preserving_01 )
{
    EXPECT_TRUE(eval_serialize_and_compare("double a =5;\ndouble b=2*a;"));
}

TEST_F(nodable_fixture, code_formatting_preserving_02 )
{
    EXPECT_TRUE(eval_serialize_and_compare("double a =5;\ndouble b=2  *  a;"));
}

TEST_F(nodable_fixture, code_formatting_preserving_03 )
{
    EXPECT_TRUE(eval_serialize_and_compare(" 5 + 2;"));
}

TEST_F(nodable_fixture, code_formatting_preserving_04 )
{
    EXPECT_TRUE(eval_serialize_and_compare("5 + 2;  "));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, Conditional_Structures_IF )
{
    std::string program =
            "double bob   = 10;"
            "double alice = 10;"
            "if(bob>alice){"
            "   string message = \"Bob is better than Alice.\";"
            "}";

    EXPECT_EQ( parse_and_serialize(program), program);
}

TEST_F(nodable_fixture, Conditional_Structures_IF_ELSE )
{
    std::string program =
            "double bob   = 10;"
            "double alice = 11;"
            "string message;"
            "if(bob<alice){"
            "   message= \"Alice is the best.\";"
            "}else{"
            "   message= \"Alice is not the best.\";"
            "}";

    EXPECT_EQ( parse_and_serialize(program), program);
}

TEST_F(nodable_fixture, Conditional_Structures_IF_ELSE_IF )
{
    std::string program =
            "double bob   = 10;"
            "double alice = 10;"
            "string message;"
            "if(bob>alice){"
            "   message= \"Bob is greater than Alice.\";"
            "} else if(bob<alice){"
            "   message= \"Bob is lower than Alice.\";"
            "} else {"
            "   message= \"Bob and Alice are equals.\";"
            "}";

    EXPECT_EQ( parse_and_serialize(program), program);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, operator_not_equal_double_double_true)
{
    EXPECT_TRUE(eval<bool>("10.0 != 9.0;") );
}

TEST_F(nodable_fixture, operator_not_equal_double_double_false)
{
    EXPECT_FALSE(eval<bool>("10.0 != 10.0;") );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, operator_not_bool)
{
    EXPECT_TRUE(eval<bool>("!false;") );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, parse_serialize_with_undeclared_variables )
{
    EXPECT_TRUE(parse_serialize_and_compare("double a = b + c * r - z;"));
}

TEST_F(nodable_fixture, parse_serialize_with_undeclared_variables_in_conditional )
{
    EXPECT_TRUE(parse_serialize_and_compare("if(a==b){}"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, parse_serialize_with_pre_ribbon_chars )
{
    EXPECT_TRUE(eval_serialize_and_compare(" double a = 5"));
}

TEST_F(nodable_fixture, parse_serialize_with_post_ribbon_chars )
{
    EXPECT_TRUE(eval_serialize_and_compare("double a = 5 "));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(nodable_fixture, parse_serialize_empty_scope )
{
    EXPECT_EQ(parse_and_serialize("{}"), "{}");
}

TEST_F(nodable_fixture, parse_serialize_empty_scope_with_spaces )
{
    EXPECT_EQ(parse_and_serialize("{ }"), "{ }");
}

TEST_F(nodable_fixture, parse_serialize_empty_scope_with_spaces_after )
{
    EXPECT_EQ(parse_and_serialize("{} "), "{} ");
}

TEST_F(nodable_fixture, parse_serialize_empty_scope_with_spaces_before )
{
    EXPECT_EQ(parse_and_serialize(" {}"), " {}");
}

TEST_F(nodable_fixture, parse_serialize_empty_scope_with_spaces_before_and_after )
{
    EXPECT_EQ(parse_and_serialize(" {} "), " {} ");
}

TEST_F(nodable_fixture, parse_serialize_empty_program )
{
    EXPECT_EQ(parse_and_serialize(""), "");
}

TEST_F(nodable_fixture, parse_serialize_empty_program_with_space )
{
    EXPECT_EQ(parse_and_serialize(" "), " ");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////