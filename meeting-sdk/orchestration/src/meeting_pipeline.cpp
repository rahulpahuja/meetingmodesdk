#include "meeting_sdk/orchestration/meeting_pipeline.hpp"

#include <algorithm>
#include <chrono>
#include <tuple>

#include "meeting_sdk/speaker/speaker_assigner.hpp"
#include "meeting_sdk/speech/streaming_stt_session.hpp"

namespace meeting_sdk::orchestration {

namespace {

core::Timestamp frameEnd(const core::AudioFrame& frame) {
    if (frame.sampleRateHz == 0) {
        return frame.capturedAt;
    }
    const auto durationMs =
        static_cast<long long>(frame.samples.size() * 1000ULL / frame.sampleRateHz);
    return core::Timestamp{frame.capturedAt.value + std::chrono::milliseconds(durationMs)};
}

// Groups per-frame diarizer output into contiguous same-speaker spans. frameEnd() is a
// documented approximation for the last frame of a run (no explicit frame-duration field), same
// class of approximation as HeuristicLanguageDetector's interpolated word timing.
std::vector<speaker::DiarizedSpan> buildDiarizedSpans(const std::vector<core::AudioFrame>& frames,
                                                        const std::vector<core::SpeakerId>& speakerIds) {
    std::vector<speaker::DiarizedSpan> spans;
    if (frames.empty() || frames.size() != speakerIds.size()) {
        return spans;
    }
    std::size_t runStart = 0;
    for (std::size_t i = 1; i <= frames.size(); ++i) {
        const bool boundary = (i == frames.size()) || !(speakerIds[i] == speakerIds[runStart]);
        if (!boundary) {
            continue;
        }
        const core::Timestamp end = (i < frames.size()) ? frames[i].capturedAt : frameEnd(frames[i - 1]);
        spans.push_back(speaker::DiarizedSpan{speakerIds[runStart], core::TimeRange{frames[runStart].capturedAt, end}});
        runStart = i;
    }
    return spans;
}

core::Language dominantLanguage(const std::vector<core::LanguageSegment>& segments) {
    const auto longest =
        std::max_element(segments.begin(), segments.end(), [](const auto& a, const auto& b) {
            return (a.range.end.value - a.range.start.value) < (b.range.end.value - b.range.start.value);
        });
    return longest->language;
}

// languageSegments empty if monolingual, per domain.hpp's TranscriptSegment convention.
void applyLanguageDetection(core::TranscriptSegment& segment,
                             const std::vector<core::LanguageSegment>& detected) {
    if (detected.size() <= 1) {
        segment.languageSegments.clear();
        if (!detected.empty()) {
            segment.detectedLanguage = detected.front().language;
        }
        return;
    }
    segment.languageSegments = detected;
    segment.detectedLanguage = dominantLanguage(detected);
}

}  // namespace

MeetingPipeline::MeetingPipeline(core::MeetingId meetingId, Dependencies deps)
    : deps_(deps), segmenter_([this](speech::SpeechSegment segment) { onSpeechSegment(std::move(segment)); }) {
    meeting_.id = std::move(meetingId);
    meeting_.state = core::MeetingState::Idle;
}

core::Result<void> MeetingPipeline::beginCapture() {
    auto captureStarted =
        deps_.audioSource.start([this](core::AudioFrame frame) { onAudioFrame(std::move(frame)); });
    if (!captureStarted) {
        return captureStarted;
    }
    // A synchronous core::IAudioSource (e.g. audio::SyntheticAudioSource) delivers every frame
    // inside start() itself, so a downstream stage can already have failed by the time it
    // returns — before the state machine has ever left Idle/Paused, where ->Failed isn't a legal
    // transition. Surface that error here instead of pretending recording began.
    if (fatalError_) {
        return *fatalError_;
    }
    auto transitioned = stateMachine_.transitionTo(core::MeetingState::Recording);
    if (!transitioned) {
        std::ignore = deps_.audioSource.stop();  // best-effort undo; we're already returning an error
        return transitioned.error();
    }
    meeting_.state = core::MeetingState::Recording;
    return {};
}

core::Result<void> MeetingPipeline::start(core::TranscriptionOptions sttOptions) {
    sttOptions_ = std::move(sttOptions);
    auto result = beginCapture();
    if (result) {
        meeting_.range.start = deps_.clock.now();
    }
    return result;
}

core::Result<void> MeetingPipeline::pause() {
    auto transitioned = stateMachine_.transitionTo(core::MeetingState::Paused);
    if (!transitioned) {
        return transitioned.error();
    }
    meeting_.state = core::MeetingState::Paused;
    return deps_.audioSource.stop();
}

core::Result<void> MeetingPipeline::resume(core::TranscriptionOptions sttOptions) {
    sttOptions_ = std::move(sttOptions);
    return beginCapture();
}

core::Result<core::Meeting> MeetingPipeline::stop() {
    if (fatalError_) {
        return *fatalError_;
    }

    auto captureStopped = deps_.audioSource.stop();
    if (!captureStopped) {
        return captureStopped.error();
    }
    segmenter_.flush();

    auto transitioned = stateMachine_.transitionTo(core::MeetingState::Processing);
    if (!transitioned) {
        return transitioned.error();
    }
    meeting_.state = core::MeetingState::Processing;
    meeting_.range.end = deps_.clock.now();
    meeting_.transcript = assembler_.assemble();

    auto summary = deps_.llmEngine.summarize(meeting_.transcript);
    auto actionItems = deps_.llmEngine.extractActionItems(meeting_.transcript);
    auto decisions = deps_.llmEngine.extractDecisions(meeting_.transcript);
    auto topics = deps_.llmEngine.extractTopics(meeting_.transcript);
    auto questions = deps_.llmEngine.extractQuestions(meeting_.transcript);
    if (!summary || !actionItems || !decisions || !topics || !questions) {
        core::Error error = !summary     ? summary.error()
                             : !actionItems ? actionItems.error()
                             : !decisions   ? decisions.error()
                             : !topics      ? topics.error()
                                            : questions.error();
        std::ignore = stateMachine_.transitionTo(core::MeetingState::Failed);  // always legal from Processing
        meeting_.state = core::MeetingState::Failed;
        return error;
    }
    meeting_.summary = summary.value();
    meeting_.actionItems = actionItems.value();
    meeting_.decisions = decisions.value();
    meeting_.topics = topics.value();
    meeting_.questions = questions.value();

    // Persist with the state the meeting will actually end up in, not the transient Processing
    // state — otherwise the row on disk stays stuck at Processing forever even though the
    // in-memory result correctly reports Completed. Save a Completed copy speculatively and only
    // commit it to meeting_ once the save has actually succeeded.
    core::Meeting completed = meeting_;
    completed.state = core::MeetingState::Completed;

    auto saved = deps_.repository.save(completed);
    if (!saved) {
        std::ignore = stateMachine_.transitionTo(core::MeetingState::Failed);  // always legal from Processing
        meeting_.state = core::MeetingState::Failed;
        return saved.error();
    }

    std::ignore = stateMachine_.transitionTo(core::MeetingState::Completed);  // always legal from Processing
    meeting_ = std::move(completed);
    return meeting_;
}

core::Result<void> MeetingPipeline::ingestTranscribedSegment(std::string text, core::TimeRange range,
                                                               core::SpeakerId speaker, float confidence) {
    if (fatalError_) {
        return *fatalError_;
    }
    if (stateMachine_.current() != core::MeetingState::Recording) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "orchestration.not_recording",
            .message = "ingestTranscribedSegment requires Recording state",
        };
    }
    if (text.empty()) {
        return {};  // no-op, matches TranscriptAssembler::addSegment's own convention
    }

    registerSpeaker(speaker);

    core::TranscriptSegment segment;
    segment.id = core::SegmentId{"utt-" + std::to_string(++utteranceCounter_)};
    segment.range = range;
    segment.speaker = std::move(speaker);
    segment.text = std::move(text);
    segment.confidence = confidence;

    auto languages = deps_.languageDetector.detect(segment);
    if (!languages) {
        fail(languages.error());
        return languages.error();
    }
    applyLanguageDetection(segment, languages.value());

    assembler_.addSegment(std::move(segment));
    meeting_.transcript = assembler_.assemble();
    return {};
}

core::Result<std::string> MeetingPipeline::translateSegment(const core::SegmentId& segmentId,
                                                               const std::string& targetBcp47) const {
    if (deps_.translator == nullptr) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "orchestration.translation_not_configured",
            .message = "no translator was configured for this pipeline",
        };
    }
    const auto it = std::find_if(meeting_.transcript.begin(), meeting_.transcript.end(),
                                  [&](const core::TranscriptSegment& s) { return s.id == segmentId; });
    if (it == meeting_.transcript.end()) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "orchestration.segment_not_found",
            .message = "no transcript segment with the given id",
        };
    }
    return deps_.translator->translate(it->text, targetBcp47);
}

core::MeetingState MeetingPipeline::state() const noexcept { return stateMachine_.current(); }

const core::Meeting& MeetingPipeline::snapshot() const noexcept { return meeting_; }

void MeetingPipeline::onAudioFrame(core::AudioFrame frame) {
    if (fatalError_) {
        return;
    }
    preprocessor_.process(frame);
    auto decision = deps_.vad.process(frame);
    if (!decision) {
        fail(decision.error());
        return;
    }
    segmenter_.pushFrame(std::move(frame), decision.value());
}

void MeetingPipeline::onSpeechSegment(speech::SpeechSegment segment) {
    if (fatalError_) {
        return;
    }
    std::vector<core::AudioFrame> frames = segment.frames;

    speech::StreamingSttSession session(deps_.sttEngine);
    auto started = session.start(
        sttOptions_, [](core::PartialResult) {},
        [this, frames](core::FinalResult result) mutable {
            onFinalResult(std::move(result), std::move(frames));
        });
    if (!started) {
        fail(started.error());
        return;
    }
    for (auto& frame : segment.frames) {
        auto pushed = session.pushAudio(frame);
        if (!pushed) {
            fail(pushed.error());
            return;
        }
    }
    auto finished = session.finish();
    if (!finished) {
        fail(finished.error());
        return;
    }
}

void MeetingPipeline::onFinalResult(core::FinalResult result, std::vector<core::AudioFrame> frames) {
    if (fatalError_) {
        return;
    }
    core::TranscriptSegment segment = std::move(result.segment);

    auto diarized = deps_.diarizer.diarize(frames);
    if (!diarized) {
        fail(diarized.error());
        return;
    }
    auto spans = buildDiarizedSpans(frames, diarized.value());
    for (const auto& span : spans) {
        registerSpeaker(span.speaker);
    }
    speaker::SpeakerAssigner assigner(spans);
    auto assigned = assigner.assign({segment});
    if (!assigned.empty()) {
        segment = std::move(assigned.front());
    }

    auto languages = deps_.languageDetector.detect(segment);
    if (!languages) {
        fail(languages.error());
        return;
    }
    applyLanguageDetection(segment, languages.value());

    assembler_.addSegment(std::move(segment));
    meeting_.transcript = assembler_.assemble();
}

void MeetingPipeline::registerSpeaker(const core::SpeakerId& id) {
    const bool exists = std::any_of(meeting_.speakers.begin(), meeting_.speakers.end(),
                                     [&](const core::Speaker& speaker) { return speaker.id == id; });
    if (!exists) {
        core::Speaker speaker;
        speaker.id = id;
        meeting_.speakers.push_back(std::move(speaker));
    }
}

void MeetingPipeline::fail(core::Error error) {
    if (!fatalError_) {
        fatalError_ = std::move(error);
    }
    std::ignore = deps_.audioSource.stop();  // best-effort; we're already reporting fatalError_
    // Ignored if illegal (e.g. still Idle — see beginCapture()): fatalError_ being set is what
    // callers actually observe in that case, not the state machine.
    auto transitioned = stateMachine_.transitionTo(core::MeetingState::Failed);
    if (transitioned) {
        meeting_.state = core::MeetingState::Failed;
    }
}

}  // namespace meeting_sdk::orchestration
