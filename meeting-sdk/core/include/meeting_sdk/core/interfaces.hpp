#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/core/errors.hpp"
#include "meeting_sdk/core/types.hpp"

namespace meeting_sdk::core {

// --- Audio -------------------------------------------------------------------------------
// Callback fires on an internal capture/processing thread; the caller must not block in it.
class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual Result<void> start(std::function<void(AudioFrame)> onFrame) = 0;
    virtual Result<void> stop() = 0;
};

enum class VadDecision { Silence, Speech };

class IVAD {
public:
    virtual ~IVAD() = default;
    // Lookahead/hangover thresholds are constructor-configured on the implementation, not
    // per-call, so the segmenter downstream can rely on a stable boundary policy per session.
    virtual Result<VadDecision> process(const AudioFrame& frame) = 0;
};

// --- Speech --------------------------------------------------------------------------------

struct TranscriptionOptions {
    std::optional<std::string> languageHint;  // BCP-47; unset = auto-detect
    bool enablePartialResults = true;
};

struct TranscriptionSessionHandle {
    std::uint64_t value = 0;
};

inline bool operator==(const TranscriptionSessionHandle& a, const TranscriptionSessionHandle& b) {
    return a.value == b.value;
}

struct PartialResult {
    TranscriptionSessionHandle session;
    std::string text;
    Language language;
};

struct FinalResult {
    TranscriptionSessionHandle session;
    TranscriptSegment segment;
};

// One interface serves both streaming and batch transcription — batch is streaming with a
// single final chunk, avoiding two interfaces with duplicated result/error/language semantics.
class ISpeechToTextEngine {
public:
    virtual ~ISpeechToTextEngine() = default;
    // Called on the pipeline's processing thread. onPartial/onFinal fire on an internal
    // inference thread — the caller marshals to its own thread if needed.
    virtual Result<TranscriptionSessionHandle> start(TranscriptionOptions options,
                                                       std::function<void(PartialResult)> onPartial,
                                                       std::function<void(FinalResult)> onFinal) = 0;
    virtual Result<void> pushAudio(TranscriptionSessionHandle session, const AudioFrame& frame) = 0;
    virtual Result<void> finish(TranscriptionSessionHandle session) = 0;  // triggers final onFinal
    virtual Result<void> cancel(TranscriptionSessionHandle session) = 0;  // no further callbacks fire
};

class ILanguageDetector {
public:
    virtual ~ILanguageDetector() = default;
    virtual Result<std::vector<LanguageSegment>> detect(const TranscriptSegment& segment) = 0;
};

// --- Speaker -------------------------------------------------------------------------------
// Diarization ("which spans belong to the same unidentified speaker") and identification
// ("which known person is this") are different problems with different data requirements —
// identification needs an enrolled voiceprint store, diarization needs none. Conflating them
// would force every diarization-only integration to also implement identification.
class ISpeakerDiarizer {
public:
    virtual ~ISpeakerDiarizer() = default;
    virtual Result<std::vector<SpeakerId>> diarize(const std::vector<AudioFrame>& segment) = 0;
};

class ISpeakerIdentifier {
public:
    virtual ~ISpeakerIdentifier() = default;
    virtual Result<std::optional<std::string>> identify(const SpeakerEmbedding& embedding) = 0;
};

// --- Translation / embeddings / intelligence ------------------------------------------------
// Cloud mode must be explicitly configured via AIProviderConfig::translation; never
// auto-upgraded from OnDevice/Disabled at runtime.
class ITranslator {
public:
    virtual ~ITranslator() = default;
    virtual Result<std::string> translate(const std::string& text, const std::string& targetBcp47) = 0;
};

class IEmbeddingEngine {
public:
    virtual ~IEmbeddingEngine() = default;
    virtual Result<std::vector<float>> embed(const std::string& text) = 0;
};

class ILLMEngine {
public:
    virtual ~ILLMEngine() = default;
    virtual Result<Summary> summarize(const std::vector<TranscriptSegment>& transcript) = 0;
    virtual Result<std::vector<ActionItem>> extractActionItems(
        const std::vector<TranscriptSegment>& transcript) = 0;
    virtual Result<std::vector<Decision>> extractDecisions(
        const std::vector<TranscriptSegment>& transcript) = 0;
    virtual Result<std::vector<Topic>> extractTopics(const std::vector<TranscriptSegment>& transcript) = 0;
    virtual Result<std::vector<Question>> extractQuestions(
        const std::vector<TranscriptSegment>& transcript) = 0;
};

// --- Storage -------------------------------------------------------------------------------
// Returns/accepts only domain models — no SQLite/Room/Core Data type ever appears here.
class IMeetingRepository {
public:
    virtual ~IMeetingRepository() = default;
    virtual Result<void> save(const Meeting& meeting) = 0;
    virtual Result<Meeting> get(const MeetingId& id) = 0;
    virtual Result<void> remove(const MeetingId& id) = 0;  // must crypto-erase, see threat model §5
    virtual Result<std::vector<MeetingId>> listAll() = 0;
};

// --- Cross-cutting ---------------------------------------------------------------------------
class IClock {
public:
    virtual ~IClock() = default;
    virtual Timestamp now() = 0;
};

enum class LogLevel { Error, Warn, Info, Debug, Trace };

// value must never be transcript/audio content — see threat model §3(c).
struct LogField {
    std::string key;
    std::string value;
};

// Structurally prevents logging raw meeting content: takes typed, pre-redacted fields, never
// a free-text blob that could accidentally carry transcript/audio content.
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, std::string_view event, std::span<const LogField> fields) = 0;
};

}  // namespace meeting_sdk::core
