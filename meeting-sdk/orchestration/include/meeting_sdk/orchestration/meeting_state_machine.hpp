#pragma once

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/core/errors.hpp"

namespace meeting_sdk::orchestration {

// Centralizes MeetingState transition validation (State pattern — see
// docs/architecture/01-requirements-and-architecture.md §4) so no call site sets
// Meeting::state directly. Not thread-safe by itself: orchestration serializes state
// transitions onto a single processing thread, per the threading contract in
// docs/architecture/02-interfaces-and-data-models.md §3.5.
class MeetingStateMachine {
public:
    explicit MeetingStateMachine(core::MeetingState initial = core::MeetingState::Idle);

    core::MeetingState current() const noexcept;

    // On success, returns the new state. On an illegal transition, returns a
    // Configuration-category Error and leaves current() unchanged.
    core::Result<core::MeetingState> transitionTo(core::MeetingState target);

private:
    core::MeetingState state_;
};

}  // namespace meeting_sdk::orchestration
