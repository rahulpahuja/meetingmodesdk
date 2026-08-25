#include "meeting_sdk/intelligence/sentence_splitter.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>

namespace meeting_sdk::intelligence {
namespace {

TEST(SplitSentences, SplitsOnTerminatingPunctuation) {
    auto sentences = splitSentences("We will ship Friday. Are we ready? Great!");
    ASSERT_EQ(sentences.size(), 3U);
    EXPECT_EQ(sentences[0], "We will ship Friday.");
    EXPECT_EQ(sentences[1], "Are we ready?");
    EXPECT_EQ(sentences[2], "Great!");
}

TEST(SplitSentences, TrimsWhitespaceAroundSentences) {
    auto sentences = splitSentences("  Hello world.   Goodbye.  ");
    ASSERT_EQ(sentences.size(), 2U);
    EXPECT_EQ(sentences[0], "Hello world.");
    EXPECT_EQ(sentences[1], "Goodbye.");
}

TEST(SplitSentences, KeepsATrailingFragmentWithoutTerminator) {
    auto sentences = splitSentences("Finished sentence. trailing fragment");
    ASSERT_EQ(sentences.size(), 2U);
    EXPECT_EQ(sentences[1], "trailing fragment");
}

TEST(SplitSentences, EmptyTextYieldsNoSentences) {
    EXPECT_TRUE(splitSentences("").empty());
}

TEST(SplitSentences, NeverEmitsAPunctuationOnlyFragment) {
    // An ellipsis mid-sentence still causes a split after the word before it (this splitter
    // has no lookahead), but it must never emit a bare "." or ".." as its own "sentence".
    auto sentences = splitSentences("Wait... really?");
    for (const auto& sentence : sentences) {
        EXPECT_TRUE(std::any_of(sentence.begin(), sentence.end(),
                                 [](unsigned char c) { return static_cast<bool>(std::isalnum(c)); }));
    }
}

}  // namespace
}  // namespace meeting_sdk::intelligence
