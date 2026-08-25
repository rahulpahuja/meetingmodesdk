#include "meeting_sdk/storage/meeting_serializer.hpp"

#include <gtest/gtest.h>

#include "support/time_helpers.hpp"

namespace meeting_sdk::storage {
namespace {

using test_support::ts;

core::Meeting makeSampleMeeting() {
    core::Meeting m;
    m.id = core::MeetingId{"m1"};
    m.state = core::MeetingState::Completed;
    m.range = core::TimeRange{ts(0), ts(60000)};

    core::SpeakerEmbedding emb;
    emb.speaker = core::SpeakerId{"s1"};
    emb.vector = {0.1F, 0.2F, 0.3F};
    emb.capturedAt = ts(500);
    core::Speaker speaker;
    speaker.id = core::SpeakerId{"s1"};
    speaker.displayName = "Alice";
    speaker.embeddings = {emb};
    m.speakers = {speaker};

    core::TranscriptSegment seg;
    seg.id = core::SegmentId{"seg1"};
    seg.range = core::TimeRange{ts(0), ts(3000)};
    seg.speaker = core::SpeakerId{"s1"};
    seg.text = "Kal hum release kar sakte hain, but security approval abhi pending hai.";
    seg.detectedLanguage = core::Language{.bcp47Code = "hi", .confidence = 0.6F};
    seg.confidence = 0.9F;
    seg.languageSegments = {
        core::LanguageSegment{core::TimeRange{ts(0), ts(1500)},
                               core::Language{.bcp47Code = "hi", .confidence = 0.7F}},
        core::LanguageSegment{core::TimeRange{ts(1500), ts(3000)},
                               core::Language{.bcp47Code = "en", .confidence = 0.8F}},
    };
    m.transcript = {seg};

    m.summary = core::Summary{.text = "Shipped release, pending approval.", .keyPoints = {"key point 1"}};

    core::ActionItem withOwnerAndDeadline;
    withOwnerAndDeadline.action = "Follow up with security";
    withOwnerAndDeadline.owner = "Alice";
    withOwnerAndDeadline.deadline = ts(90000);
    withOwnerAndDeadline.sourceSegment = core::SegmentId{"seg1"};
    withOwnerAndDeadline.confidence = 0.5F;

    core::ActionItem withoutOwnerOrDeadline;
    withoutOwnerOrDeadline.action = "Send the doc";
    withoutOwnerOrDeadline.sourceSegment = core::SegmentId{"seg1"};
    withoutOwnerOrDeadline.confidence = 0.5F;

    m.actionItems = {withOwnerAndDeadline, withoutOwnerOrDeadline};

    m.decisions = {core::Decision{.text = "We will ship Friday.",
                                   .sourceSegment = core::SegmentId{"seg1"},
                                   .confidence = 0.5F}};

    m.topics = {core::Topic{.label = "security", .relatedSegments = {core::SegmentId{"seg1"}}}};

    m.questions = {core::Question{
        .text = "Is security approval done?", .sourceSegment = core::SegmentId{"seg1"}, .resolved = false}};

    return m;
}

TEST(MeetingSerializer, RoundTripsAllFields) {
    const core::Meeting original = makeSampleMeeting();
    const auto bytes = MeetingSerializer::serialize(original);
    auto result = MeetingSerializer::deserialize(bytes);
    ASSERT_TRUE(result);
    const core::Meeting& m = result.value();

    EXPECT_EQ(m.id.value, "m1");
    EXPECT_EQ(m.state, core::MeetingState::Completed);
    EXPECT_EQ(m.range.start.value, original.range.start.value);
    EXPECT_EQ(m.range.end.value, original.range.end.value);

    ASSERT_EQ(m.speakers.size(), 1U);
    EXPECT_EQ(m.speakers[0].id.value, "s1");
    ASSERT_TRUE(m.speakers[0].displayName.has_value());
    EXPECT_EQ(*m.speakers[0].displayName, "Alice");
    ASSERT_EQ(m.speakers[0].embeddings.size(), 1U);
    EXPECT_EQ(m.speakers[0].embeddings[0].vector, (std::vector<float>{0.1F, 0.2F, 0.3F}));

    ASSERT_EQ(m.transcript.size(), 1U);
    EXPECT_EQ(m.transcript[0].text, original.transcript[0].text);
    EXPECT_EQ(m.transcript[0].detectedLanguage.bcp47Code, "hi");
    ASSERT_EQ(m.transcript[0].languageSegments.size(), 2U);
    EXPECT_EQ(m.transcript[0].languageSegments[1].language.bcp47Code, "en");

    ASSERT_TRUE(m.summary.has_value());
    EXPECT_EQ(m.summary->text, "Shipped release, pending approval.");
    EXPECT_EQ(m.summary->keyPoints, (std::vector<std::string>{"key point 1"}));

    ASSERT_EQ(m.actionItems.size(), 2U);
    EXPECT_TRUE(m.actionItems[0].owner.has_value());
    EXPECT_TRUE(m.actionItems[0].deadline.has_value());
    EXPECT_FALSE(m.actionItems[1].owner.has_value());
    EXPECT_FALSE(m.actionItems[1].deadline.has_value());

    ASSERT_EQ(m.decisions.size(), 1U);
    EXPECT_EQ(m.decisions[0].text, "We will ship Friday.");

    ASSERT_EQ(m.topics.size(), 1U);
    EXPECT_EQ(m.topics[0].relatedSegments[0].value, "seg1");

    ASSERT_EQ(m.questions.size(), 1U);
    EXPECT_FALSE(m.questions[0].resolved);
}

TEST(MeetingSerializer, RoundTripsAnEmptyMeeting) {
    core::Meeting empty;
    empty.id = core::MeetingId{"empty"};
    empty.range = core::TimeRange{ts(0), ts(0)};

    auto result = MeetingSerializer::deserialize(MeetingSerializer::serialize(empty));
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().id.value, "empty");
    EXPECT_TRUE(result.value().speakers.empty());
    EXPECT_TRUE(result.value().transcript.empty());
    EXPECT_FALSE(result.value().summary.has_value());
}

TEST(MeetingSerializer, DeserializeOfTruncatedBytesReturnsCorruptRecordError) {
    auto bytes = MeetingSerializer::serialize(makeSampleMeeting());
    bytes.resize(bytes.size() / 2);  // truncate mid-record
    auto result = MeetingSerializer::deserialize(bytes);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "storage.corrupt_record");
}

}  // namespace
}  // namespace meeting_sdk::storage
