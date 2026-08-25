#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace meeting_sdk::intelligence {

// Tokenizes text into lowercased content words, filtering a small closed English stopword
// list and punctuation, and counts frequency. Shared by KeywordTopicExtractor and
// ExtractiveSummarizer so both rank content by the same notion of "salient word" — a real
// statistical or embedding-based ranking is a swappable ILLMEngine provider upgrade, not a
// change to this utility's contract.
class WordFrequencyAnalyzer {
public:
    // Word -> occurrence count across all given texts, stopwords and punctuation excluded.
    std::unordered_map<std::string, int> countWords(const std::vector<std::string>& texts) const;

    // Content words from one text, in order, lowercased, stopwords excluded.
    std::vector<std::string> contentWords(const std::string& text) const;
};

}  // namespace meeting_sdk::intelligence
