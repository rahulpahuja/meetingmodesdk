# Testing & Performance Strategy

## 1. Testability design

Every pipeline stage and every `orchestration` component takes its dependencies as
constructor-injected `core/interfaces` references — no service locator, no globals, per
`01-requirements-and-architecture.md` §4's Dependency Injection decision. This means every domain
component (VAD, segmenter, diarizer, LLM-backed extractors, repositories) can be unit-tested by
substituting a hand-written deterministic fake for its interface.

**Fake vs. mock**: the domain layer uses hand-written fakes (a `FakeSpeechToTextEngine` that
returns scripted results, a `FakeClock` with a settable time), not a mocking framework. A fake is
cheaper to reason about, doesn't couple tests to call-order/argument-matcher syntax, and avoids
adding a C++ mocking dependency to the whole core SDK just to unit-test domain logic. GoogleTest +
GoogleMock is acceptable — and appropriate — at true integration boundaries (e.g. verifying the
JNI bridge calls the C++ facade with correctly marshaled arguments), where interaction verification
genuinely matters more than behavior verification.

## 2. Test pyramid

| Level | Scope | Approach |
|---|---|---|
| **Unit** | VAD threshold/hangover logic, diarization clustering over synthetic embedding vectors, language-segment merge logic, action-item confidence scoring, `Error`/`Result` construction and propagation, `MeetingState` transition table. | Fakes only; no real audio, no real models; runs on any CI machine in milliseconds. |
| **Integration** | The six pipelines: Audio→VAD→STT, STT→Diarization, STT→LanguageDetection, Transcript→Intelligence, Transcript→Embeddings→Search, Meeting→Storage→Retrieval. | Small (few-second) fixture audio clips checked into the repo, run against lightweight on-device models or deterministic stub providers — not production-size models, to keep CI fast. |
| **End-to-end** | Full meeting processing over a handful of scripted fixture recordings: English, Hindi, Hindi-English code-switched, multi-speaker, silence-heavy. | Assert *structural/shape* properties (segment count > 0, every segment has a speaker and language, summary is non-empty, known keywords appear) — never exact model output, since inference isn't bit-exact across model versions/hardware. |
| **Platform** | JNI marshaling (primitive/struct conversion, exception-to-`Error`-code translation, thread handoff correctness), Obj-C++ bridge equivalents, KMP `commonTest` against fakes so the same Kotlin test source runs on both `androidTest` and `iosTest`. | Minimal Android instrumentation harness; XCTest for iOS; `commonTest` for KMP. |
| **Failure** | No microphone, permission denied, model unavailable, corrupt audio, network unavailable, storage failure, cancellation mid-pipeline, low memory, interrupted recording. | Each maps to a specific `ErrorCategory`/code from `02-interfaces-and-data-models.md` §1 (e.g. "no microphone" → `Audio` category, `"audio.device_unavailable"`); test asserts the specific typed error, not just "it doesn't crash." |
| **Multilingual** | Pure English, pure Hindi, Hindi-English code-switched single utterance, speaker-switching between languages mid-meeting, translation round-trip (translate then back, semantic similarity check not exact-match), cross-language semantic search (English query retrieves a Hindi-only segment). | Fixture-driven, same infra as integration/e2e tiers. |

## 3. Performance strategy

Initial targets — to be validated empirically once real models are integrated, not treated as
guarantees at this architecture stage:

| Metric | Initial target |
|---|---|
| VAD decision latency | < 50 ms per frame |
| STT streaming partial-result latency | < 500 ms from speech onset to first partial |
| End-to-end pipeline latency | < 1x real-time per minute of audio processed (i.e. keeps pace with a live meeting) |
| Resident model memory ceiling | < 500 MB on a mid-tier phone with the on-device model set loaded |
| Cold start / model load time | < 3 s before recording can begin |
| Storage footprint | < 50 MB per meeting-hour (compressed audio + transcript + embeddings, encrypted) |

**Methodology**: instrumented per-stage timers exposed through the `ILogger` observability path
(structured fields, e.g. `stage=vad, latency_ms=12` — never transcript content), not ad-hoc
profiling scattered through the codebase. **Profile before optimizing** — these targets exist to
catch regressions and guide provider selection, not to justify premature optimization. The
Strategy-pattern provider abstraction (`01-requirements-and-architecture.md` §4) is what keeps this
possible: swapping a slow on-device STT model for a faster one is a wiring change in
`orchestration`, never a rewrite of pipeline or domain code.

## 4. Milestone plan critique

The section-32 milestone order (0 Architecture → 1 Core SDK → 2 Audio → 3 Speech → 4 Speaker → 5
Intelligence → 6 Multilingual → 7 Storage/Search → 8 KMP → 9 Android → 10 iOS → 11 Hardening) is
endorsed with one adjustment: **pull multilingual language-metadata support into Milestone 3
(Speech), not Milestone 6.** Retrofitting `LanguageSegment`/code-switching representation onto an
already-built transcript-assembly module (built in Milestone 3, then reworked in Milestone 6) is
wasted rework — the `TranscriptSegment.languageSegments` field is part of the core data model from
day one (see `02-interfaces-and-data-models.md`), so segmentation and STT integration should be
built against it from the start. Milestone 6 then becomes "multilingual providers, translation, and
multilingual embeddings" — genuinely new capability — rather than "go back and fix the transcript
model." Everything else in the order is sound: storage/search after intelligence makes sense
because search indexes intelligence output (summaries/topics), and KMP/Android/iOS correctly come
last since they're thin bridges over an already-proven core.

## 5. See also

- `01-requirements-and-architecture.md` — DI approach, module boundaries.
- `02-interfaces-and-data-models.md` — interfaces being faked, error taxonomy used by failure tests.
- `05-diagrams.md` — pipeline diagram referenced by the integration/e2e test scope.
