// Thin JNI bridge over bindings/c/meeting_sdk.h — no business logic lives here, only argument
// marshaling, matching "JNI should be thin" (product spec §20). Every function here is a
// direct 1:1 call into the stable C API.
#include <jni.h>

#include "meeting_sdk_c/meeting_sdk.h"

extern "C" {

JNIEXPORT jstring JNICALL Java_com_meetingsdk_jvm_NativeBridge_translate(JNIEnv* env, jobject /*thiz*/,
                                                                          jstring text,
                                                                          jstring targetBcp47) {
    const char* textChars = env->GetStringUTFChars(text, nullptr);
    const char* targetChars = env->GetStringUTFChars(targetBcp47, nullptr);

    char* result = nullptr;
    const int32_t status = msdk_translate(textChars, targetChars, &result);

    env->ReleaseStringUTFChars(text, textChars);
    env->ReleaseStringUTFChars(targetBcp47, targetChars);

    if (status != MSDK_OK) {
        return nullptr;
    }
    jstring jresult = env->NewStringUTF(result);
    msdk_free_string(result);
    return jresult;
}

JNIEXPORT jlong JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositoryOpen(JNIEnv* env, jobject /*thiz*/,
                                                                             jstring dbPath) {
    const char* pathChars = env->GetStringUTFChars(dbPath, nullptr);
    msdk_repository* handle = nullptr;
    const int32_t status = msdk_repository_open(pathChars, &handle);
    env->ReleaseStringUTFChars(dbPath, pathChars);
    return status == MSDK_OK ? reinterpret_cast<jlong>(handle) : 0;
}

JNIEXPORT void JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositoryClose(JNIEnv* /*env*/,
                                                                             jobject /*thiz*/, jlong handle) {
    msdk_repository_close(reinterpret_cast<msdk_repository*>(handle));
}

JNIEXPORT jboolean JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositorySaveSimpleMeeting(
    JNIEnv* env, jobject /*thiz*/, jlong handle, jstring meetingId, jstring text) {
    const char* idChars = env->GetStringUTFChars(meetingId, nullptr);
    const char* textChars = env->GetStringUTFChars(text, nullptr);
    const int32_t status = msdk_repository_save_simple_meeting(reinterpret_cast<msdk_repository*>(handle),
                                                                 idChars, textChars);
    env->ReleaseStringUTFChars(meetingId, idChars);
    env->ReleaseStringUTFChars(text, textChars);
    return status == MSDK_OK ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositoryGetMeetingJson(
    JNIEnv* env, jobject /*thiz*/, jlong handle, jstring meetingId) {
    const char* idChars = env->GetStringUTFChars(meetingId, nullptr);
    char* json = nullptr;
    const int32_t status =
        msdk_repository_get_meeting_json(reinterpret_cast<msdk_repository*>(handle), idChars, &json);
    env->ReleaseStringUTFChars(meetingId, idChars);
    if (status != MSDK_OK) {
        return nullptr;
    }
    jstring result = env->NewStringUTF(json);
    msdk_free_string(json);
    return result;
}

JNIEXPORT jstring JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositoryListMeetingIdsJson(
    JNIEnv* env, jobject /*thiz*/, jlong handle) {
    char* json = nullptr;
    const int32_t status =
        msdk_repository_list_meeting_ids_json(reinterpret_cast<msdk_repository*>(handle), &json);
    if (status != MSDK_OK) {
        return nullptr;
    }
    jstring result = env->NewStringUTF(json);
    msdk_free_string(json);
    return result;
}

JNIEXPORT jboolean JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositoryRemoveMeeting(
    JNIEnv* env, jobject /*thiz*/, jlong handle, jstring meetingId) {
    const char* idChars = env->GetStringUTFChars(meetingId, nullptr);
    const int32_t status =
        msdk_repository_remove_meeting(reinterpret_cast<msdk_repository*>(handle), idChars);
    env->ReleaseStringUTFChars(meetingId, idChars);
    return status == MSDK_OK ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositoryRunDemoMeeting(
    JNIEnv* env, jobject /*thiz*/, jlong handle, jstring meetingId) {
    const char* idChars = env->GetStringUTFChars(meetingId, nullptr);
    const int32_t status =
        msdk_repository_run_demo_meeting(reinterpret_cast<msdk_repository*>(handle), idChars);
    env->ReleaseStringUTFChars(meetingId, idChars);
    return status == MSDK_OK ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_com_meetingsdk_jvm_NativeBridge_repositorySearchJson(
    JNIEnv* env, jobject /*thiz*/, jlong handle, jstring queryText) {
    const char* queryChars = env->GetStringUTFChars(queryText, nullptr);
    char* json = nullptr;
    const int32_t status =
        msdk_repository_search_json(reinterpret_cast<msdk_repository*>(handle), queryChars, &json);
    env->ReleaseStringUTFChars(queryText, queryChars);
    if (status != MSDK_OK) {
        return nullptr;
    }
    jstring result = env->NewStringUTF(json);
    msdk_free_string(json);
    return result;
}

JNIEXPORT jlong JNICALL Java_com_meetingsdk_jvm_NativeBridge_pipelineCreate(JNIEnv* env, jobject /*thiz*/,
                                                                              jlong repositoryHandle,
                                                                              jstring meetingId) {
    const char* idChars = env->GetStringUTFChars(meetingId, nullptr);
    msdk_pipeline* handle = nullptr;
    const int32_t status = msdk_pipeline_create(reinterpret_cast<msdk_repository*>(repositoryHandle),
                                                 idChars, &handle);
    env->ReleaseStringUTFChars(meetingId, idChars);
    return status == MSDK_OK ? reinterpret_cast<jlong>(handle) : 0;
}

JNIEXPORT jboolean JNICALL Java_com_meetingsdk_jvm_NativeBridge_pipelineStart(JNIEnv* /*env*/,
                                                                                jobject /*thiz*/,
                                                                                jlong handle) {
    return msdk_pipeline_start(reinterpret_cast<msdk_pipeline*>(handle)) == MSDK_OK ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_meetingsdk_jvm_NativeBridge_pipelineIngestUtterance(
    JNIEnv* env, jobject /*thiz*/, jlong handle, jstring text, jlong startMs, jlong endMs,
    jstring speakerId, jfloat confidence) {
    const char* textChars = env->GetStringUTFChars(text, nullptr);
    const char* speakerChars = env->GetStringUTFChars(speakerId, nullptr);
    const int32_t status =
        msdk_pipeline_ingest_utterance(reinterpret_cast<msdk_pipeline*>(handle), textChars, startMs, endMs,
                                        speakerChars, confidence);
    env->ReleaseStringUTFChars(text, textChars);
    env->ReleaseStringUTFChars(speakerId, speakerChars);
    return status == MSDK_OK ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_com_meetingsdk_jvm_NativeBridge_pipelineStop(JNIEnv* env, jobject /*thiz*/,
                                                                              jlong handle) {
    char* json = nullptr;
    const int32_t status = msdk_pipeline_stop(reinterpret_cast<msdk_pipeline*>(handle), &json);
    if (status != MSDK_OK) {
        return nullptr;
    }
    jstring result = env->NewStringUTF(json);
    msdk_free_string(json);
    return result;
}

JNIEXPORT void JNICALL Java_com_meetingsdk_jvm_NativeBridge_pipelineDestroy(JNIEnv* /*env*/,
                                                                              jobject /*thiz*/, jlong handle) {
    msdk_pipeline_destroy(reinterpret_cast<msdk_pipeline*>(handle));
}

JNIEXPORT jlong JNICALL Java_com_meetingsdk_jvm_NativeBridge_diarizerCreate(JNIEnv* /*env*/,
                                                                              jobject /*thiz*/,
                                                                              jfloat similarityThreshold) {
    msdk_diarizer* handle = nullptr;
    const int32_t status = msdk_diarizer_create(similarityThreshold, &handle);
    return status == MSDK_OK ? reinterpret_cast<jlong>(handle) : 0;
}

JNIEXPORT jstring JNICALL Java_com_meetingsdk_jvm_NativeBridge_diarizerAssign(JNIEnv* env, jobject /*thiz*/,
                                                                                jlong handle,
                                                                                jfloatArray embedding) {
    jsize length = env->GetArrayLength(embedding);
    jfloat* elements = env->GetFloatArrayElements(embedding, nullptr);

    char* speakerId = nullptr;
    const int32_t status = msdk_diarizer_assign(reinterpret_cast<msdk_diarizer*>(handle), elements, length,
                                                 &speakerId);
    env->ReleaseFloatArrayElements(embedding, elements, JNI_ABORT);

    if (status != MSDK_OK) {
        return nullptr;
    }
    jstring result = env->NewStringUTF(speakerId);
    msdk_free_string(speakerId);
    return result;
}

JNIEXPORT void JNICALL Java_com_meetingsdk_jvm_NativeBridge_diarizerDestroy(JNIEnv* /*env*/,
                                                                              jobject /*thiz*/, jlong handle) {
    msdk_diarizer_destroy(reinterpret_cast<msdk_diarizer*>(handle));
}

}  // extern "C"
