#include <gtest/gtest.h>

#include "tools/core/reflection/Type.h"

#include "Graph.h"
#include "ASTFunctionCall.h"
#include "ASTNode.h"
#include "ASTLiteral.h"
#include "ASTVariable.h"
#include "ASTScope.h"
#include "ASTSlotLink.h"

#include "fixtures/core.h"
#include "ASTUtils.h"

using namespace ndbl;
using namespace tools;
typedef ::testing::Core Graph_;

TEST_F(Graph_, constructor)
{
    EXPECT_TRUE(app.graph()->is_empty());
    EXPECT_NE(app.graph()->root_node(), nullptr );
}

TEST_F(Graph_, create_node)
{
    ASTNode* node = app.graph()->create_node();
    EXPECT_EQ(node->scope(), app.graph()->root_scope());
}

TEST_F(Graph_, connect)
{
    // Prepare
    Graph* graph = app.graph();
    auto* node_1 = graph->create_node();
    auto* prop_1 = node_1->add_prop<bool>("prop_1");
    auto* slot_1 = node_1->add_slot(prop_1, SlotFlag_OUTPUT, 1);

    auto* node_2 = graph->create_node();
    auto* prop_2 = node_2->add_prop<bool>("prop_2");
    auto* slot_2 = node_2->add_slot(prop_2, SlotFlag_INPUT, 1);

    // Act
    ASTSlotLink edge = graph->connect_or_merge(slot_1, slot_2 );

    // Verify
    EXPECT_EQ(edge.tail->property, prop_1 );
    EXPECT_EQ(edge.head->property, prop_2 );
    EXPECT_EQ(graph->edges().size(), 1);
 }

TEST_F(Graph_, disconnect)
{
    // Prepare
    Graph* graph = app.graph();
    auto node_1 = graph->create_node();
    auto prop_1 = node_1->add_prop<bool>("prop_1");
    auto slot_1 = node_1->add_slot(prop_1, SlotFlag_OUTPUT, 1);

    auto node_2 = graph->create_node();
    auto prop_2 = node_2->add_prop<bool>("prop_2");
    auto slot_2 = node_2->add_slot(prop_2, SlotFlag_INPUT, 1);

    EXPECT_EQ(graph->edges().size(), 0);
    ASTSlotLink edge = graph->connect_or_merge(slot_1, slot_2 );
    EXPECT_EQ(graph->edges().size(), 1);

    // Act
    graph->disconnect(edge, GraphFlag_ALLOW_SIDE_EFFECTS );

    // Check
    EXPECT_EQ(graph->edges().size() , 0);
    EXPECT_EQ( node_1->adjacent_slot_count( SlotFlag_OUTPUT ), 0);
    EXPECT_EQ( node_2->adjacent_slot_count( SlotFlag_INPUT ) , 0);
}

TEST_F(Graph_, clear)
{
    Graph* graph = app.graph();
    EXPECT_TRUE( graph->is_empty() );
    EXPECT_TRUE(graph->edges().empty() );

    FunctionDescriptor  f;
    f.init<int(int, int)>("+");

    ASTVariable* variable  = graph->create_variable(type::get<int>(), "var");
    auto operator_node = graph->create_operator(f);

    EXPECT_TRUE(graph->edges().empty() );

    graph->connect(
            operator_node->value_out(),
            variable->value_in(),
            GraphFlag_ALLOW_SIDE_EFFECTS);

    EXPECT_FALSE( graph->is_empty() );
    EXPECT_FALSE(graph->edges().empty() );

    // act
    graph->reset();

    // test
    EXPECT_TRUE( graph->is_empty() );
    EXPECT_TRUE( graph->nodes().size() == 1 && *graph->nodes().cbegin() == graph->root_node() );
    EXPECT_TRUE(graph->edges().empty() );
}


TEST_F(Graph_, create_and_delete_relations)
{
    // prepare
    Graph* graph = app.graph();
    auto& edges = graph->edges();
    EXPECT_EQ(edges.size(), 0);
    auto node_1 = graph->create_literal<int>();
    EXPECT_EQ(edges.size(), 0);
    auto node_2 = graph->create_variable( type::get<int>(), "a" );

    // Act and test

    // INPUT (and by reciprocity OUTPUT)
    EXPECT_EQ(edges.size(), 0);
    EXPECT_EQ(ASTUtils::get_adjacent_nodes(node_2, SlotFlag_TYPE_VALUE ).size(), 0);
    ASTSlotLink edge_1 = graph->connect(node_1->value_out(), node_2->value_in());
    EXPECT_EQ(ASTUtils::get_adjacent_nodes(node_2, SlotFlag_TYPE_VALUE ).size(), 1);
    EXPECT_EQ(edges.size(), 1);
    graph->disconnect(edge_1);
    EXPECT_EQ(ASTUtils::get_adjacent_nodes(node_2, SlotFlag_TYPE_VALUE ).size(), 0);
}

TEST_F(Graph_, erase_node_from_non_root_scope)
{
    // prepare
    Graph* graph = app.graph();
    ASTNode* scope_node = graph->create_scope(graph->root_scope() );
    graph->connect(graph->root_node()->flow_enter(), scope_node->flow_in());
    ASTNode* child = graph->create_node(scope_node->internal_scope() );

    EXPECT_EQ(child->scope(), scope_node->internal_scope() );

    graph->find_and_destroy( child );

    EXPECT_FALSE( graph->contains( child ) );
    EXPECT_TRUE(scope_node->internal_scope()->empty() );
}


TEST_F(Graph_, erase_first_node_of_a_scope_with_another_child_after)
{
    // prepare
    Graph* graph = app.graph();

    ASTNode* scope_node = graph->create_scope( graph->root_scope() );
    ASTNode* child1 = graph->create_node();
    ASTNode* child2 = graph->create_node();

    graph->connect( scope_node->flow_enter(), child1->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );
    graph->connect( child1->flow_out(), child2->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );

    EXPECT_EQ(child1->scope(), scope_node->internal_scope() );
    EXPECT_EQ(child1->scope(), child2->scope());

    graph->find_and_destroy( child1 );

    EXPECT_FALSE(graph->contains( child1 ));
    EXPECT_TRUE(graph->contains( child2 ));
    EXPECT_FALSE(scope_node->internal_scope()->contains(child1) );
    EXPECT_TRUE(scope_node->internal_scope()->contains(child2 ) );
}