#include <gtest/gtest.h>
#include "SpatialNode.h"

using namespace tools;

TEST(SpatialNode2D, add_child)
{
    // prepare
    SpatialNode child, root;

    // act
    root.add_child( &child );

    // verify
    EXPECT_TRUE(child.parent() == &root );
    EXPECT_TRUE(*root.children().begin() == &child );
}

TEST(SpatialNode2D, remove_child)
{
    // requires add_child to succeed

    // prepare
    SpatialNode child, root;
    root.add_child( &child );

    // act
    root.remove_child( &child );

    // verify
    EXPECT_TRUE(child.parent() == nullptr );
    EXPECT_TRUE(root.children().size() == 0 );
}


TEST(SpatialNode2D, add_child__with_offset)
{
    // prepare
    SpatialNode child, root;

    // act
    child.translate({ 10.f, 15.f});
    root.add_child( &child );

    // verify
    EXPECT_TRUE(child.parent() == &root );
    EXPECT_TRUE(*root.children().begin() == &child );
    EXPECT_FLOAT_EQ(child.position().x, 10.f );
    EXPECT_FLOAT_EQ(child.position().y, 15.f );
}

TEST(SpatialNode2D, set_pos__LOCAL_SPACE)
{
    // !!! This requires add_child to pass !!!

    // prepare
    SpatialNode child, root;
    root.add_child( &child );

    // act
    child.translate({-10.f, -10.f});

    // verify
    EXPECT_FLOAT_EQ(child.position().x, -10.f );
    EXPECT_FLOAT_EQ(child.position().y, -10.f );
}

TEST(SpatialNode2D, set_pos__PARENT_SPACE)
{
    // !!! This requires add_child__with_offset and set_pos__LOCAL_SPACE to pass !!!

    // prepare
    SpatialNode child, root;
    child.translate({10.f, 10.f});
    root.add_child(&child);

    EXPECT_FLOAT_EQ(child.position().x, 10.f );
    EXPECT_FLOAT_EQ(child.position().y, 10.f );

    // act
    child.set_position({0.f, 0.f}, PARENT_SPACE);

    // verify
    EXPECT_FLOAT_EQ(child.position().x, root.position().x );
    EXPECT_FLOAT_EQ(child.position().y, root.position().y );
}

TEST(SpatialNode2D, get_pos__GLOBAL_SPACE)
{
    // !!! This requires add_child and set_pos__LOCAL_SPACE to pass !!!

    // prepare
    SpatialNode level1, level0, root;

    root.add_child(&level0);
    level0.add_child(&level1);

    root.translate({10.f, 10.f});
    level0.translate({10.f, 10.f});
    level1.translate({10.f, 10.f});

    // pre check
    EXPECT_FLOAT_EQ(level1.position().x, 10.f );
    EXPECT_FLOAT_EQ(level1.position().y, 10.f );
    EXPECT_FLOAT_EQ(level1.position(WORLD_SPACE).x, 30.f );
    EXPECT_FLOAT_EQ(level1.position(WORLD_SPACE).y, 30.f );
}

TEST(SpatialNode2D, set_pos__GLOBAL_SPACE)
{
    // !!! This requires add_child and get_pos__SCREEN_SPACE to pass !!!

    // prepare
    SpatialNode child, root;
    root.add_child(&child);
    root.translate({10.f, 10.f});
    child.translate({10.f, 10.f});

    // pre check
    EXPECT_FLOAT_EQ(child.position(PARENT_SPACE).x, 10.f );
    EXPECT_FLOAT_EQ(child.position(PARENT_SPACE).y, 10.f );

    // act
    child.set_position({0.f, 0.f}, WORLD_SPACE);

    // check
    EXPECT_FLOAT_EQ(child.position(PARENT_SPACE).x, -10.f );
    EXPECT_FLOAT_EQ(child.position(PARENT_SPACE).y, -10.f );
    EXPECT_FLOAT_EQ(child.position(WORLD_SPACE).x, 0.f );
    EXPECT_FLOAT_EQ(child.position(WORLD_SPACE).y, 0.f );
}