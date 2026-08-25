#include "meeting_sdk_c/meeting_sdk.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/storage/in_memory_key_provider.hpp"
#include "meeting_sdk/storage/sodium_encryptor.hpp"
#include "meeting_sdk/storage/sqlite_meeting_repository.hpp"
#include "meeting_sdk/translation/dictionary_translator.hpp"
#include "meeting_sdk/translation/gated_translator.hpp"

namespace {

char* newCString(const std::string& s) {
    auto* out = new char[s.size() + 1];
    std::memcpy(out, s.c_str(), s.size() + 1);
    return out;
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) >= 0x20) {
                    out += c;
                }  // else: drop other control chars rather than emit invalid JSON
        }
    }
    return out;
}

std::int64_t toEpochMs(const meeting_sdk::core::Timestamp& t) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(t.value.time_since_epoch()).count();
}

std::string meetingToJson(const meeting_sdk::core::Meeting& m) {
    std::ostringstream os;
    os << R"({"id":")" << escapeJson(m.id.value) << R"(","state":")"
       << meeting_sdk::core::toString(m.state) << R"(","transcript":[)";
    for (std::size_t i = 0; i < m.transcript.size(); ++i) {
        const auto& seg = m.transcript[i];
        if (i > 0) {
            os << ",";
        }
        os << R"({"id":")" << escapeJson(seg.id.value) << R"(","text":")" << escapeJson(seg.text)
           << R"(","speaker":")" << escapeJson(seg.speaker.value) << R"(","startMs":)"
           << toEpochMs(seg.range.start) << R"(,"endMs":)" << toEpochMs(seg.range.end) << "}";
    }
    os << "]}";
    return os.str();
}

}  // namespace

struct msdk_repository {
    meeting_sdk::storage::SodiumEncryptor encryptor;
    meeting_sdk::storage::InMemoryKeyProvider keyProvider{encryptor.keyLength()};
    std::unique_ptr<meeting_sdk::storage::SqliteMeetingRepository> repository;
};

int32_t msdk_translate(const char* text, const char* target_bcp47, char** out_result) {
    if (text == nullptr || target_bcp47 == nullptr || out_result == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    meeting_sdk::translation::DictionaryTranslator dictionary;
    meeting_sdk::translation::GatedTranslator gated(meeting_sdk::core::ProviderMode::OnDevice, &dictionary);
    auto result = gated.translate(text, target_bcp47);
    if (!result) {
        return MSDK_ERROR_TRANSLATION_FAILED;
    }
    *out_result = newCString(result.value());
    return MSDK_OK;
}

void msdk_free_string(char* s) { delete[] s; }

int32_t msdk_repository_open(const char* db_path, msdk_repository** out_handle) {
    if (db_path == nullptr || out_handle == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto handle = std::make_unique<msdk_repository>();
    auto opened =
        meeting_sdk::storage::SqliteMeetingRepository::open(db_path, handle->encryptor, handle->keyProvider);
    if (!opened) {
        return MSDK_ERROR_STORAGE_FAILED;
    }
    handle->repository = std::move(opened.value());
    *out_handle = handle.release();
    return MSDK_OK;
}

void msdk_repository_close(msdk_repository* handle) { delete handle; }

int32_t msdk_repository_save_simple_meeting(msdk_repository* handle, const char* meeting_id,
                                             const char* text) {
    if (handle == nullptr || meeting_id == nullptr || text == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    meeting_sdk::core::Meeting meeting;
    meeting.id = meeting_sdk::core::MeetingId{meeting_id};
    meeting.state = meeting_sdk::core::MeetingState::Completed;
    const meeting_sdk::core::Timestamp now{std::chrono::system_clock::now()};
    meeting.range = meeting_sdk::core::TimeRange{now, now};

    meeting_sdk::core::TranscriptSegment segment;
    segment.id = meeting_sdk::core::SegmentId{std::string(meeting_id) + "-seg1"};
    segment.range = meeting.range;
    segment.text = text;
    meeting.transcript = {segment};

    return handle->repository->save(meeting) ? MSDK_OK : MSDK_ERROR_STORAGE_FAILED;
}

int32_t msdk_repository_get_meeting_json(msdk_repository* handle, const char* meeting_id,
                                          char** out_json) {
    if (handle == nullptr || meeting_id == nullptr || out_json == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto result = handle->repository->get(meeting_sdk::core::MeetingId{meeting_id});
    if (!result) {
        return MSDK_ERROR_NOT_FOUND;
    }
    *out_json = newCString(meetingToJson(result.value()));
    return MSDK_OK;
}

int32_t msdk_repository_list_meeting_ids_json(msdk_repository* handle, char** out_json) {
    if (handle == nullptr || out_json == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto result = handle->repository->listAll();
    if (!result) {
        return MSDK_ERROR_STORAGE_FAILED;
    }
    std::ostringstream os;
    os << "[";
    for (std::size_t i = 0; i < result.value().size(); ++i) {
        if (i > 0) {
            os << ",";
        }
        os << "\"" << escapeJson(result.value()[i].value) << "\"";
    }
    os << "]";
    *out_json = newCString(os.str());
    return MSDK_OK;
}

int32_t msdk_repository_remove_meeting(msdk_repository* handle, const char* meeting_id) {
    if (handle == nullptr || meeting_id == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto result = handle->repository->remove(meeting_sdk::core::MeetingId{meeting_id});
    return result ? MSDK_OK : MSDK_ERROR_STORAGE_FAILED;
}
