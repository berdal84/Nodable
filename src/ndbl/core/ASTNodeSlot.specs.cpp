#include <gtest/gtest.h>
#include "ASTSlotLink.h"
#include "ASTNodeSlot.h"

using namespace ndbl;

TEST(Slot, is_full)
{
    ASTNodeSlot slot;
    ASTNodeSlot other1;
    ASTNodeSlot other2;

    EXPECT_TRUE(slot.is_full());

    slot.expand_capacity(2);
    slot.add_adjacent(&other1);
    EXPECT_FALSE(slot.is_full());

    slot.add_adjacent(&other2);
    EXPECT_TRUE(slot.is_full());
}

TEST(Slot, adjacent_at)
{
    // prepare
    ASTNodeSlot slot  {nullptr, SlotFlag_OUTPUT};
    slot.expand_capacity(2);

    ASTNodeSlot slot_0{nullptr, SlotFlag_INPUT};
    ASTNodeSlot slot_1{nullptr, SlotFlag_INPUT};

    slot.add_adjacent(&slot_0);
    slot.add_adjacent(&slot_1);

    // act
    ASTNodeSlot* adjacent_slot_0 = slot.adjacent_at(0 );
    ASTNodeSlot* adjacent_slot_1 = slot.adjacent_at(1 );

    // verify
    EXPECT_EQ(adjacent_slot_0, &slot_0);
    EXPECT_EQ(adjacent_slot_1, &slot_1);
}

TEST(Slot, allows_relation)
{
    // prepare
    ASTNodeSlot slot;

    EXPECT_TRUE(slot.flags() == SlotFlag_NONE );

    slot.set_flags( SlotFlag_INPUT );

    EXPECT_TRUE( SlotFlag_INPUT & SlotFlag_TYPE_VALUE );
    EXPECT_TRUE( slot.has_flags( SlotFlag_TYPE_VALUE ) );
}