#include "meeting_sdk/intelligence/word_frequency.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace meeting_sdk::intelligence {
namespace {

const std::unordered_set<std::string> kStopwords = {
    "a",     "an",   "the",   "is",    "are",  "was",  "were", "am",   "be",   "been", "being",
    "and",   "or",   "but",   "to",    "of",   "in",   "on",   "for",  "with", "this", "that",
    "these", "those", "we",   "i",     "you",  "it",   "they", "he",   "she",  "will", "have",
    "has",   "had",  "not",   "so",    "do",   "does", "did",  "at",   "as",   "by",   "from",
    "if",    "then",
};

bool isStopword(const std::string& word) { return kStopwords.count(word) > 0; }

}  // namespace

std::vector<std::string> WordFrequencyAnalyzer::contentWords(const std::string& text) const {
    std::vector<std::string> words;
    std::string current;
    auto flush = [&] {
        if (current.empty()) {
            return;
        }
        std::transform(current.begin(), current.end(), current.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!isStopword(current)) {
            words.push_back(current);
        }
        current.clear();
    };
    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '\'') {
            current.push_back(ch);
        } else {
            flush();
        }
    }
    flush();
    return words;
}

std::unordered_map<std::string, int> WordFrequencyAnalyzer::countWords(
    const std::vector<std::string>& texts) const {
    std::unordered_map<std::string, int> counts;
    for (const auto& text : texts) {
        for (const auto& word : contentWords(text)) {
            ++counts[word];
        }
    }
    return counts;
}

}  // namespace meeting_sdk::intelligence
