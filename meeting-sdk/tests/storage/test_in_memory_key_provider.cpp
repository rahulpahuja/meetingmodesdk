#include "meeting_sdk/storage/in_memory_key_provider.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::storage {
namespace {

TEST(InMemoryKeyProvider, IsStableAcrossRepeatedCallsForTheSameId) {
    InMemoryKeyProvider provider(32);
    core::MeetingId id{"m1"};
    auto first = provider.getOrCreateKey(id);
    auto second = provider.getOrCreateKey(id);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first.value(), second.value());
}

TEST(InMemoryKeyProvider, DifferentMeetingsGetDifferentKeys) {
    InMemoryKeyProvider provider(32);
    auto a = provider.getOrCreateKey(core::MeetingId{"m1"});
    auto b = provider.getOrCreateKey(core::MeetingId{"m2"});
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    EXPECT_NE(a.value(), b.value());
}

TEST(InMemoryKeyProvider, DeleteKeyThenRecreateProducesADifferentKey) {
    InMemoryKeyProvider provider(32);
    core::MeetingId id{"m1"};
    auto original = provider.getOrCreateKey(id);
    ASSERT_TRUE(original);

    ASSERT_TRUE(provider.deleteKey(id));

    auto recreated = provider.getOrCreateKey(id);
    ASSERT_TRUE(recreated);
    EXPECT_NE(original.value(), recreated.value());  // proves the old key is truly gone
}

TEST(InMemoryKeyProvider, KeyHasTheRequestedLength) {
    InMemoryKeyProvider provider(24);
    auto key = provider.getOrCreateKey(core::MeetingId{"m1"});
    ASSERT_TRUE(key);
    EXPECT_EQ(key.value().size(), 24U);
}

}  // namespace
}  // namespace meeting_sdk::storage
