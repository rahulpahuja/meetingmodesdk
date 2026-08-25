#include "meeting_sdk/speaker/embedding_similarity.hpp"

#include <cmath>
#include <cstddef>

namespace meeting_sdk::speaker {

float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0F;
    }
    double dot = 0.0;
    double normA = 0.0;
    double normB = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        normA += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        normB += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }
    if (normA == 0.0 || normB == 0.0) {
        return 0.0F;
    }
    return static_cast<float>(dot / (std::sqrt(normA) * std::sqrt(normB)));
}

}  // namespace meeting_sdk::speaker
