#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes returned by every msdk_* function. 0 always means success. This is the stable
 * ABI boundary between the C++ core and every native binding (JNI, Kotlin/Native cinterop,
 * Objective-C++) — see docs/architecture/01-requirements-and-architecture.md §2. No C++
 * exception, class, or STL type ever crosses this header. */
typedef enum {
    MSDK_OK = 0,
    MSDK_ERROR_INVALID_ARGUMENT = -1,
    MSDK_ERROR_TRANSLATION_FAILED = -2,
    MSDK_ERROR_NOT_FOUND = -3,
    MSDK_ERROR_STORAGE_FAILED = -4,
    MSDK_ERROR_PIPELINE_FAILED = -5,
} msdk_error_t;

/* --- Translation ---------------------------------------------------------------------------
 * On-device dictionary translation (see meeting_sdk::translation::DictionaryTranslator). Cloud
 * translation is not exposed here — enabling it is a host-app configuration decision made
 * through AIProviderConfig, not something this narrow demo surface should default to.
 */

/* Translates text to target_bcp47 ("en" or "hi"). On MSDK_OK, *out_result is a heap-allocated,
 * NUL-terminated string the caller must release with msdk_free_string(). Untouched on error. */
int32_t msdk_translate(const char* text, const char* target_bcp47, char** out_result);

void msdk_free_string(char* s);

/* --- Meeting repository ---------------------------------------------------------------------
 * Opaque handle to an open, encrypted-at-rest repository. Not thread-safe: callers serialize
 * access to one handle onto a single storage thread (see docs/architecture/
 * 02-interfaces-and-data-models.md §3.5).
 */
typedef struct msdk_repository msdk_repository;

/* db_path: a file path, or ":memory:" for an ephemeral database. */
int32_t msdk_repository_open(const char* db_path, msdk_repository** out_handle);
void msdk_repository_close(msdk_repository* handle);

/* Creates or replaces a single-segment meeting containing one TranscriptSegment with `text`.
 * A richer creation API (multi-segment, speaker-attributed) lands once real capture/STT
 * (Milestones 9-11 providers) can actually produce that data — until then this is the honest
 * surface: a text note stored as a meeting. */
int32_t msdk_repository_save_simple_meeting(msdk_repository* handle, const char* meeting_id,
                                             const char* text);

/* On MSDK_OK, *out_json is heap-allocated JSON the caller must release with msdk_free_string().
 * Shape: {"id":string,"state":string,
 *         "transcript":[{"id":string,"text":string,"speaker":string,"startMs":number,"endMs":number}],
 *         "summary":{"text":string,"keyPoints":[string]}|null,
 *         "actionItems":[{"action":string,"owner":string|null,"confidence":number}],
 *         "decisions":[{"text":string,"confidence":number}],
 *         "topics":[{"label":string}],
 *         "questions":[{"text":string,"resolved":boolean}]}
 * The intelligence fields (summary/actionItems/decisions/topics/questions) are only populated for
 * meetings produced by msdk_repository_run_demo_meeting(); msdk_repository_save_simple_meeting()
 * never runs extraction, so they come back empty/null for those. */
int32_t msdk_repository_get_meeting_json(msdk_repository* handle, const char* meeting_id,
                                          char** out_json);

/* *out_json is a JSON array of meeting id strings, e.g. ["m1","m2"]. */
int32_t msdk_repository_list_meeting_ids_json(msdk_repository* handle, char** out_json);

int32_t msdk_repository_remove_meeting(msdk_repository* handle, const char* meeting_id);

/* --- Meeting pipeline demo ------------------------------------------------------------------
 * Runs meeting_sdk::orchestration::MeetingPipeline end-to-end over a short, deterministic
 * synthetic utterance (audio::SyntheticAudioSource) using the same hand-written demo fakes the
 * SDK's own pipeline tests use for STT/diarization (see meeting-sdk/tests/orchestration/
 * test_meeting_pipeline.cpp) — no real on-device STT/diarizer model ships in this SDK yet
 * (Milestones 9-11 providers), so this is an honest fixture, not production transcription.
 * Language detection and meeting intelligence extraction are real (HeuristicLanguageDetector,
 * HeuristicLlmEngine). Persists the finished Meeting into `handle`; fetch it afterward with
 * msdk_repository_get_meeting_json(). */
int32_t msdk_repository_run_demo_meeting(msdk_repository* handle, const char* meeting_id);

/* --- Search -----------------------------------------------------------------------------------
 * Keyword search (meeting_sdk::search::InvertedIndexSearch) over every meeting currently saved
 * in `handle`. Builds the index fresh from persisted meetings on each call — this binding has no
 * standing index to keep in sync, which is fine at demo scale but not how a production host app
 * would wire it (see InvertedIndexSearch's own header for why this is real keyword search, not a
 * placeholder, and why it isn't semantic/cross-language).
 * On MSDK_OK, *out_json is a JSON array of {"meetingId":string,"segmentId":string,"score":number}
 * the caller must release with msdk_free_string(). */
int32_t msdk_repository_search_json(msdk_repository* handle, const char* query_text, char** out_json);

/* --- Live pipeline (real capture, externally-transcribed) ------------------------------------
 * Opaque handle to a meeting_sdk::orchestration::MeetingPipeline driven by
 * msdk_pipeline_ingest_utterance() instead of raw audio frames — for hosts whose STT source
 * captures and transcribes as one atomic step and can only hand back recognized text (e.g.
 * Android's on-device SpeechRecognizer). Internally uses audio::NullAudioSource (delivers no
 * frames; MeetingPipeline::start() still needs *an* IAudioSource to reach Recording) and the
 * SDK's real HeuristicLanguageDetector/HeuristicLlmEngine for language detection and meeting
 * intelligence — diarization is skipped for this path (ISpeakerDiarizer needs raw AudioFrames
 * this path never has; the caller supplies speaker_id directly instead, same reasoning as
 * MeetingPipeline::ingestTranscribedSegment's own header comment). Not thread-safe: same
 * single-thread contract as msdk_repository.
 */
typedef struct msdk_pipeline msdk_pipeline;

/* Creates a pipeline that will persist its finished Meeting into `repository` under `meeting_id`
 * once msdk_pipeline_stop() is called. Does not start capture — call msdk_pipeline_start() next. */
int32_t msdk_pipeline_create(msdk_repository* repository, const char* meeting_id,
                              msdk_pipeline** out_handle);

/* Idle -> Recording. */
int32_t msdk_pipeline_start(msdk_pipeline* handle);

/* Adds one already-transcribed utterance (e.g. one SpeechRecognizer onResults() call) to the
 * transcript. Requires Recording state (i.e. after msdk_pipeline_start(), before
 * msdk_pipeline_stop()). start_ms/end_ms are epoch milliseconds. No-op if text is empty. */
int32_t msdk_pipeline_ingest_utterance(msdk_pipeline* handle, const char* text, int64_t start_ms,
                                        int64_t end_ms, const char* speaker_id, float confidence);

/* Recording -> Processing -> Completed|Failed. Runs meeting intelligence extraction and persists
 * the finished Meeting into the repository passed to msdk_pipeline_create(). On MSDK_OK,
 * *out_meeting_json is heap-allocated JSON (same shape as msdk_repository_get_meeting_json) the
 * caller must release with msdk_free_string(). */
int32_t msdk_pipeline_stop(msdk_pipeline* handle, char** out_meeting_json);

/* Releases the handle. Safe to call whether or not msdk_pipeline_stop() was called first (an
 * unstopped pipeline is simply discarded without persisting). */
void msdk_pipeline_destroy(msdk_pipeline* handle);

/* --- Acoustic speaker clustering (real clustering, caller-supplied features) -----------------
 * Wraps meeting_sdk::speaker::SpeakerClusterer (online nearest-centroid clustering, already real
 * and unit-tested — see meeting-sdk/tests/speaker/test_speaker_clusterer.cpp) for hosts that can
 * compute *some* per-utterance acoustic feature vector themselves but have no neural voice
 * embedding model (none ships in this SDK). The clusterer doesn't care how the vector was
 * derived, only that acoustically-similar vectors land close together — so a host-supplied crude
 * feature vector (e.g. pitch + energy) still gets a real, working clustering algorithm behind it,
 * not a fabricated one. Not thread-safe: same single-thread contract as msdk_repository.
 */
typedef struct msdk_diarizer msdk_diarizer;

/* similarity_threshold: cosine similarity a new embedding must strictly exceed against an
 * existing speaker's centroid to join it, otherwise a new speaker is minted. 0.75 is
 * SpeakerClusterer's own default if unsure. */
int32_t msdk_diarizer_create(float similarity_threshold, msdk_diarizer** out_handle);

/* Assigns `embedding` (length embedding_length) to an existing speaker or mints a new one
 * ("speaker-1", "speaker-2", ...). On MSDK_OK, *out_speaker_id is heap-allocated, the caller must
 * release with msdk_free_string(). */
int32_t msdk_diarizer_assign(msdk_diarizer* handle, const float* embedding, int32_t embedding_length,
                              char** out_speaker_id);

void msdk_diarizer_destroy(msdk_diarizer* handle);

#ifdef __cplusplus
}
#endif
