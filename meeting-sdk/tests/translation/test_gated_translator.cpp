#include "meeting_sdk/translation/gated_translator.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::translation {
namespace {

class FakeTranslator final : public core::ITranslator {
public:
    core::Result<std::string> translate(const std::string& text, const std::string& /*targetBcp47*/) override {
        ++callCount;
        return "translated:" + text;
    }

    int callCount = 0;
};

TEST(GatedTranslator, DisabledRejectsBeforeReachingTheWrappedTranslator) {
    FakeTranslator fake;
    GatedTranslator gated(core::ProviderMode::Disabled, &fake);

    auto result = gated.translate("hello", "hi");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::Configuration);
    EXPECT_EQ(result.error().code, "translation.disabled");
    EXPECT_EQ(fake.callCount, 0);
}

TEST(GatedTranslator, OnDeviceForwardsToTheWrappedTranslator) {
    FakeTranslator fake;
    GatedTranslator gated(core::ProviderMode::OnDevice, &fake);

    auto result = gated.translate("hello", "hi");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "translated:hello");
    EXPECT_EQ(fake.callCount, 1);
}

TEST(GatedTranslator, CloudForwardsToTheWrappedTranslator) {
    FakeTranslator fake;
    GatedTranslator gated(core::ProviderMode::Cloud, &fake);

    auto result = gated.translate("hello", "hi");
    ASSERT_TRUE(result);
    EXPECT_EQ(fake.callCount, 1);
}

TEST(GatedTranslator, EnabledModeWithNoTranslatorInstanceIsRejected) {
    GatedTranslator gated(core::ProviderMode::Cloud, nullptr);

    auto result = gated.translate("hello", "hi");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "translation.no_provider_configured");
}

}  // namespace
}  // namespace meeting_sdk::translation
