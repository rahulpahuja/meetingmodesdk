#include "meeting_sdk/storage/sqlite_meeting_repository.hpp"

#include <gtest/gtest.h>

#include "meeting_sdk/storage/in_memory_key_provider.hpp"
#include "meeting_sdk/storage/sodium_encryptor.hpp"
#include "support/time_helpers.hpp"

namespace meeting_sdk::storage {
namespace {

using test_support::ts;

core::Meeting makeMeeting(std::string id, std::string text) {
    core::Meeting m;
    m.id = core::MeetingId{std::move(id)};
    m.state = core::MeetingState::Completed;
    m.range = core::TimeRange{ts(0), ts(1000)};

    core::TranscriptSegment seg;
    seg.id = core::SegmentId{"seg1"};
    seg.range = m.range;
    seg.text = std::move(text);
    m.transcript = {seg};
    return m;
}

class SqliteMeetingRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto opened = SqliteMeetingRepository::open(":memory:", encryptor_, keyProvider_);
        ASSERT_TRUE(opened);
        repo_ = std::move(opened.value());
    }

    SodiumEncryptor encryptor_;
    InMemoryKeyProvider keyProvider_{encryptor_.keyLength()};
    std::unique_ptr<SqliteMeetingRepository> repo_;
};

TEST_F(SqliteMeetingRepositoryTest, SaveThenGetRoundTripsTheMeeting) {
    auto meeting = makeMeeting("m1", "hello world");
    ASSERT_TRUE(repo_->save(meeting));

    auto fetched = repo_->get(core::MeetingId{"m1"});
    ASSERT_TRUE(fetched);
    EXPECT_EQ(fetched.value().transcript[0].text, "hello world");
}

TEST_F(SqliteMeetingRepositoryTest, SaveTwiceUpdatesInPlace) {
    ASSERT_TRUE(repo_->save(makeMeeting("m1", "first version")));
    ASSERT_TRUE(repo_->save(makeMeeting("m1", "second version")));

    auto fetched = repo_->get(core::MeetingId{"m1"});
    ASSERT_TRUE(fetched);
    EXPECT_EQ(fetched.value().transcript[0].text, "second version");

    auto ids = repo_->listAll();
    ASSERT_TRUE(ids);
    EXPECT_EQ(ids.value().size(), 1U);  // update, not a second row
}

TEST_F(SqliteMeetingRepositoryTest, GetOnMissingIdReturnsNotFound) {
    auto result = repo_->get(core::MeetingId{"nonexistent"});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "storage.not_found");
}

TEST_F(SqliteMeetingRepositoryTest, RemoveDeletesTheMeetingAndItsKey) {
    ASSERT_TRUE(repo_->save(makeMeeting("m1", "to be deleted")));
    ASSERT_TRUE(repo_->remove(core::MeetingId{"m1"}));

    auto result = repo_->get(core::MeetingId{"m1"});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "storage.not_found");
}

TEST_F(SqliteMeetingRepositoryTest, ListAllReturnsEverySavedId) {
    ASSERT_TRUE(repo_->save(makeMeeting("m1", "a")));
    ASSERT_TRUE(repo_->save(makeMeeting("m2", "b")));

    auto ids = repo_->listAll();
    ASSERT_TRUE(ids);
    ASSERT_EQ(ids.value().size(), 2U);
}

}  // namespace
}  // namespace meeting_sdk::storage
