#include "meeting_sdk/audio/ring_buffer.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::audio {
namespace {

TEST(RingBuffer, PopOnEmptyReturnsNullopt) {
    RingBuffer<int> buffer(4);
    EXPECT_FALSE(buffer.tryPop().has_value());
}

TEST(RingBuffer, PushThenPopRoundTrips) {
    RingBuffer<int> buffer(4);
    ASSERT_TRUE(buffer.tryPush(7));
    auto popped = buffer.tryPop();
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(*popped, 7);
}

TEST(RingBuffer, PreservesFifoOrder) {
    RingBuffer<int> buffer(4);
    buffer.tryPush(1);
    buffer.tryPush(2);
    buffer.tryPush(3);
    EXPECT_EQ(buffer.tryPop(), 1);
    EXPECT_EQ(buffer.tryPop(), 2);
    EXPECT_EQ(buffer.tryPop(), 3);
}

TEST(RingBuffer, PushFailsWhenFull) {
    RingBuffer<int> buffer(2);
    EXPECT_TRUE(buffer.tryPush(1));
    EXPECT_TRUE(buffer.tryPush(2));
    EXPECT_FALSE(buffer.tryPush(3));  // capacity is 2; producer must not block
}

TEST(RingBuffer, WrapsAroundAfterDraining) {
    RingBuffer<int> buffer(2);
    buffer.tryPush(1);
    buffer.tryPush(2);
    buffer.tryPop();
    ASSERT_TRUE(buffer.tryPush(3));  // frees a slot, should wrap the internal index
    EXPECT_EQ(buffer.tryPop(), 2);
    EXPECT_EQ(buffer.tryPop(), 3);
}

}  // namespace
}  // namespace meeting_sdk::audio
