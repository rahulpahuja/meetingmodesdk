package com.meetingsdk.jvm

/**
 * Thin JNI bridge over bindings/kotlin/jni (itself a thin wrapper over
 * bindings/c/meeting_sdk.h). No business logic lives here — every method is a direct 1:1 call
 * into native code, matching "JNI should be thin" (product spec §20).
 */
internal object NativeBridge {
    init {
        System.loadLibrary("meeting_sdk_jni")
    }

    external fun translate(text: String, targetBcp47: String): String?

    external fun repositoryOpen(dbPath: String): Long
    external fun repositoryClose(handle: Long)
    external fun repositorySaveSimpleMeeting(handle: Long, meetingId: String, text: String): Boolean
    external fun repositoryGetMeetingJson(handle: Long, meetingId: String): String?
    external fun repositoryListMeetingIdsJson(handle: Long): String?
    external fun repositoryRemoveMeeting(handle: Long, meetingId: String): Boolean
}
