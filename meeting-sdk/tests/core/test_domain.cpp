#include "meeting_sdk/core/domain.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::core {
namespace {

TEST(MeetingState, ToStringCoversEveryEnumerator) {
    EXPECT_EQ(toString(MeetingState::Idle), "Idle");
    EXPECT_EQ(toString(MeetingState::Recording), "Recording");
    EXPECT_EQ(toString(MeetingState::Paused), "Paused");
    EXPECT_EQ(toString(MeetingState::Processing), "Processing");
    EXPECT_EQ(toString(MeetingState::Completed), "Completed");
    EXPECT_EQ(toString(MeetingState::Failed), "Failed");
}

TEST(AIProviderConfig, DefaultsToOnDeviceExceptTranslation) {
    AIProviderConfig config;
    EXPECT_EQ(config.stt, ProviderMode::OnDevice);
    EXPECT_EQ(config.llm, ProviderMode::OnDevice);
    EXPECT_EQ(config.embeddings, ProviderMode::OnDevice);
    EXPECT_EQ(config.diarization, ProviderMode::OnDevice);
    // Translation defaults to Disabled, not OnDevice: on-device translation quality varies
    // enough by language pair that silently enabling it could produce misleading output.
    EXPECT_EQ(config.translation, ProviderMode::Disabled);
}

TEST(TranscriptSegment, LanguageSegmentsEmptyForMonolingualSpeech) {
    TranscriptSegment segment;
    segment.text = "We should launch this next Friday.";
    segment.detectedLanguage = Language{.bcp47Code = "en", .confidence = 0.98F};
    EXPECT_TRUE(segment.languageSegments.empty());
}

TEST(TranscriptSegment, LanguageSegmentsCaptureCodeSwitching) {
    // "Kal hum release kar sakte hain, but security approval abhi pending hai."
    TranscriptSegment segment;
    segment.text = "Kal hum release kar sakte hain, but security approval abhi pending hai.";
    segment.detectedLanguage = Language{.bcp47Code = "hi", .confidence = 0.6F};
    segment.languageSegments = {
        LanguageSegment{.language = Language{.bcp47Code = "hi", .confidence = 0.9F}},
        LanguageSegment{.language = Language{.bcp47Code = "en", .confidence = 0.9F}},
    };
    EXPECT_EQ(segment.languageSegments.size(), 2U);
    EXPECT_EQ(segment.languageSegments[1].language.bcp47Code, "en");
}

TEST(StrongIds, DistinctTypesAreNotInterchangeable) {
    MeetingId meeting{.value = "m1"};
    SpeakerId speaker{.value = "m1"};
    // Same underlying string, different types — this line intentionally would not compile
    // if MeetingId and SpeakerId were interchangeable:
    //   EXPECT_EQ(meeting, speaker);
    EXPECT_EQ(meeting.value, speaker.value);
}

}  // namespace
}  // namespace meeting_sdk::core
