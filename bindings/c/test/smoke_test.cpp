// Sanity check for the C API surface itself, exercised as plain C (no C++ types cross the
// header). Exhaustive behavioral testing of the underlying classes lives in meeting-sdk's own
// GoogleTest suite; this only proves the ABI boundary marshals correctly end to end — the same
// thing the JNI layer will rely on.
#include "meeting_sdk_c/meeting_sdk.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::fprintf(stderr, "FAILED: %s (line %d)\n", #cond, __LINE__); \
            std::exit(1);                                               \
        }                                                               \
    } while (0)

int main() {
    char* translated = nullptr;
    CHECK(msdk_translate("namaste, kaise hain aap", "en", &translated) == MSDK_OK);
    CHECK(std::strcmp(translated, "hello, how hain you") == 0);
    msdk_free_string(translated);

    CHECK(msdk_translate("hello", "fr", &translated) != MSDK_OK);

    msdk_repository* repo = nullptr;
    CHECK(msdk_repository_open(":memory:", &repo) == MSDK_OK);

    CHECK(msdk_repository_save_simple_meeting(repo, "m1", "hello world") == MSDK_OK);

    char* json = nullptr;
    CHECK(msdk_repository_get_meeting_json(repo, "m1", &json) == MSDK_OK);
    CHECK(std::strstr(json, "hello world") != nullptr);
    CHECK(std::strstr(json, "\"id\":\"m1\"") != nullptr);
    msdk_free_string(json);

    CHECK(msdk_repository_get_meeting_json(repo, "missing", &json) == MSDK_ERROR_NOT_FOUND);

    CHECK(msdk_repository_save_simple_meeting(repo, "m2", "second meeting") == MSDK_OK);
    char* idsJson = nullptr;
    CHECK(msdk_repository_list_meeting_ids_json(repo, &idsJson) == MSDK_OK);
    CHECK(std::strstr(idsJson, "m1") != nullptr);
    CHECK(std::strstr(idsJson, "m2") != nullptr);
    msdk_free_string(idsJson);

    CHECK(msdk_repository_remove_meeting(repo, "m1") == MSDK_OK);
    CHECK(msdk_repository_get_meeting_json(repo, "m1", &json) == MSDK_ERROR_NOT_FOUND);

    CHECK(msdk_repository_run_demo_meeting(repo, "demo1") == MSDK_OK);
    CHECK(msdk_repository_get_meeting_json(repo, "demo1", &json) == MSDK_OK);
    CHECK(std::strstr(json, "\"state\":\"Completed\"") != nullptr);
    CHECK(std::strstr(json, "decided to launch") != nullptr);
    CHECK(std::strstr(json, "\"summary\":{") != nullptr);
    msdk_free_string(json);

    char* searchJson = nullptr;
    CHECK(msdk_repository_search_json(repo, "launch", &searchJson) == MSDK_OK);
    CHECK(std::strstr(searchJson, "\"meetingId\":\"demo1\"") != nullptr);
    msdk_free_string(searchJson);

    msdk_pipeline* pipeline = nullptr;
    CHECK(msdk_pipeline_create(repo, "live1", &pipeline) == MSDK_OK);
    CHECK(msdk_pipeline_start(pipeline) == MSDK_OK);
    CHECK(msdk_pipeline_ingest_utterance(pipeline, "namaste, kaise hain aap", 0, 1000, "you", 0.95F) ==
          MSDK_OK);
    CHECK(msdk_pipeline_ingest_utterance(pipeline, "", 1000, 1500, "you", 0.9F) == MSDK_OK);  // no-op

    char* liveJson = nullptr;
    CHECK(msdk_pipeline_stop(pipeline, &liveJson) == MSDK_OK);
    CHECK(std::strstr(liveJson, "\"state\":\"Completed\"") != nullptr);
    CHECK(std::strstr(liveJson, "namaste") != nullptr);
    CHECK(std::strstr(liveJson, "\"speaker\":\"you\"") != nullptr);
    msdk_free_string(liveJson);
    msdk_pipeline_destroy(pipeline);

    CHECK(msdk_repository_get_meeting_json(repo, "live1", &json) == MSDK_OK);
    CHECK(std::strstr(json, "namaste") != nullptr);
    msdk_free_string(json);

    msdk_repository_close(repo);

    msdk_diarizer* diarizer = nullptr;
    CHECK(msdk_diarizer_create(0.9F, &diarizer) == MSDK_OK);
    const float voiceA[] = {1.0F, 0.0F, 0.0F};
    const float voiceAAgain[] = {0.99F, 0.01F, 0.0F};
    const float voiceB[] = {0.0F, 1.0F, 0.0F};
    char* speakerId1 = nullptr;
    char* speakerId2 = nullptr;
    char* speakerId3 = nullptr;
    CHECK(msdk_diarizer_assign(diarizer, voiceA, 3, &speakerId1) == MSDK_OK);
    CHECK(msdk_diarizer_assign(diarizer, voiceAAgain, 3, &speakerId2) == MSDK_OK);
    CHECK(msdk_diarizer_assign(diarizer, voiceB, 3, &speakerId3) == MSDK_OK);
    CHECK(std::strcmp(speakerId1, speakerId2) == 0);  // similar voice rejoins the same speaker
    CHECK(std::strcmp(speakerId1, speakerId3) != 0);  // dissimilar voice mints a new one
    msdk_free_string(speakerId1);
    msdk_free_string(speakerId2);
    msdk_free_string(speakerId3);
    msdk_diarizer_destroy(diarizer);

    std::printf("all smoke tests passed\n");
    return 0;
}
