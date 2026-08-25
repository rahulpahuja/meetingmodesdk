#include "meeting_sdk/orchestration/meeting_state_machine.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::orchestration {
namespace {

using core::MeetingState;

TEST(MeetingStateMachine, StartsIdleByDefault) {
    MeetingStateMachine machine;
    EXPECT_EQ(machine.current(), MeetingState::Idle);
}

TEST(MeetingStateMachine, AllowsTheHappyPath) {
    MeetingStateMachine machine;
    ASSERT_TRUE(machine.transitionTo(MeetingState::Recording));
    ASSERT_TRUE(machine.transitionTo(MeetingState::Processing));
    ASSERT_TRUE(machine.transitionTo(MeetingState::Completed));
    EXPECT_EQ(machine.current(), MeetingState::Completed);
}

TEST(MeetingStateMachine, AllowsPauseAndResume) {
    MeetingStateMachine machine;
    ASSERT_TRUE(machine.transitionTo(MeetingState::Recording));
    ASSERT_TRUE(machine.transitionTo(MeetingState::Paused));
    ASSERT_TRUE(machine.transitionTo(MeetingState::Recording));
    EXPECT_EQ(machine.current(), MeetingState::Recording);
}

TEST(MeetingStateMachine, RejectsSkippingRecording) {
    MeetingStateMachine machine;
    auto result = machine.transitionTo(MeetingState::Processing);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::Configuration);
    EXPECT_EQ(result.error().code, "orchestration.illegal_state_transition");
    // State is unchanged after a rejected transition.
    EXPECT_EQ(machine.current(), MeetingState::Idle);
}

TEST(MeetingStateMachine, CompletedAndFailedAreTerminal) {
    MeetingStateMachine completed(MeetingState::Completed);
    EXPECT_FALSE(completed.transitionTo(MeetingState::Recording));

    MeetingStateMachine failed(MeetingState::Failed);
    EXPECT_FALSE(failed.transitionTo(MeetingState::Idle));
}

TEST(MeetingStateMachine, FailedIsReachableFromRecordingAndPaused) {
    MeetingStateMachine fromRecording;
    ASSERT_TRUE(fromRecording.transitionTo(MeetingState::Recording));
    EXPECT_TRUE(fromRecording.transitionTo(MeetingState::Failed));

    MeetingStateMachine fromPaused;
    ASSERT_TRUE(fromPaused.transitionTo(MeetingState::Recording));
    ASSERT_TRUE(fromPaused.transitionTo(MeetingState::Paused));
    EXPECT_TRUE(fromPaused.transitionTo(MeetingState::Failed));
}

}  // namespace
}  // namespace meeting_sdk::orchestration
