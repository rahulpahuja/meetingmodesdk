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

    msdk_repository_close(repo);

    std::printf("all smoke tests passed\n");
    return 0;
}
