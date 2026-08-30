#include <cstddef>
#include <cstdint>
#include <string>

#include "meeting_sdk_c/meeting_sdk.h"

// libFuzzer entry point. Feeds arbitrary bytes through the C ABI's string-handling and
// JSON-emitting paths: the gated translator, escapeJson / meetingToJson, the full
// encrypt -> SQLite -> decrypt -> deserialize round trip, id listing, and inverted-index
// search. Any crash, ASan report, UBSan error, or leak here is a real defect at the boundary
// every binding (JNI, Kotlin/Native, Obj-C++) sits behind.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const std::string input(reinterpret_cast<const char*>(data), size);
    char* out = nullptr;

    if (msdk_translate(input.c_str(), "en", &out) == MSDK_OK) {
        msdk_free_string(out);
        out = nullptr;
    }

    msdk_repository* repo = nullptr;
    if (msdk_repository_open(":memory:", &repo) != MSDK_OK || repo == nullptr) {
        return 0;
    }

    const std::string id = "fuzz-" + std::to_string(size);
    msdk_repository_save_simple_meeting(repo, id.c_str(), input.c_str());

    if (msdk_repository_get_meeting_json(repo, id.c_str(), &out) == MSDK_OK) {
        msdk_free_string(out);
        out = nullptr;
    }
    if (msdk_repository_list_meeting_ids_json(repo, &out) == MSDK_OK) {
        msdk_free_string(out);
        out = nullptr;
    }
    if (msdk_repository_search_json(repo, input.c_str(), &out) == MSDK_OK) {
        msdk_free_string(out);
        out = nullptr;
    }

    msdk_repository_close(repo);
    return 0;
}
