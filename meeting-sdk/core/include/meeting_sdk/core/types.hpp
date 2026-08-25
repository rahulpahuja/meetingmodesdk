#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace meeting_sdk::core {

// Strong IDs — prevent parameter-order bugs at construction sites (e.g. passing a
// SpeakerId where a SegmentId is expected fails to compile instead of misbehaving at runtime).
struct MeetingId {
    std::string value;
};
struct SpeakerId {
    std::string value;
};
struct SegmentId {
    std::string value;
};

inline bool operator==(const MeetingId& a, const MeetingId& b) { return a.value == b.value; }
inline bool operator==(const SpeakerId& a, const SpeakerId& b) { return a.value == b.value; }
inline bool operator==(const SegmentId& a, const SegmentId& b) { return a.value == b.value; }

struct Timestamp {
    std::chrono::system_clock::time_point value;
};
struct Duration_ {
    std::chrono::milliseconds value;
};

inline bool operator==(const Timestamp& a, const Timestamp& b) { return a.value == b.value; }
inline bool operator<(const Timestamp& a, const Timestamp& b) { return a.value < b.value; }

struct TimeRange {
    Timestamp start;
    Timestamp end;
};

struct Language {
    std::string bcp47Code;  // e.g. "hi", "en", "hi-Latn"
    float confidence = 0.0F;
};

// A sub-span of a TranscriptSegment attributed to one language — represents code-switching.
struct LanguageSegment {
    TimeRange range;
    Language language;
};

// Raw PCM audio, mono, produced by IAudioSource and consumed by IVAD/ISpeechToTextEngine.
struct AudioFrame {
    std::vector<float> samples;
    std::uint32_t sampleRateHz = 0;
    Timestamp capturedAt;
};

}  // namespace meeting_sdk::core
