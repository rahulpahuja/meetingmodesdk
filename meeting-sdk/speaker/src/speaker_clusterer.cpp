#include "meeting_sdk/speaker/speaker_clusterer.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "meeting_sdk/speaker/embedding_similarity.hpp"

namespace meeting_sdk::speaker {
namespace {

std::vector<float> centroid(const core::Speaker& speaker) {
    if (speaker.embeddings.empty()) {
        return {};
    }
    std::vector<float> sum(speaker.embeddings.front().vector.size(), 0.0F);
    for (const auto& embedding : speaker.embeddings) {
        for (std::size_t i = 0; i < sum.size() && i < embedding.vector.size(); ++i) {
            sum[i] += embedding.vector[i];
        }
    }
    for (float& value : sum) {
        value /= static_cast<float>(speaker.embeddings.size());
    }
    return sum;
}

}  // namespace

SpeakerClusterer::SpeakerClusterer(float similarityThreshold)
    : similarityThreshold_(similarityThreshold) {}

core::SpeakerId SpeakerClusterer::assign(core::SpeakerEmbedding embedding) {
    core::Speaker* best = nullptr;
    float bestSimilarity = similarityThreshold_;  // must exceed threshold, not just be highest
    for (auto& speaker : speakers_) {
        const float similarity = cosineSimilarity(centroid(speaker), embedding.vector);
        if (similarity > bestSimilarity) {
            bestSimilarity = similarity;
            best = &speaker;
        }
    }

    if (best != nullptr) {
        embedding.speaker = best->id;
        best->embeddings.push_back(std::move(embedding));
        return best->id;
    }

    core::SpeakerId newId{.value = "speaker-" + std::to_string(nextSpeakerNumber_++)};
    embedding.speaker = newId;
    core::Speaker speaker;
    speaker.id = newId;
    speaker.embeddings.push_back(std::move(embedding));
    speakers_.push_back(std::move(speaker));
    return newId;
}

const std::vector<core::Speaker>& SpeakerClusterer::speakers() const noexcept { return speakers_; }

}  // namespace meeting_sdk::speaker
