#pragma once

#include <string>
#include <unordered_map>

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::translation {

// Word-for-word dictionary substitution between a small closed Hindi<->English vocabulary.
// This is a crude, deterministic on-device translation baseline — no grammar, no reordering,
// no context — not real machine translation. It needs no model or network call, so it's a
// legitimate ON_DEVICE default until a real NMT model is bundled behind providers/on_device.
// Words not found in the dictionary are left unchanged, the standard fallback for
// dictionary-based MT; trailing punctuation on a word is preserved across substitution.
class DictionaryTranslator final : public core::ITranslator {
public:
    DictionaryTranslator();

    // targetBcp47 must be "en" or "hi" — anything else is a Configuration error.
    core::Result<std::string> translate(const std::string& text, const std::string& targetBcp47) override;

private:
    std::unordered_map<std::string, std::string> hindiToEnglish_;
    std::unordered_map<std::string, std::string> englishToHindi_;
};

}  // namespace meeting_sdk::translation
