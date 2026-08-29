package com.meetingsdk.jvm

/**
 * JVM `actual`: every call is a direct `external` JNI entry point into bindings/kotlin/jni
 * (itself a thin wrapper over bindings/c/meeting_sdk.h). No logic here.
 */
internal actual object NativeBridge {
    init {
        System.loadLibrary("meeting_sdk_jni")
    }

    actual external fun translate(text: String, targetBcp47: String): String?

    actual external fun repositoryOpen(dbPath: String): Long
    actual external fun repositoryClose(handle: Long)
    actual external fun repositorySaveSimpleMeeting(handle: Long, meetingId: String, text: String): Boolean
    actual external fun repositoryGetMeetingJson(handle: Long, meetingId: String): String?
    actual external fun repositoryListMeetingIdsJson(handle: Long): String?
    actual external fun repositoryRemoveMeeting(handle: Long, meetingId: String): Boolean
    actual external fun repositoryRunDemoMeeting(handle: Long, meetingId: String): Boolean
    actual external fun repositorySearchJson(handle: Long, queryText: String): String?

    actual external fun pipelineCreate(repositoryHandle: Long, meetingId: String): Long
    actual external fun pipelineStart(handle: Long): Boolean
    actual external fun pipelineIngestUtterance(
        handle: Long,
        text: String,
        startMs: Long,
        endMs: Long,
        speakerId: String,
        confidence: Float,
    ): Boolean
    actual external fun pipelineStop(handle: Long): String?
    actual external fun pipelineDestroy(handle: Long)
}
