#include "meeting_sdk/speaker/enrolled_speaker_identifier.hpp"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace meeting_sdk::speaker {
namespace {

core::SpeakerEmbedding makeEmbedding(std::vector<float> vec) {
    core::SpeakerEmbedding embedding;
    embedding.vector = std::move(vec);
    return embedding;
}

TEST(EnrolledSpeakerIdentifier, MatchesAnEnrolledVoiceprint) {
    EnrolledSpeakerIdentifier identifier;
    identifier.enroll("Alice", makeEmbedding({1.F, 0.F, 0.F}));

    auto result = identifier.identify(makeEmbedding({0.99F, 0.01F, 0.F}));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().has_value());
    EXPECT_EQ(*result.value(), "Alice");
}

TEST(EnrolledSpeakerIdentifier, ReturnsNulloptForAnUnrecognizedVoice) {
    EnrolledSpeakerIdentifier identifier;
    identifier.enroll("Alice", makeEmbedding({1.F, 0.F, 0.F}));

    auto result = identifier.identify(makeEmbedding({0.F, 1.F, 0.F}));
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value().has_value());
}

TEST(EnrolledSpeakerIdentifier, PicksTheNearestOfMultipleEnrolledSpeakers) {
    EnrolledSpeakerIdentifier identifier;
    identifier.enroll("Alice", makeEmbedding({1.F, 0.F, 0.F}));
    identifier.enroll("Bob", makeEmbedding({0.F, 1.F, 0.F}));

    auto result = identifier.identify(makeEmbedding({0.05F, 0.95F, 0.F}));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value().has_value());
    EXPECT_EQ(*result.value(), "Bob");
}

}  // namespace
}  // namespace meeting_sdk::speaker
