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
 *         "transcript":[{"id":string,"text":string,"speaker":string,"startMs":number,"endMs":number}]}
 */
int32_t msdk_repository_get_meeting_json(msdk_repository* handle, const char* meeting_id,
                                          char** out_json);

/* *out_json is a JSON array of meeting id strings, e.g. ["m1","m2"]. */
int32_t msdk_repository_list_meeting_ids_json(msdk_repository* handle, char** out_json);

int32_t msdk_repository_remove_meeting(msdk_repository* handle, const char* meeting_id);

#ifdef __cplusplus
}
#endif
