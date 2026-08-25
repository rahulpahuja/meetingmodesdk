#include "meeting_sdk/speaker/embedding_similarity.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::speaker {
namespace {

TEST(CosineSimilarity, IdenticalVectorsAreOne) {
    EXPECT_NEAR(cosineSimilarity({1.F, 0.F, 0.F}, {1.F, 0.F, 0.F}), 1.0F, 1e-5F);
}

TEST(CosineSimilarity, OrthogonalVectorsAreZero) {
    EXPECT_NEAR(cosineSimilarity({1.F, 0.F}, {0.F, 1.F}), 0.0F, 1e-5F);
}

TEST(CosineSimilarity, OppositeVectorsAreMinusOne) {
    EXPECT_NEAR(cosineSimilarity({1.F, 0.F}, {-1.F, 0.F}), -1.0F, 1e-5F);
}

TEST(CosineSimilarity, ZeroMagnitudeVectorIsTreatedAsNoSignal) {
    EXPECT_EQ(cosineSimilarity({0.F, 0.F}, {1.F, 0.F}), 0.0F);
}

TEST(CosineSimilarity, DimensionMismatchIsTreatedAsNoSignal) {
    EXPECT_EQ(cosineSimilarity({1.F, 0.F}, {1.F, 0.F, 0.F}), 0.0F);
}

}  // namespace
}  // namespace meeting_sdk::speaker
