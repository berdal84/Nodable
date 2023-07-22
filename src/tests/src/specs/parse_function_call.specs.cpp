#include "../fixtures/core.h"
#include <gtest/gtest.h>
#include "fw/core/log.h"
#include "nodable/core/InvokableComponent.h"

using namespace ndbl;

typedef ::testing::Core parse_function_call;

TEST_F(parse_function_call, dna_to_protein)
{
    // prepare parser
    std::string buffer{"dna_to_protein(\"GATACA\")"};
    nodlang.parser_state.clear();
    nodlang.parser_state.set_source_buffer(buffer.data(), buffer.length());
    nodlang.parser_state.graph = &graph;

    // tokenize
    nodlang.tokenize(buffer);

    // check
    EXPECT_EQ(nodlang.parser_state.ribbon.size(), 4);
    EXPECT_EQ(nodlang.parser_state.ribbon.at(0).m_type, Token_t::identifier);
    EXPECT_EQ(nodlang.parser_state.ribbon.at(1).m_type, Token_t::parenthesis_open);
    EXPECT_EQ(nodlang.parser_state.ribbon.at(2).m_type, Token_t::literal_string);
    EXPECT_EQ(nodlang.parser_state.ribbon.at(3).m_type, Token_t::parenthesis_close);

    // parse
    Property* result = nodlang.parse_function_call();

    // check
    EXPECT_TRUE(result != nullptr);
    Node* invokable_node = result->get_owner();
    EXPECT_TRUE(invokable_node->components.has<InvokableComponent>());
    auto type = invokable_node->components.get<InvokableComponent>()->get_type();
    EXPECT_TRUE(invokable_node->components.get<InvokableComponent>()->has_function()); // should not be abstract
}