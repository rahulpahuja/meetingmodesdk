#include "meeting_sdk/speaker/speaker_clusterer.hpp"

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

TEST(SpeakerClusterer, FirstEmbeddingCreatesANewSpeaker) {
    SpeakerClusterer clusterer;
    auto id = clusterer.assign(makeEmbedding({1.F, 0.F, 0.F}));
    EXPECT_FALSE(id.value.empty());
    EXPECT_EQ(clusterer.speakers().size(), 1U);
}

TEST(SpeakerClusterer, SimilarEmbeddingJoinsTheSameSpeaker) {
    SpeakerClusterer clusterer(/*similarityThreshold=*/0.9F);
    auto first = clusterer.assign(makeEmbedding({1.F, 0.F, 0.F}));
    auto second = clusterer.assign(makeEmbedding({0.99F, 0.01F, 0.F}));

    EXPECT_EQ(first, second);
    ASSERT_EQ(clusterer.speakers().size(), 1U);
    EXPECT_EQ(clusterer.speakers()[0].embeddings.size(), 2U);
}

TEST(SpeakerClusterer, DissimilarEmbeddingCreatesANewSpeaker) {
    SpeakerClusterer clusterer(/*similarityThreshold=*/0.9F);
    auto first = clusterer.assign(makeEmbedding({1.F, 0.F, 0.F}));
    auto second = clusterer.assign(makeEmbedding({0.F, 1.F, 0.F}));

    EXPECT_NE(first, second);
    EXPECT_EQ(clusterer.speakers().size(), 2U);
}

TEST(SpeakerClusterer, SimilarityMustStrictlyExceedThreshold) {
    // Two vectors 45 degrees apart have cosine similarity ~0.707 — below a 0.9 threshold.
    SpeakerClusterer clusterer(/*similarityThreshold=*/0.9F);
    clusterer.assign(makeEmbedding({1.F, 0.F}));
    clusterer.assign(makeEmbedding({0.707F, 0.707F}));
    EXPECT_EQ(clusterer.speakers().size(), 2U);
}

}  // namespace
}  // namespace meeting_sdk::speaker
