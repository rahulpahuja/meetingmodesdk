#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "meeting_sdk/core/types.hpp"

namespace meeting_sdk::core {

struct SpeakerEmbedding {
    SpeakerId speaker;
    std::vector<float> vector;
    Timestamp capturedAt;
};

// Mutable aggregate: identity persists as a speaker is re-clustered/renamed by the user.
struct Speaker {
    SpeakerId id;
    std::optional<std::string> displayName;
    std::vector<SpeakerEmbedding> embeddings;
};

// Immutable once assembled; corrections create a new segment rather than mutating this one.
struct TranscriptSegment {
    SegmentId id;
    TimeRange range;
    SpeakerId speaker;
    std::string text;
    Language detectedLanguage;
    float confidence = 0.0F;
    std::vector<LanguageSegment> languageSegments;  // empty if monolingual
};

struct ActionItem {
    std::string action;
    std::optional<std::string> owner;
    std::optional<Timestamp> deadline;
    SegmentId sourceSegment;
    float confidence = 0.0F;
};

struct Decision {
    std::string text;
    SegmentId sourceSegment;
    float confidence = 0.0F;
};

struct Topic {
    std::string label;
    std::vector<SegmentId> relatedSegments;
};

struct Question {
    std::string text;
    SegmentId sourceSegment;
    bool resolved = false;
};

struct Summary {
    std::string text;
    std::vector<std::string> keyPoints;
};

// State pattern: transitions are validated centrally by orchestration::MeetingStateMachine,
// not scattered across call sites that would otherwise set this enum directly.
enum class MeetingState {
    Idle,
    Recording,
    Paused,
    Processing,
    Completed,
    Failed,
};

std::string_view toString(MeetingState state) noexcept;

struct Meeting {
    MeetingId id;
    MeetingState state = MeetingState::Idle;
    TimeRange range;
    std::vector<Speaker> speakers;
    std::vector<TranscriptSegment> transcript;
    std::optional<Summary> summary;
    std::vector<ActionItem> actionItems;
    std::vector<Decision> decisions;
    std::vector<Topic> topics;
    std::vector<Question> questions;
};

enum class ProviderMode {
    OnDevice,
    Cloud,
    Disabled,
};

// One independently-configurable slot per provider family (see threat model §6): enabling
// cloud for one capability must never implicitly enable it for another.
struct AIProviderConfig {
    ProviderMode stt = ProviderMode::OnDevice;
    ProviderMode llm = ProviderMode::OnDevice;
    ProviderMode translation = ProviderMode::Disabled;
    ProviderMode embeddings = ProviderMode::OnDevice;
    ProviderMode diarization = ProviderMode::OnDevice;
};

}  // namespace meeting_sdk::core
