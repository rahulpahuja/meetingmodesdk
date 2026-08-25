#include "meeting_sdk/translation/gated_translator.hpp"

namespace meeting_sdk::translation {

GatedTranslator::GatedTranslator(core::ProviderMode mode, core::ITranslator* translator)
    : mode_(mode), translator_(translator) {}

core::Result<std::string> GatedTranslator::translate(const std::string& text,
                                                       const std::string& targetBcp47) {
    if (mode_ == core::ProviderMode::Disabled) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "translation.disabled",
            .message =
                "translation is disabled in AIProviderConfig; enable ON_DEVICE or CLOUD "
                "explicitly to translate",
        };
    }
    if (translator_ == nullptr) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "translation.no_provider_configured",
            .message = "translation mode is enabled but no translator instance was supplied",
        };
    }
    return translator_->translate(text, targetBcp47);
}

}  // namespace meeting_sdk::translation
