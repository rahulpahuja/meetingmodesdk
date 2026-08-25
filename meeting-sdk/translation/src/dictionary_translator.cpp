#include "meeting_sdk/translation/dictionary_translator.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <utility>
#include <vector>

namespace meeting_sdk::translation {
namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Splits a whitespace-delimited token into {alnum core, trailing punctuation}, e.g.
// "namaste," -> {"namaste", ","}, so substitution doesn't have to swallow sentence punctuation.
std::pair<std::string, std::string> splitTrailingPunctuation(const std::string& word) {
    std::size_t end = word.size();
    while (end > 0 && !std::isalnum(static_cast<unsigned char>(word[end - 1]))) {
        --end;
    }
    return {word.substr(0, end), word.substr(end)};
}

}  // namespace

DictionaryTranslator::DictionaryTranslator() {
    const std::vector<std::pair<std::string, std::string>> hiToEn = {
        {"namaste", "hello"}, {"dhanyavad", "thank you"}, {"haan", "yes"}, {"nahi", "no"},
        {"aaj", "today"},     {"accha", "good"},          {"theek", "okay"}, {"hum", "we"},
        {"aap", "you"},       {"kya", "what"},            {"kahan", "where"}, {"kaise", "how"},
    };
    for (const auto& [hi, en] : hiToEn) {
        hindiToEnglish_[hi] = en;
        englishToHindi_[en] = hi;
    }
}

core::Result<std::string> DictionaryTranslator::translate(const std::string& text,
                                                            const std::string& targetBcp47) {
    const std::unordered_map<std::string, std::string>* dictionary = nullptr;
    if (targetBcp47 == "en") {
        dictionary = &hindiToEnglish_;
    } else if (targetBcp47 == "hi") {
        dictionary = &englishToHindi_;
    } else {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "translation.unsupported_target_language",
            .message = "DictionaryTranslator only supports targetBcp47 'en' or 'hi', got '" +
                       targetBcp47 + "'",
        };
    }

    std::istringstream stream{text};
    std::string token;
    std::vector<std::string> outputWords;
    while (stream >> token) {
        auto [core_, trailingPunctuation] = splitTrailingPunctuation(token);
        const auto it = dictionary->find(toLower(core_));
        outputWords.push_back((it != dictionary->end() ? it->second : core_) + trailingPunctuation);
    }

    std::string result;
    for (std::size_t i = 0; i < outputWords.size(); ++i) {
        if (i > 0) {
            result += " ";
        }
        result += outputWords[i];
    }
    return result;
}

}  // namespace meeting_sdk::translation
