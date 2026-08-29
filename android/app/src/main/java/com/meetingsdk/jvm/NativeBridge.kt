package com.meetingsdk.jvm

/**
 * Thin JNI bridge over bindings/kotlin/jni (itself a thin wrapper over bindings/c/meeting_sdk.h),
 * built for this app via externalNativeBuild pointing straight at
 * ../../../bindings/kotlin/jni/CMakeLists.txt — no separate native build definition.
 *
 * This lives under package com.meetingsdk.jvm (not com.meetingsdk.android) on purpose: the
 * compiled meeting_sdk_jni.so exports symbols named Java_com_meetingsdk_jvm_NativeBridge_* (see
 * bindings/kotlin/jni/src/meeting_sdk_jni.cpp) because kmp/src/jvmMain declares the same object
 * under the same fully-qualified name. Matching it here means zero JNI-side changes were needed
 * to reuse the existing bridge from Android. kmp/build.gradle.kts documents androidTarget() as
 * Milestone 9 scope; this file is this app's own stand-in until that lands, not a fork of it —
 * it is line-for-line the same external declarations as kmp's jvmMain/NativeBridge.kt.
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
    external fun repositoryRunDemoMeeting(handle: Long, meetingId: String): Boolean
    external fun repositorySearchJson(handle: Long, queryText: String): String?

    external fun pipelineCreate(repositoryHandle: Long, meetingId: String): Long
    external fun pipelineStart(handle: Long): Boolean
    external fun pipelineIngestUtterance(
        handle: Long,
        text: String,
        startMs: Long,
        endMs: Long,
        speakerId: String,
        confidence: Float,
    ): Boolean
    external fun pipelineStop(handle: Long): String?
    external fun pipelineDestroy(handle: Long)

    external fun diarizerCreate(similarityThreshold: Float): Long
    external fun diarizerAssign(handle: Long, embedding: FloatArray): String?
    external fun diarizerDestroy(handle: Long)
}
