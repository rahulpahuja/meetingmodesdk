#include "meeting_sdk/core/errors.hpp"

namespace meeting_sdk::core {

std::string_view toString(ErrorCategory category) noexcept {
    switch (category) {
        case ErrorCategory::Audio:
            return "Audio";
        case ErrorCategory::Transcription:
            return "Transcription";
        case ErrorCategory::Model:
            return "Model";
        case ErrorCategory::Storage:
            return "Storage";
        case ErrorCategory::Network:
            return "Network";
        case ErrorCategory::Permission:
            return "Permission";
        case ErrorCategory::Configuration:
            return "Configuration";
        case ErrorCategory::Security:
            return "Security";
        case ErrorCategory::Cancellation:
            return "Cancellation";
    }
    return "Unknown";
}

}  // namespace meeting_sdk::core
