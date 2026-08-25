#include "meeting_sdk/orchestration/meeting_state_machine.hpp"

#include <string>

namespace meeting_sdk::orchestration {

using core::MeetingState;

namespace {

bool isAllowed(MeetingState from, MeetingState to) {
    switch (from) {
        case MeetingState::Idle:
            return to == MeetingState::Recording;
        case MeetingState::Recording:
            return to == MeetingState::Paused || to == MeetingState::Processing ||
                   to == MeetingState::Failed;
        case MeetingState::Paused:
            return to == MeetingState::Recording || to == MeetingState::Processing ||
                   to == MeetingState::Failed;
        case MeetingState::Processing:
            return to == MeetingState::Completed || to == MeetingState::Failed;
        case MeetingState::Completed:
        case MeetingState::Failed:
            return false;  // terminal
    }
    return false;
}

}  // namespace

MeetingStateMachine::MeetingStateMachine(MeetingState initial) : state_(initial) {}

MeetingState MeetingStateMachine::current() const noexcept { return state_; }

core::Result<MeetingState> MeetingStateMachine::transitionTo(MeetingState target) {
    if (!isAllowed(state_, target)) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "orchestration.illegal_state_transition",
            .message = std::string("cannot transition from ") + std::string(core::toString(state_)) +
                       " to " + std::string(core::toString(target)),
        };
    }
    state_ = target;
    return state_;
}

}  // namespace meeting_sdk::orchestration
