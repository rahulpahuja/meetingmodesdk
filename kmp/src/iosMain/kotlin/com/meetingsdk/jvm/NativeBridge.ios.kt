package com.meetingsdk.jvm

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.toCPointer
import kotlinx.cinterop.toKString
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
// Opaque forward-declared structs land in the cinterop-synthesized `cnames.structs` package.
import cnames.structs.msdk_pipeline
import cnames.structs.msdk_repository
import meeting_sdk_c.msdk_free_string
import meeting_sdk_c.msdk_pipeline_create
import meeting_sdk_c.msdk_pipeline_destroy
import meeting_sdk_c.msdk_pipeline_ingest_utterance
import meeting_sdk_c.msdk_pipeline_start
import meeting_sdk_c.msdk_pipeline_stop
import meeting_sdk_c.msdk_repository_close
import meeting_sdk_c.msdk_repository_get_meeting_json
import meeting_sdk_c.msdk_repository_list_meeting_ids_json
import meeting_sdk_c.msdk_repository_open
import meeting_sdk_c.msdk_repository_remove_meeting
import meeting_sdk_c.msdk_repository_run_demo_meeting
import meeting_sdk_c.msdk_repository_save_simple_meeting
import meeting_sdk_c.msdk_repository_search_json
import meeting_sdk_c.msdk_translate

/**
 * Apple `actual`: every call is a direct cinterop entry point into bindings/c/meeting_sdk.h.
 * The only real work here is the C `char**` out-param + [msdk_free_string] ownership dance;
 * no business logic. `MSDK_OK` is 0, so `rc == 0` is success everywhere.
 */
@OptIn(ExperimentalForeignApi::class)
internal actual object NativeBridge {

    private inline fun outString(fill: (CPointer<CPointerVar<ByteVar>>) -> Int): String? = memScoped {
        val out = alloc<CPointerVar<ByteVar>>()
        if (fill(out.ptr) != 0) return@memScoped null
        val value = out.value ?: return@memScoped null
        val copied = value.toKString()
        msdk_free_string(value)
        copied
    }

    private fun repo(handle: Long): CPointer<msdk_repository>? = handle.toCPointer()
    private fun pipe(handle: Long): CPointer<msdk_pipeline>? = handle.toCPointer()

    actual fun translate(text: String, targetBcp47: String): String? =
        outString { msdk_translate(text, targetBcp47, it) }

    actual fun repositoryOpen(dbPath: String): Long = memScoped {
        val out = alloc<CPointerVar<msdk_repository>>()
        if (msdk_repository_open(dbPath, out.ptr) != 0) 0L else out.value.toLong()
    }

    actual fun repositoryClose(handle: Long) {
        msdk_repository_close(repo(handle))
    }

    actual fun repositorySaveSimpleMeeting(handle: Long, meetingId: String, text: String): Boolean =
        msdk_repository_save_simple_meeting(repo(handle), meetingId, text) == 0

    actual fun repositoryGetMeetingJson(handle: Long, meetingId: String): String? =
        outString { msdk_repository_get_meeting_json(repo(handle), meetingId, it) }

    actual fun repositoryListMeetingIdsJson(handle: Long): String? =
        outString { msdk_repository_list_meeting_ids_json(repo(handle), it) }

    actual fun repositoryRemoveMeeting(handle: Long, meetingId: String): Boolean =
        msdk_repository_remove_meeting(repo(handle), meetingId) == 0

    actual fun repositoryRunDemoMeeting(handle: Long, meetingId: String): Boolean =
        msdk_repository_run_demo_meeting(repo(handle), meetingId) == 0

    actual fun repositorySearchJson(handle: Long, queryText: String): String? =
        outString { msdk_repository_search_json(repo(handle), queryText, it) }

    actual fun pipelineCreate(repositoryHandle: Long, meetingId: String): Long = memScoped {
        val out = alloc<CPointerVar<msdk_pipeline>>()
        if (msdk_pipeline_create(repo(repositoryHandle), meetingId, out.ptr) != 0) 0L
        else out.value.toLong()
    }

    actual fun pipelineStart(handle: Long): Boolean = msdk_pipeline_start(pipe(handle)) == 0

    actual fun pipelineIngestUtterance(
        handle: Long,
        text: String,
        startMs: Long,
        endMs: Long,
        speakerId: String,
        confidence: Float,
    ): Boolean =
        msdk_pipeline_ingest_utterance(pipe(handle), text, startMs, endMs, speakerId, confidence) == 0

    actual fun pipelineStop(handle: Long): String? =
        outString { msdk_pipeline_stop(pipe(handle), it) }

    actual fun pipelineDestroy(handle: Long) {
        msdk_pipeline_destroy(pipe(handle))
    }
}
