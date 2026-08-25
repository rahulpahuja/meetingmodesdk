#include "meeting_sdk/speech/heuristic_language_detector.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <sstream>
#include <string>
#include <string_view>

namespace meeting_sdk::speech {
namespace {

constexpr std::array<std::string_view, 22> kRomanizedHindiWords = {
    "kal",  "hum",   "hain",  "kar",   "sakte", "abhi", "kya",  "nahi", "hai",  "yeh",   "woh",
    "aur",  "lekin", "accha", "theek", "haan",  "nahin", "kaise", "kahan", "kyun", "tum", "aap",
};

std::string stripPunctuation(std::string_view word) {
    std::size_t begin = 0;
    std::size_t end = word.size();
    while (begin < end && std::ispunct(static_cast<unsigned char>(word[begin]))) {
        ++begin;
    }
    while (end > begin && std::ispunct(static_cast<unsigned char>(word[end - 1]))) {
        --end;
    }
    return std::string(word.substr(begin, end - begin));
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Devanagari (U+0900-U+097F) encodes in UTF-8 as 0xE0 0xA4-0xA5 0x80-0xBF.
bool containsDevanagari(std::string_view word) {
    for (std::size_t i = 0; i + 2 < word.size(); ++i) {
        const auto b0 = static_cast<unsigned char>(word[i]);
        const auto b1 = static_cast<unsigned char>(word[i + 1]);
        if (b0 == 0xE0 && (b1 == 0xA4 || b1 == 0xA5)) {
            return true;
        }
    }
    return false;
}

bool isRomanizedHindi(const std::string& lowerWord) {
    return std::find(kRomanizedHindiWords.begin(), kRomanizedHindiWords.end(), lowerWord) !=
           kRomanizedHindiWords.end();
}

std::string classify(std::string_view rawWord) {
    if (containsDevanagari(rawWord)) {
        return "hi";
    }
    return isRomanizedHindi(toLower(stripPunctuation(rawWord))) ? "hi" : "en";
}

core::Timestamp interpolate(core::Timestamp start, core::Timestamp end, double fraction) {
    // system_clock's native duration resolution varies by platform (microseconds on some
    // libc++ builds, nanoseconds elsewhere) — compute in nanoseconds, then cast back down so
    // the addition below always compiles regardless of that resolution.
    const auto totalNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end.value - start.value).count();
    const auto offsetNs = static_cast<long long>(static_cast<double>(totalNs) * fraction);
    const auto offset =
        std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(offsetNs));
    return core::Timestamp{start.value + offset};
}

}  // namespace

core::Result<std::vector<core::LanguageSegment>> HeuristicLanguageDetector::detect(
    const core::TranscriptSegment& segment) {
    std::vector<std::string> words;
    std::istringstream stream{segment.text};
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    if (words.empty()) {
        return std::vector<core::LanguageSegment>{};
    }

    std::vector<core::LanguageSegment> result;
    std::size_t groupStart = 0;
    std::string groupLang = classify(words[0]);

    auto flush = [&](std::size_t groupEndExclusive) {
        core::LanguageSegment seg;
        seg.language = core::Language{.bcp47Code = groupLang, .confidence = 0.6F};
        seg.range = core::TimeRange{
            interpolate(segment.range.start, segment.range.end,
                        static_cast<double>(groupStart) / static_cast<double>(words.size())),
            interpolate(segment.range.start, segment.range.end,
                        static_cast<double>(groupEndExclusive) / static_cast<double>(words.size())),
        };
        result.push_back(std::move(seg));
    };

    for (std::size_t i = 1; i < words.size(); ++i) {
        std::string lang = classify(words[i]);
        if (lang != groupLang) {
            flush(i);
            groupStart = i;
            groupLang = std::move(lang);
        }
    }
    flush(words.size());

    return result;
}

}  // namespace meeting_sdk::speech
