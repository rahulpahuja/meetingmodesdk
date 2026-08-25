# Core Interfaces & Data Models

See `01-requirements-and-architecture.md` for module boundaries and pattern rationale;
`03-threat-model-and-security.md` for why the logger/error shapes below are what they are.

## 1. Error model

**Decision**: domain and interface boundaries use a `Result<T>` (`std::expected<T, Error>`-style)
return type, not exceptions. Exceptions crossing the JNI/Obj-C++ boundary are expensive to
translate correctly (JNI has no native exception propagation; Obj-C++ mixing C++ exceptions with
Objective-C exceptions is a known footgun) and make cancellation/error paths in a streaming
pipeline hard to reason about locally. Exceptions remain acceptable *within* a single module for
truly exceptional conditions (e.g. `std::bad_alloc`), but never as part of a public interface
contract.

```cpp
enum class ErrorCategory {
    Audio, Transcription, Model, Storage, Network,
    Permission, Configuration, Security, Cancellation
};

struct Error {
    ErrorCategory category;
    std::string code;          // stable, machine-readable, e.g. "audio.device_unavailable"
    std::string message;       // developer-facing, never raw meeting content
    std::shared_ptr<Error> cause = nullptr; // optional chain, for diagnostics
};

template <typename T>
using Result = std::expected<T, Error>;
```

Vendor-specific error codes are translated into this taxonomy at the `providers/*` boundary and
never escape it — `core`, `orchestration`, and every layer above see only `Error`.

## 2. Core domain models (`core/domain`)

Strong types instead of raw primitives to prevent parameter-order bugs at construction sites:

```cpp
struct MeetingId    { std::string value; };
struct SpeakerId    { std::string value; };
struct SegmentId    { std::string value; };
struct Timestamp    { std::chrono::system_clock::time_point value; };   // absolute
struct Duration_    { std::chrono::milliseconds value; };               // relative

struct TimeRange { Timestamp start; Timestamp end; };                   // immutable value type

struct Language {
    std::string bcp47Code;     // e.g. "hi", "en", "hi-Latn"
    float confidence;          // [0,1]
};

// A sub-span of a segment attributed to one language — enables code-switching representation.
struct LanguageSegment {
    TimeRange range;
    Language language;
};

struct SpeakerEmbedding {
    SpeakerId speaker;
    std::vector<float> vector;
    Timestamp capturedAt;
};

// Mutable aggregate: identity persists as a speaker is re-clustered/renamed by the user.
struct Speaker {
    SpeakerId id;
    std::optional<std::string> displayName;  // null until identified/labeled
    std::vector<SpeakerEmbedding> embeddings;
};

// Immutable value object once assembled; corrections create a new segment, they don't mutate.
struct TranscriptSegment {
    SegmentId id;
    TimeRange range;
    SpeakerId speaker;
    std::string text;
    Language detectedLanguage;             // dominant language for the segment
    float confidence;
    std::vector<LanguageSegment> languageSegments; // empty if monolingual
};

struct ActionItem {
    std::string action;
    std::optional<std::string> owner;
    std::optional<Timestamp> deadline;
    SegmentId sourceSegment;
    float confidence;
};

struct Decision {
    std::string text;
    SegmentId sourceSegment;
    float confidence;
};

struct Topic {
    std::string label;
    std::vector<SegmentId> relatedSegments;
};

struct Question {
    std::string text;
    SegmentId sourceSegment;
    bool resolved;
};

struct Summary {
    std::string text;
    std::vector<std::string> keyPoints;
};

// State pattern: transitions validated centrally, not scattered across call sites.
enum class MeetingState { Idle, Recording, Paused, Processing, Completed, Failed };

struct Meeting {
    MeetingId id;
    MeetingState state;                    // immutable snapshot; engine holds the live FSM
    TimeRange range;
    std::vector<Speaker> speakers;
    std::vector<TranscriptSegment> transcript;
    std::optional<Summary> summary;
    std::vector<ActionItem> actionItems;
    std::vector<Decision> decisions;
    std::vector<Topic> topics;
    std::vector<Question> questions;
};

enum class ProviderMode { OnDevice, Cloud, Disabled };

// One independently-configurable slot per provider family — see threat model §6.
struct AIProviderConfig {
    ProviderMode stt;
    ProviderMode llm;
    ProviderMode translation;
    ProviderMode embeddings;
    ProviderMode diarization;
};
```

## 3. Core interfaces (`core/interfaces`)

All pure virtual, all constructor-injected — no factory reaches into a global registry.

```cpp
// --- Audio -------------------------------------------------------------
// Callback fires on an internal capture/processing thread; caller must not block in it.
class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual Result<void> start(std::function<void(AudioFrame)> onFrame) = 0;
    virtual Result<void> stop() = 0;
};

// Decision: streaming push, not pull — capture hardware drives timing, not the consumer.

class IVAD {
public:
    virtual ~IVAD() = default;
    // lookahead/hangover are constructor-configured on the implementation, not per-call,
    // so the segmenter downstream can rely on a stable boundary policy for the session.
    virtual Result<VadDecision> process(const AudioFrame& frame) = 0;
};

// --- Speech --------------------------------------------------------------
// Decision: one interface serves both streaming and batch — batch is streaming with a single
// final chunk. This avoids two interfaces with duplicated result/error/language semantics.
class ISpeechToTextEngine {
public:
    virtual ~ISpeechToTextEngine() = default;
    // Called on the pipeline's processing thread. Callbacks fire on an internal inference
    // thread — caller marshals to its own thread if needed.
    virtual Result<TranscriptionSessionHandle> start(
        TranscriptionOptions options,
        std::function<void(PartialResult)> onPartial,
        std::function<void(FinalResult)> onFinal) = 0;
    virtual Result<void> pushAudio(TranscriptionSessionHandle, const AudioFrame&) = 0;
    virtual Result<void> finish(TranscriptionSessionHandle) = 0;   // triggers final onFinal
    virtual Result<void> cancel(TranscriptionSessionHandle) = 0;   // no further callbacks fire
};

class ILanguageDetector {
public:
    virtual ~ILanguageDetector() = default;
    virtual Result<std::vector<LanguageSegment>> detect(const TranscriptSegment&) = 0;
};

// --- Speaker ---------------------------------------------------------------
// Decision: diarization ("which spans belong to the same unidentified speaker") and
// identification ("which known person is this") are different problems with different
// failure modes and different data requirements (identification needs an enrolled voiceprint
// store; diarization needs none) — conflating them would force every diarization-only
// integration to also implement identification.
class ISpeakerDiarizer {
public:
    virtual ~ISpeakerDiarizer() = default;
    virtual Result<std::vector<SpeakerId>> diarize(
        const std::vector<AudioFrame>& segment) = 0;    // incremental, streaming-friendly
};

class ISpeakerIdentifier {
public:
    virtual ~ISpeakerIdentifier() = default;
    virtual Result<std::optional<std::string>> identify(const SpeakerEmbedding&) = 0;
};

// --- Translation -------------------------------------------------------------
// Cloud mode must be explicitly configured per AIProviderConfig.translation; never
// auto-upgraded from OnDevice/Disabled at runtime.
class ITranslator {
public:
    virtual ~ITranslator() = default;
    virtual Result<std::string> translate(
        const std::string& text, const std::string& targetBcp47) = 0;
};

class IEmbeddingEngine {
public:
    virtual ~IEmbeddingEngine() = default;
    virtual Result<std::vector<float>> embed(const std::string& text) = 0;
};

// --- Intelligence -------------------------------------------------------------
class ILLMEngine {
public:
    virtual ~ILLMEngine() = default;
    virtual Result<Summary> summarize(const std::vector<TranscriptSegment>&) = 0;
    virtual Result<std::vector<ActionItem>> extractActionItems(const std::vector<TranscriptSegment>&) = 0;
    virtual Result<std::vector<Decision>> extractDecisions(const std::vector<TranscriptSegment>&) = 0;
    virtual Result<std::vector<Topic>> extractTopics(const std::vector<TranscriptSegment>&) = 0;
    virtual Result<std::vector<Question>> extractQuestions(const std::vector<TranscriptSegment>&) = 0;
};

// --- Storage -------------------------------------------------------------
// Returns/accepts only domain models — no SQLite/Room/Core Data type ever appears here.
class IMeetingRepository {
public:
    virtual ~IMeetingRepository() = default;
    virtual Result<void> save(const Meeting&) = 0;
    virtual Result<Meeting> get(const MeetingId&) = 0;
    virtual Result<void> remove(const MeetingId&) = 0;   // must crypto-erase, see threat model
    virtual Result<std::vector<MeetingId>> listAll() = 0;
};

// --- Cross-cutting -------------------------------------------------------------
class IClock {
public:
    virtual ~IClock() = default;
    virtual Timestamp now() = 0;
};

// Structurally prevents logging raw meeting content: takes typed, pre-redacted fields,
// never a free-text blob that could accidentally contain transcript/audio content.
enum class LogLevel { Error, Warn, Info, Debug, Trace };
struct LogField { std::string key; std::string value; };  // value must never be transcript text
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel, std::string_view event, std::span<const LogField> fields) = 0;
};
```

## 3.5 Threading contract

- **Audio / VAD**: `IAudioSource::start` is called once from `orchestration` on its own thread;
  the frame callback fires on an internal realtime capture thread and must never block (no I/O, no
  locks that can be held by a slower thread) — see `05-diagrams.md` §4. `IVAD::process` is called
  synchronously from the processing thread pool that drains the ring buffer, not from the realtime
  capture thread.
- **STT**: `start`/`pushAudio`/`finish`/`cancel` are called from the orchestration processing
  thread; `onPartial`/`onFinal` fire on an internal inference thread owned by the STT
  implementation. `orchestration` is responsible for marshaling results to whatever thread the
  platform bridge (JNI/Obj-C++) needs them on before they cross into Kotlin/Swift — the C++ core
  never assumes the caller's threading model.
- **Storage**: `IMeetingRepository` calls are synchronous and blocking from the caller's
  perspective; `orchestration` invokes them from a dedicated storage/IO thread, never from the
  audio or inference threads, so a slow disk/encryption operation cannot stall capture or
  transcription.

## 4. See also

- `01-requirements-and-architecture.md` — module boundaries, dependency rule, pattern rationale.
- `03-threat-model-and-security.md` — why `ILogger` and error handling are shaped this way.
- `04-testing-and-performance-strategy.md` — how these interfaces get faked in tests.
