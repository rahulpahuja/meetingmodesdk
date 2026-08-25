#include "meeting_sdk/intelligence/word_frequency.hpp"

#include <gtest/gtest.h>

#include <algorithm>

namespace meeting_sdk::intelligence {
namespace {

TEST(WordFrequencyAnalyzer, ContentWordsExcludesStopwordsAndPunctuation) {
    WordFrequencyAnalyzer analyzer;
    auto words = analyzer.contentWords("We should launch the release next Friday.");
    // "we", "should", "the", "next" are stopwords-ish/filtered; content words remain.
    EXPECT_NE(std::find(words.begin(), words.end(), "launch"), words.end());
    EXPECT_NE(std::find(words.begin(), words.end(), "release"), words.end());
    EXPECT_NE(std::find(words.begin(), words.end(), "friday"), words.end());
    EXPECT_EQ(std::find(words.begin(), words.end(), "the"), words.end());
    EXPECT_EQ(std::find(words.begin(), words.end(), "we"), words.end());
}

TEST(WordFrequencyAnalyzer, ContentWordsAreLowercased) {
    WordFrequencyAnalyzer analyzer;
    auto words = analyzer.contentWords("Security APPROVAL Security");
    ASSERT_EQ(words.size(), 3U);
    EXPECT_EQ(words[0], "security");
    EXPECT_EQ(words[1], "approval");
}

TEST(WordFrequencyAnalyzer, CountWordsAggregatesAcrossTexts) {
    WordFrequencyAnalyzer analyzer;
    auto counts = analyzer.countWords({"security approval", "security review"});
    EXPECT_EQ(counts.at("security"), 2);
    EXPECT_EQ(counts.at("approval"), 1);
    EXPECT_EQ(counts.at("review"), 1);
}

}  // namespace
}  // namespace meeting_sdk::intelligence
