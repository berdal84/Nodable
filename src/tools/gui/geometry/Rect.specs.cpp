#include <gtest/gtest.h>
#include "Rect.h"

using namespace tools;

TEST(Rect, make_row__no_gap)
{
    const float gap = 0.f;
    const Vec2  origin = {100.f, 100.f};
    std::vector<Rect> r{ {10.f, 5.f}, {20.f, 5.f}, {30.f, 5.f} };
    r[0].translate(origin); // move first item to be sure it is not on (0,0)
    Rect::make_row( r, gap );

    EXPECT_FLOAT_EQ( r[0].min.x, origin.x);
    EXPECT_FLOAT_EQ( r[0].min.y, origin.y);
    EXPECT_FLOAT_EQ( r[0].max.x, r[1].min.x );
    EXPECT_FLOAT_EQ( r[1].max.x, r[2].min.x );
    EXPECT_FLOAT_EQ( r[2].max.x - r[0].min.x, r[0].width() + r[1].width() + r[2].width() );
}

TEST(Rect, make_row__with_gap)
{
    const float gap = 10.f;
    const Vec2  origin = {100.f, 100.f};
    std::vector<Rect> r{ {10.f, 5.f}, {20.f, 5.f}, {30.f, 5.f} };
    r[0].translate(origin); // move first item to be sure it is not on (0,0)
    Rect::make_row( r, gap );

    EXPECT_FLOAT_EQ( r[0].min.x, origin.x);
    EXPECT_FLOAT_EQ( r[0].min.y, origin.y);
    EXPECT_FLOAT_EQ( r[0].max.x + gap, r[1].min.x );
    EXPECT_FLOAT_EQ( r[1].max.x + gap, r[2].min.x );
    EXPECT_FLOAT_EQ( r[2].max.x - r[0].min.x, r[0].width() + gap + r[1].width() + gap + r[2].width() );
}

TEST(Rect, align_top__positive_coord)
{
    const float coord = 200.f;
    std::vector<Rect> r{ {10.f, 5.f}, {20.f, 50.f}, {30.f, 500.f} };
    Rect::align_top( r, coord);

    EXPECT_FLOAT_EQ( r[0].min.y, coord);
    EXPECT_FLOAT_EQ( r[1].min.y, coord);
    EXPECT_FLOAT_EQ( r[2].min.y, coord);
}


TEST(Rect, align_top__negative_coord)
{
    const float coord = -200.f;
    std::vector<Rect> r{ {10.f, 5.f}, {20.f, 50.f}, {30.f, 500.f} };
    Rect::align_top( r, coord);

    EXPECT_FLOAT_EQ( r[0].min.y, coord);
    EXPECT_FLOAT_EQ( r[1].min.y, coord);
    EXPECT_FLOAT_EQ( r[2].min.y, coord);
}