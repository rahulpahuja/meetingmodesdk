#include "meeting_sdk_c/meeting_sdk.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>

#include "meeting_sdk/audio/null_audio_source.hpp"
#include "meeting_sdk/audio/synthetic_audio_source.hpp"
#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/intelligence/heuristic_llm_engine.hpp"
#include "meeting_sdk/orchestration/meeting_pipeline.hpp"
#include "meeting_sdk/search/inverted_index_search.hpp"
#include "meeting_sdk/search/search_query.hpp"
#include "meeting_sdk/speaker/speaker_clusterer.hpp"
#include "meeting_sdk/speech/heuristic_language_detector.hpp"
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

meeting_sdk::core::Timestamp fromEpochMs(std::int64_t ms) {
    return meeting_sdk::core::Timestamp{
        std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(ms))};
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
    os << "],";

    os << R"("summary":)";
    if (m.summary.has_value()) {
        os << R"({"text":")" << escapeJson(m.summary->text) << R"(","keyPoints":[)";
        for (std::size_t i = 0; i < m.summary->keyPoints.size(); ++i) {
            if (i > 0) {
                os << ",";
            }
            os << "\"" << escapeJson(m.summary->keyPoints[i]) << "\"";
        }
        os << "]}";
    } else {
        os << "null";
    }

    os << R"(,"actionItems":[)";
    for (std::size_t i = 0; i < m.actionItems.size(); ++i) {
        const auto& item = m.actionItems[i];
        if (i > 0) {
            os << ",";
        }
        os << R"({"action":")" << escapeJson(item.action) << R"(","owner":)"
           << (item.owner.has_value() ? ("\"" + escapeJson(*item.owner) + "\"") : "null")
           << R"(,"confidence":)" << item.confidence << "}";
    }
    os << R"(],"decisions":[)";
    for (std::size_t i = 0; i < m.decisions.size(); ++i) {
        const auto& decision = m.decisions[i];
        if (i > 0) {
            os << ",";
        }
        os << R"({"text":")" << escapeJson(decision.text) << R"(","confidence":)" << decision.confidence
           << "}";
    }
    os << R"(],"topics":[)";
    for (std::size_t i = 0; i < m.topics.size(); ++i) {
        if (i > 0) {
            os << ",";
        }
        os << R"({"label":")" << escapeJson(m.topics[i].label) << "\"}";
    }
    os << R"(],"questions":[)";
    for (std::size_t i = 0; i < m.questions.size(); ++i) {
        const auto& question = m.questions[i];
        if (i > 0) {
            os << ",";
        }
        os << R"({"text":")" << escapeJson(question.text) << R"(","resolved":)"
           << (question.resolved ? "true" : "false") << "}";
    }
    os << "]}";
    return os.str();
}

// --- Demo pipeline fakes ----------------------------------------------------------------------
// Mirrors meeting-sdk/tests/orchestration/test_meeting_pipeline.cpp's hand-written fakes exactly
// — no real on-device STT/diarizer model ships in this SDK yet (Milestones 9-11 providers), so
// this binding is honest about using the same fixture-grade stand-ins the SDK's own tests use,
// not a fabricated "real" engine.
class DemoVad final : public meeting_sdk::core::IVAD {
public:
    explicit DemoVad(int speechFrames) : speechFrames_(speechFrames) {}
    meeting_sdk::core::Result<meeting_sdk::core::VadDecision> process(
        const meeting_sdk::core::AudioFrame&) override {
        ++seen_;
        return seen_ <= speechFrames_ ? meeting_sdk::core::VadDecision::Speech
                                       : meeting_sdk::core::VadDecision::Silence;
    }

private:
    int speechFrames_;
    int seen_ = 0;
};

class DemoSpeechToTextEngine final : public meeting_sdk::core::ISpeechToTextEngine {
public:
    explicit DemoSpeechToTextEngine(std::string text) : text_(std::move(text)) {}

    meeting_sdk::core::Result<meeting_sdk::core::TranscriptionSessionHandle> start(
        meeting_sdk::core::TranscriptionOptions, std::function<void(meeting_sdk::core::PartialResult)>,
        std::function<void(meeting_sdk::core::FinalResult)> onFinal) override {
        onFinal_ = std::move(onFinal);
        return meeting_sdk::core::TranscriptionSessionHandle{++nextHandle_};
    }
    meeting_sdk::core::Result<void> pushAudio(meeting_sdk::core::TranscriptionSessionHandle,
                                               const meeting_sdk::core::AudioFrame&) override {
        return {};
    }
    meeting_sdk::core::Result<void> finish(meeting_sdk::core::TranscriptionSessionHandle handle) override {
        meeting_sdk::core::TranscriptSegment segment;
        segment.id = meeting_sdk::core::SegmentId{"seg-" + std::to_string(handle.value)};
        // audio::SyntheticAudioSource never stamps AudioFrame::capturedAt (it defaults to the
        // epoch), so this segment's range must fall in that same epoch-relative window for
        // MeetingPipeline's diarization-span overlap check to actually assign a speaker — using
        // wall-clock "now" here would never overlap. Mirrors meeting-sdk/tests/orchestration/
        // test_meeting_pipeline.cpp's FakeSpeechToTextEngine (ts(0)/ts(1000)).
        const meeting_sdk::core::Timestamp epoch{};
        segment.range = meeting_sdk::core::TimeRange{
            epoch, meeting_sdk::core::Timestamp{epoch.value + std::chrono::milliseconds(1000)}};
        segment.text = text_;
        segment.confidence = 0.9F;
        onFinal_(meeting_sdk::core::FinalResult{handle, segment});
        return {};
    }
    meeting_sdk::core::Result<void> cancel(meeting_sdk::core::TranscriptionSessionHandle) override {
        return {};
    }

private:
    std::string text_;
    std::function<void(meeting_sdk::core::FinalResult)> onFinal_;
    std::uint64_t nextHandle_ = 0;
};

class DemoDiarizer final : public meeting_sdk::core::ISpeakerDiarizer {
public:
    meeting_sdk::core::Result<std::vector<meeting_sdk::core::SpeakerId>> diarize(
        const std::vector<meeting_sdk::core::AudioFrame>& frames) override {
        return std::vector<meeting_sdk::core::SpeakerId>(frames.size(),
                                                           meeting_sdk::core::SpeakerId{"demo-speaker"});
    }
};

class SystemClock final : public meeting_sdk::core::IClock {
public:
    meeting_sdk::core::Timestamp now() override {
        return meeting_sdk::core::Timestamp{std::chrono::system_clock::now()};
    }
};

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

int32_t msdk_repository_run_demo_meeting(msdk_repository* handle, const char* meeting_id) {
    if (handle == nullptr || meeting_id == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }

    auto source = meeting_sdk::audio::SyntheticAudioSource(std::vector<float>(960, 0.5F), 16000, 160);
    DemoVad vad(5);
    DemoSpeechToTextEngine sttEngine("We decided to launch the new design next month. I will send the "
                                      "invite to the design team. Can we confirm the launch date?");
    meeting_sdk::speech::HeuristicLanguageDetector languageDetector;
    DemoDiarizer diarizer;
    meeting_sdk::intelligence::HeuristicLlmEngine llmEngine;
    SystemClock clock;

    meeting_sdk::orchestration::MeetingPipeline pipeline(
        meeting_sdk::core::MeetingId{meeting_id},
        meeting_sdk::orchestration::MeetingPipeline::Dependencies{
            .audioSource = source,
            .vad = vad,
            .sttEngine = sttEngine,
            .languageDetector = languageDetector,
            .diarizer = diarizer,
            .llmEngine = llmEngine,
            .repository = *handle->repository,
            .clock = clock,
        });

    if (!pipeline.start()) {
        return MSDK_ERROR_PIPELINE_FAILED;
    }
    if (!pipeline.stop()) {
        return MSDK_ERROR_PIPELINE_FAILED;
    }
    return MSDK_OK;
}

struct msdk_pipeline {
    meeting_sdk::audio::NullAudioSource audioSource;
    // vad/sttEngine/diarizer are never invoked: NullAudioSource delivers no frames, so nothing
    // ever drives Preprocessor/VAD/Segmenter/STT, and ingestTranscribedSegment's caller supplies
    // speaker_id directly rather than going through ISpeakerDiarizer. They exist only to satisfy
    // MeetingPipeline::Dependencies' reference fields.
    DemoVad vad{0};
    DemoSpeechToTextEngine sttEngine{""};
    DemoDiarizer diarizer;
    meeting_sdk::speech::HeuristicLanguageDetector languageDetector;
    meeting_sdk::intelligence::HeuristicLlmEngine llmEngine;
    SystemClock clock;
    meeting_sdk::orchestration::MeetingPipeline pipeline;

    msdk_pipeline(meeting_sdk::core::MeetingId meetingId,
                  meeting_sdk::storage::SqliteMeetingRepository& repository)
        : pipeline(std::move(meetingId), meeting_sdk::orchestration::MeetingPipeline::Dependencies{
                                              .audioSource = audioSource,
                                              .vad = vad,
                                              .sttEngine = sttEngine,
                                              .languageDetector = languageDetector,
                                              .diarizer = diarizer,
                                              .llmEngine = llmEngine,
                                              .repository = repository,
                                              .clock = clock,
                                          }) {}
};

int32_t msdk_pipeline_create(msdk_repository* repository, const char* meeting_id, msdk_pipeline** out_handle) {
    if (repository == nullptr || meeting_id == nullptr || out_handle == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    *out_handle = new msdk_pipeline(meeting_sdk::core::MeetingId{meeting_id}, *repository->repository);
    return MSDK_OK;
}

int32_t msdk_pipeline_start(msdk_pipeline* handle) {
    if (handle == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    return handle->pipeline.start() ? MSDK_OK : MSDK_ERROR_PIPELINE_FAILED;
}

int32_t msdk_pipeline_ingest_utterance(msdk_pipeline* handle, const char* text, int64_t start_ms,
                                        int64_t end_ms, const char* speaker_id, float confidence) {
    if (handle == nullptr || text == nullptr || speaker_id == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto result = handle->pipeline.ingestTranscribedSegment(
        text, meeting_sdk::core::TimeRange{fromEpochMs(start_ms), fromEpochMs(end_ms)},
        meeting_sdk::core::SpeakerId{speaker_id}, confidence);
    return result ? MSDK_OK : MSDK_ERROR_PIPELINE_FAILED;
}

int32_t msdk_pipeline_stop(msdk_pipeline* handle, char** out_meeting_json) {
    if (handle == nullptr || out_meeting_json == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto result = handle->pipeline.stop();
    if (!result) {
        return MSDK_ERROR_PIPELINE_FAILED;
    }
    *out_meeting_json = newCString(meetingToJson(result.value()));
    return MSDK_OK;
}

void msdk_pipeline_destroy(msdk_pipeline* handle) { delete handle; }

struct msdk_diarizer {
    meeting_sdk::speaker::SpeakerClusterer clusterer;
    explicit msdk_diarizer(float similarityThreshold) : clusterer(similarityThreshold) {}
};

int32_t msdk_diarizer_create(float similarity_threshold, msdk_diarizer** out_handle) {
    if (out_handle == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    *out_handle = new msdk_diarizer(similarity_threshold);
    return MSDK_OK;
}

int32_t msdk_diarizer_assign(msdk_diarizer* handle, const float* embedding, int32_t embedding_length,
                              char** out_speaker_id) {
    if (handle == nullptr || embedding == nullptr || embedding_length <= 0 || out_speaker_id == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    meeting_sdk::core::SpeakerEmbedding e;
    e.vector.assign(embedding, embedding + embedding_length);
    auto id = handle->clusterer.assign(std::move(e));
    *out_speaker_id = newCString(id.value);
    return MSDK_OK;
}

void msdk_diarizer_destroy(msdk_diarizer* handle) { delete handle; }

int32_t msdk_repository_search_json(msdk_repository* handle, const char* query_text, char** out_json) {
    if (handle == nullptr || query_text == nullptr || out_json == nullptr) {
        return MSDK_ERROR_INVALID_ARGUMENT;
    }
    auto ids = handle->repository->listAll();
    if (!ids) {
        return MSDK_ERROR_STORAGE_FAILED;
    }

    meeting_sdk::search::InvertedIndexSearch index;
    for (const auto& id : ids.value()) {
        auto meeting = handle->repository->get(id);
        if (meeting) {
            index.index(id, meeting.value().transcript);
        }
    }

    meeting_sdk::search::SearchQuery query;
    query.text = query_text;
    auto results = index.search(query);

    std::ostringstream os;
    os << "[";
    for (std::size_t i = 0; i < results.size(); ++i) {
        if (i > 0) {
            os << ",";
        }
        os << R"({"meetingId":")" << escapeJson(results[i].meeting.value) << R"(","segmentId":")"
           << escapeJson(results[i].segment.value) << R"(","score":)" << results[i].score << "}";
    }
    os << "]";
    *out_json = newCString(os.str());
    return MSDK_OK;
}
