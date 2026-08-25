#include "meeting_sdk/translation/dictionary_translator.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::translation {
namespace {

TEST(DictionaryTranslator, TranslatesKnownHindiWordsToEnglish) {
    DictionaryTranslator translator;
    auto result = translator.translate("namaste, kaise hain aap", "en");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "hello, how hain you");
}

TEST(DictionaryTranslator, TranslatesKnownEnglishWordsToHindi) {
    DictionaryTranslator translator;
    // "hello", "how", and "you" are all reverse-dictionary entries; "are" has no mapping.
    auto result = translator.translate("hello, how are you", "hi");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "namaste, kaise are aap");
}

TEST(DictionaryTranslator, LeavesUnknownWordsUnchanged) {
    DictionaryTranslator translator;
    auto result = translator.translate("blockchain namaste", "en");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "blockchain hello");
}

TEST(DictionaryTranslator, LookupIsCaseInsensitive) {
    DictionaryTranslator translator;
    auto result = translator.translate("NAMASTE", "en");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "hello");
}

TEST(DictionaryTranslator, RejectsUnsupportedTargetLanguage) {
    DictionaryTranslator translator;
    auto result = translator.translate("namaste", "fr");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::Configuration);
    EXPECT_EQ(result.error().code, "translation.unsupported_target_language");
}

}  // namespace
}  // namespace meeting_sdk::translation
