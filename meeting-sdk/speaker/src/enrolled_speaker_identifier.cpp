#include "meeting_sdk/speaker/enrolled_speaker_identifier.hpp"

#include "meeting_sdk/speaker/embedding_similarity.hpp"

namespace meeting_sdk::speaker {

EnrolledSpeakerIdentifier::EnrolledSpeakerIdentifier(float similarityThreshold)
    : similarityThreshold_(similarityThreshold) {}

void EnrolledSpeakerIdentifier::enroll(std::string name, core::SpeakerEmbedding referenceEmbedding) {
    enrolled_.emplace_back(std::move(name), std::move(referenceEmbedding));
}

core::Result<std::optional<std::string>> EnrolledSpeakerIdentifier::identify(
    const core::SpeakerEmbedding& embedding) {
    const std::string* bestName = nullptr;
    float bestSimilarity = similarityThreshold_;  // must exceed threshold, not just be highest
    for (const auto& [name, reference] : enrolled_) {
        const float similarity = cosineSimilarity(embedding.vector, reference.vector);
        if (similarity > bestSimilarity) {
            bestSimilarity = similarity;
            bestName = &name;
        }
    }
    if (bestName == nullptr) {
        return std::optional<std::string>{std::nullopt};
    }
    return std::optional<std::string>{*bestName};
}

}  // namespace meeting_sdk::speaker
