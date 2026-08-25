# meeting-sdk

Privacy-first, on-device-first C++ core for the AI Meeting Mode SDK. Architecture docs live in
`../docs/architecture/`. This tree builds and tests standalone, with no Android/iOS toolchain.

## Prerequisites

- CMake 3.24+
- `libsodium` (encryption at rest — `brew install libsodium` on macOS)
- `sqlite3` — already present on macOS/Linux/Android/iOS; no install needed

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Options

| Option | Default | Purpose |
|---|---|---|
| `MEETING_SDK_BUILD_TESTS` | `ON` | Build the GoogleTest suite (fetched via `FetchContent`). |
| `MEETING_SDK_WARNINGS_AS_ERRORS` | `ON` | `-Werror` on first-party targets. |
| `MEETING_SDK_ENABLE_ASAN` | `OFF` | AddressSanitizer + UndefinedBehaviorSanitizer. |
| `MEETING_SDK_ENABLE_TSAN` | `OFF` | ThreadSanitizer (mutually exclusive with ASan). |

Example sanitizer build: `cmake -S . -B build-asan -DMEETING_SDK_ENABLE_ASAN=ON && cmake --build build-asan -j && ctest --test-dir build-asan`

## Layout (Milestones 1–7 scope)

- `core/` — domain models, error model (`Result<T>`), interfaces. Header-heavy, one small
  static library (`meeting_sdk::core`).
- `orchestration/` — `MeetingStateMachine` and `TranscriptAssembler` (orders/validates
  transcript segments arriving from concurrent STT sessions into `Meeting::transcript`), plus
  `MeetingPipeline`, the full pipeline composing `audio`, `speech`, `speaker`, `translation`,
  `intelligence`, and `storage` (`search` stays a separate consumer of the persisted `Meeting`,
  not wired into the pipeline itself) into one driven session: audio capture → preprocessing →
  VAD → segmentation → STT → diarization → speaker assignment → language detection → transcript
  assembly → meeting intelligence extraction → encrypted persistence, with `MeetingStateMachine`
  transitions driven at each stage. Every pluggable stage is injected as a `core::` interface
  (`core::ISpeechToTextEngine`, `core::ISpeakerDiarizer`, etc.) so it composes only against
  interfaces that already exist — no concrete STT/diarization engine ships yet (see `speech/` and
  `speaker/` below), so `MeetingPipeline` depends on `core::ISpeechToTextEngine` and
  `core::ISpeakerDiarizer` directly rather than on any concrete provider.
- `audio/` — `RingBuffer<T>` (lock-free SPSC), `Preprocessor` (DC-offset removal + downward
  peak normalization), `EnergyVad` (the default on-device `core::IVAD`), and
  `SyntheticAudioSource` (a deterministic `core::IAudioSource` fixture for tests/demos — real
  hardware capture backends land in Milestones 9–10).
- `speech/` — `Segmenter` (VAD decisions → `SpeechSegment`s), `StreamingSttSession` (enforces
  correct start/pushAudio/finish/cancel sequencing over any `core::ISpeechToTextEngine`), and
  `HeuristicLanguageDetector` (deterministic word-level Hindi/English + code-switching
  detector — a real on-device baseline, not a statistical language-ID model). No concrete
  `ISpeechToTextEngine` ships yet: real ASR requires bundled model weights and a model
  runtime, which belong to `providers/on_device` in a later milestone — this SDK does not fake
  one and present it as production STT.
- `speaker/` — `cosineSimilarity` (shared by the two below), `SpeakerClusterer` (online
  nearest-centroid clustering over voice embeddings — the clustering half of diarization),
  `EnrolledSpeakerIdentifier` (nearest-neighbor match against an enrolled voiceprint registry,
  deliberately separate from clustering per the diarization-vs-identification distinction in
  the architecture doc), and `SpeakerAssigner` (assigns each `TranscriptSegment` the speaker of
  its best-overlapping diarized time span). No concrete `ISpeakerDiarizer` ships yet — same
  reasoning as STT: real diarization needs a neural voice-embedding extractor, which belongs to
  `providers/on_device` once real model weights are in scope.
- `intelligence/` — a real, complete `core::ILLMEngine` implementation
  (`HeuristicLlmEngine`), unlike the STT/diarization gaps above: text-based extraction *is*
  achievable without a model. Composes five small, independently-testable pieces:
  `HeuristicQuestionExtractor` (question-mark detection), `HeuristicDecisionExtractor` and
  `HeuristicActionItemExtractor` (closed-set phrase markers), `KeywordTopicExtractor` and
  `ExtractiveSummarizer` (share `WordFrequencyAnalyzer` for stopword-filtered word-frequency
  ranking). All deterministic, on-device, and explicitly documented as a baseline — swappable
  in full for an LLM-backed `providers/cloud` or `providers/on_device` engine with no
  orchestration-layer change. Action-item owner/deadline extraction is left `nullopt` (needs
  named-entity/date parsing this heuristic doesn't attempt); question resolution-tracking
  always defaults to `false` (needs cross-segment reasoning a single pass can't do).
- `translation/` — `GatedTranslator` (enforces `core::ProviderMode` before any call reaches a
  translator — "cloud translation must never silently activate" is structural here, not a
  convention: `Disabled` rejects before touching the wrapped translator at all) and
  `DictionaryTranslator` (a real, if crude, word-substitution `ITranslator` for a small
  Hindi↔English vocabulary — an honest ON_DEVICE default until a real NMT model is bundled).
  Multilingual embeddings and cross-language search are **not** built yet: both need a real
  multilingual sentence-embedding model, which doesn't exist in this environment — same
  reasoning as STT/diarization. They're deferred to Milestone 7 rather than faked; language
  metadata and code-switching representation (the rest of this milestone's scope) were already
  built for real reasons in Milestones 1 and 3.
- `storage/` — real encrypted persistence, not a stub: `SodiumEncryptor` (libsodium
  `crypto_secretbox`, authenticated XSalsa20-Poly1305), `InMemoryKeyProvider` (a dev/test
  `IKeyProvider` — real per-meeting key generation via a CSPRNG, but explicitly **not**
  hardware-backed; a real deployment's `IKeyProvider` wraps keys via Android Keystore / iOS
  Secure Enclave in a Milestone 9/10 platform bridge), `MeetingSerializer` (hand-rolled
  length-prefixed binary codec — avoids a JSON dependency and escaping bugs), and
  `SqliteMeetingRepository` (only ciphertext ever touches disk; `remove()` deletes the DEK
  before the row, so crypto-erase holds even if row deletion later fails). Uses the system
  `sqlite3` (already present on macOS/Linux/Android/iOS) and `libsodium` (installed via
  Homebrew here — document this as a required third-party dependency per product spec §28).
- `search/` — `InvertedIndexSearch`: a real, working keyword search with meeting/speaker/
  date-range filtering, ranked by term-match count. Semantic (embedding-based) and
  cross-language search need a real multilingual sentence-embedding model, the same
  "no model available in this environment" constraint that shaped the STT/diarization/
  translation gaps in earlier milestones — deferred rather than faked with meaningless vectors,
  which would silently produce garbage results.
- `tests/` — GoogleTest suite; `ctest` discovers each `TEST(...)` individually. Hand-written
  fakes for interfaces like `ISpeechToTextEngine` live in the test files that need them, per
  `docs/architecture/04-testing-and-performance-strategy.md` §1 — they are not shipped SDK code.
  `tests/support/` holds small shared test-only helpers (e.g. `time_helpers.hpp`).

Not yet present: `providers/`. Scoped to later milestones per
`../docs/architecture/04-testing-and-performance-strategy.md` §4 — they are intentionally absent
rather than stubbed, per the "no placeholder implementations disguised as production code" rule.
