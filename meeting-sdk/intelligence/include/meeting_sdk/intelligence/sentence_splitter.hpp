#pragma once

#include <string>
#include <vector>

namespace meeting_sdk::intelligence {

// Splits text into trimmed sentences on '.', '!', '?' boundaries, keeping the terminating
// punctuation as the last character of each sentence — callers use it to distinguish
// questions from statements. Empty sentences (e.g. from "..." or trailing whitespace) are
// dropped. A trailing fragment with no terminator is still returned as a sentence.
std::vector<std::string> splitSentences(const std::string& text);

}  // namespace meeting_sdk::intelligence
