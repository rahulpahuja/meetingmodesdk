#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "meeting_sdk/audio/preprocessor.hpp"
#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/core/errors.hpp"
#include "meeting_sdk/core/interfaces.hpp"
#include "meeting_sdk/orchestration/meeting_state_machine.hpp"
#include "meeting_sdk/orchestration/transcript_assembler.hpp"
#include "meeting_sdk/speech/segmenter.hpp"

namespace meeting_sdk::orchestration {

// Composes the full pipeline (see docs/architecture/05-diagrams.md §3: Audio Capture -> Noise
// Reduction -> VAD -> Segmentation -> STT -> Diarization -> Speaker Assignment -> Transcript
// Assembly -> Meeting Intelligence -> Storage) into one driven session, one instance per Meeting.
// Every pluggable stage is injected as a core:: interface reference (Strategy pattern) and must
// outlive the pipeline, same contract as SqliteMeetingRepository's injected dependencies.
// audio::Preprocessor and speech::Segmenter have no interface of their own yet (see their
// headers), so the pipeline owns them directly — matching 05-diagrams.md §2, where orchestration
// alone may depend on every feature module's concrete types, not just core.
//
// Diarization is injected as core::ISpeakerDiarizer rather than composed from
// speaker::SpeakerClusterer directly: SpeakerClusterer clusters SpeakerEmbeddings, but nothing in
// this tree extracts embeddings from audio (that needs a neural model — the same "no model
// available in this environment" constraint that leaves STT and diarization providers unshipped
// elsewhere in this codebase). A real ISpeakerDiarizer implementation wraps extraction +
// clustering together behind the one interface orchestration already depends on.
//
// Not thread-safe by itself: like MeetingStateMachine, callers serialize onto a single
// processing thread (docs/architecture/02-interfaces-and-data-models.md §3.5). This pipeline
// does not itself hand off between a capture thread and a worker thread via audio::RingBuffer —
// that handoff belongs to a hardware-backed core::IAudioSource (Milestones 9-10); the
// deterministic audio::SyntheticAudioSource used in tests delivers frames synchronously.
class MeetingPipeline {
public:
    struct Dependencies {
        core::IAudioSource& audioSource;
        core::IVAD& vad;
        core::ISpeechToTextEngine& sttEngine;
        core::ILanguageDetector& languageDetector;
        core::ISpeakerDiarizer& diarizer;
        core::ILLMEngine& llmEngine;
        core::IMeetingRepository& repository;
        core::IClock& clock;
        core::ITranslator* translator = nullptr;  // nullptr: translateSegment() always rejects
    };

    MeetingPipeline(core::MeetingId meetingId, Dependencies deps);

    // Idle -> Recording. Starts audio capture; frames flow through preprocessing, VAD,
    // segmentation, STT, diarization, and language detection until pause()/stop(). Any
    // downstream stage error moves the meeting straight to Failed.
    core::Result<void> start(core::TranscriptionOptions sttOptions = {});

    // Recording -> Paused. Suspends audio capture without ending the meeting.
    core::Result<void> pause();

    // Paused -> Recording. Resumes audio capture.
    core::Result<void> resume(core::TranscriptionOptions sttOptions = {});

    // Recording|Paused -> Processing -> Completed|Failed. Stops capture, assembles the ordered
    // transcript, runs meeting intelligence extraction over it, persists the finished Meeting via
    // Dependencies::repository, and returns it.
    core::Result<core::Meeting> stop();

    // Adds one already-transcribed utterance directly to the transcript, for STT sources that
    // capture and transcribe as one atomic step and can only hand back recognized text — not raw
    // AudioFrames — e.g. a platform speech service like Android's SpeechRecognizer, which owns
    // its own microphone capture. Requires Recording (started with audio::NullAudioSource or
    // equivalent, since this bypasses the frame-driven audioSource/vad/segmenter/sttEngine chain
    // entirely). Runs real language detection on the text via Dependencies::languageDetector, but
    // not diarization: ISpeakerDiarizer needs raw AudioFrames this path never has, and fabricating
    // fake frames just to satisfy it would be worse than the caller supplying the speaker
    // directly, so `speaker` is taken as given. No-op if text is empty, matching
    // TranscriptAssembler::addSegment's own convention.
    core::Result<void> ingestTranscribedSegment(std::string text, core::TimeRange range,
                                                 core::SpeakerId speaker, float confidence);

    // On-demand translation of one already-assembled transcript segment; independent of the
    // automatic stages above (see 05-diagrams.md §3's "Translation enabled?" branch — gating is
    // Dependencies::translator's own concern, e.g. a translation::GatedTranslator constructed
    // with the host app's core::AIProviderConfig::translation mode, not re-implemented here).
    core::Result<std::string> translateSegment(const core::SegmentId& segmentId,
                                                const std::string& targetBcp47) const;

    core::MeetingState state() const noexcept;
    const core::Meeting& snapshot() const noexcept;

private:
    core::Result<void> beginCapture();
    void onAudioFrame(core::AudioFrame frame);
    void onSpeechSegment(speech::SpeechSegment segment);
    void onFinalResult(core::FinalResult result, std::vector<core::AudioFrame> frames);
    void registerSpeaker(const core::SpeakerId& id);
    void fail(core::Error error);

    Dependencies deps_;
    core::Meeting meeting_;
    MeetingStateMachine stateMachine_;
    TranscriptAssembler assembler_;
    audio::Preprocessor preprocessor_;
    speech::Segmenter segmenter_;
    core::TranscriptionOptions sttOptions_;
    std::optional<core::Error> fatalError_;
    std::uint64_t utteranceCounter_ = 0;
};

}  // namespace meeting_sdk::orchestration
