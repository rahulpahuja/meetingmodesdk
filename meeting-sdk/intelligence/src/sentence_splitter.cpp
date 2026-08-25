#include "meeting_sdk/intelligence/sentence_splitter.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>

namespace meeting_sdk::intelligence {
namespace {

std::string trim(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(begin, end - begin);
}

bool hasAlnum(const std::string& s) {
    return std::any_of(s.begin(), s.end(),
                        [](unsigned char c) { return static_cast<bool>(std::isalnum(c)); });
}

}  // namespace

std::vector<std::string> splitSentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::string current;
    for (char ch : text) {
        current.push_back(ch);
        if (ch == '.' || ch == '!' || ch == '?') {
            std::string trimmed = trim(current);
            // A run of bare punctuation (e.g. mid-ellipsis "...") has no alnum content —
            // keep accumulating instead of emitting it as its own "sentence".
            if (hasAlnum(trimmed)) {
                sentences.push_back(std::move(trimmed));
                current.clear();
            }
        }
    }
    std::string trimmed = trim(current);
    if (!trimmed.empty()) {
        sentences.push_back(std::move(trimmed));
    }
    return sentences;
}

}  // namespace meeting_sdk::intelligence
