package com.meetingsdk.jvm

/**
 * The single thin seam onto native code. Each platform's `actual` is a 1:1 pass-through — JVM
 * via JNI (bindings/kotlin/jni), Apple via Kotlin/Native cinterop (bindings/c) — with no
 * business logic, matching "the binding layer should be thin" (product spec §20). Everything
 * above this object is platform-neutral and lives in commonMain.
 */
internal expect object NativeBridge {
    fun translate(text: String, targetBcp47: String): String?

    fun repositoryOpen(dbPath: String): Long
    fun repositoryClose(handle: Long)
    fun repositorySaveSimpleMeeting(handle: Long, meetingId: String, text: String): Boolean
    fun repositoryGetMeetingJson(handle: Long, meetingId: String): String?
    fun repositoryListMeetingIdsJson(handle: Long): String?
    fun repositoryRemoveMeeting(handle: Long, meetingId: String): Boolean
    fun repositoryRunDemoMeeting(handle: Long, meetingId: String): Boolean
    fun repositorySearchJson(handle: Long, queryText: String): String?

    fun pipelineCreate(repositoryHandle: Long, meetingId: String): Long
    fun pipelineStart(handle: Long): Boolean
    fun pipelineIngestUtterance(
        handle: Long,
        text: String,
        startMs: Long,
        endMs: Long,
        speakerId: String,
        confidence: Float,
    ): Boolean
    fun pipelineStop(handle: Long): String?
    fun pipelineDestroy(handle: Long)
}
