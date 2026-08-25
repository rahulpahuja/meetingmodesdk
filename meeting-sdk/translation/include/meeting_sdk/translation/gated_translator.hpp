#pragma once

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::translation {

// Enforces core::ProviderMode before any translate() call reaches the wrapped translator —
// "Cloud translation must never silently activate" (product spec §12). Constructed with the
// mode the host app configured (core::AIProviderConfig::translation) and the translator that
// mode implies. DISABLED rejects every call before it can reach any translator at all, cloud
// or on-device; ON_DEVICE/CLOUD still require a non-null translator to have been supplied —
// there is no fallback path that silently swaps one mode's translator for another's.
class GatedTranslator final : public core::ITranslator {
public:
    GatedTranslator(core::ProviderMode mode, core::ITranslator* translator);

    core::Result<std::string> translate(const std::string& text, const std::string& targetBcp47) override;

private:
    core::ProviderMode mode_;
    core::ITranslator* translator_;
};

}  // namespace meeting_sdk::translation
