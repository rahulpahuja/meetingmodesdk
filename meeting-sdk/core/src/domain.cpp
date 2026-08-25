#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::core {

std::string_view toString(MeetingState state) noexcept {
    switch (state) {
        case MeetingState::Idle:
            return "Idle";
        case MeetingState::Recording:
            return "Recording";
        case MeetingState::Paused:
            return "Paused";
        case MeetingState::Processing:
            return "Processing";
        case MeetingState::Completed:
            return "Completed";
        case MeetingState::Failed:
            return "Failed";
    }
    return "Unknown";
}

}  // namespace meeting_sdk::core
