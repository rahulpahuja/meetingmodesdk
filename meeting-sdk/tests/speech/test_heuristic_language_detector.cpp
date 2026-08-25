#include "meeting_sdk/speech/heuristic_language_detector.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>

namespace meeting_sdk::speech {
namespace {

core::TranscriptSegment makeSegment(std::string text) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.range.start = core::Timestamp{std::chrono::system_clock::time_point{}};
    segment.range.end =
        core::Timestamp{std::chrono::system_clock::time_point{} + std::chrono::seconds(10)};
    return segment;
}

TEST(HeuristicLanguageDetector, EmptyTextYieldsNoSegments) {
    HeuristicLanguageDetector detector;
    auto result = detector.detect(makeSegment(""));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value().empty());
}

TEST(HeuristicLanguageDetector, PureEnglishIsOneSegment) {
    HeuristicLanguageDetector detector;
    auto result = detector.detect(makeSegment("We should launch this next Friday."));
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value()[0].language.bcp47Code, "en");
}

TEST(HeuristicLanguageDetector, PureDevanagariIsOneHindiSegment) {
    HeuristicLanguageDetector detector;
    auto result = detector.detect(makeSegment("नमस्ते दोस्तों"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value()[0].language.bcp47Code, "hi");
}

TEST(HeuristicLanguageDetector, DetectsHindiEnglishCodeSwitching) {
    HeuristicLanguageDetector detector;
    auto result = detector.detect(
        makeSegment("Kal hum release kar sakte hain, but security approval abhi pending hai."));
    ASSERT_TRUE(result);
    const auto& segments = result.value();
    ASSERT_GT(segments.size(), 1U);

    bool sawHindi = false;
    bool sawEnglish = false;
    for (const auto& seg : segments) {
        if (seg.language.bcp47Code == "hi") sawHindi = true;
        if (seg.language.bcp47Code == "en") sawEnglish = true;
    }
    EXPECT_TRUE(sawHindi);
    EXPECT_TRUE(sawEnglish);
}

TEST(HeuristicLanguageDetector, SegmentTimeRangesCoverTheWholeSpanInOrder) {
    HeuristicLanguageDetector detector;
    auto result = detector.detect(makeSegment("Kal hum release."));
    ASSERT_TRUE(result);
    const auto& segments = result.value();
    ASSERT_FALSE(segments.empty());
    EXPECT_EQ(segments.front().range.start.value, std::chrono::system_clock::time_point{});
    EXPECT_EQ(segments.back().range.end.value,
              std::chrono::system_clock::time_point{} + std::chrono::seconds(10));
    for (std::size_t i = 1; i < segments.size(); ++i) {
        EXPECT_EQ(segments[i].range.start.value, segments[i - 1].range.end.value);
    }
}

}  // namespace
}  // namespace meeting_sdk::speech
