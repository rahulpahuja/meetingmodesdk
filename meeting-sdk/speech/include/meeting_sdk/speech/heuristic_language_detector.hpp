#pragma once

#include <vector>

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::speech {

// Word-level Hindi/English (including romanized Hindi) language detector. Classifies each word
// by script (Devanagari => Hindi) or, for Latin-script words, by membership in a small closed
// set of common romanized Hindi function words; adjacent same-language words are merged into
// one LanguageSegment each. This is a deterministic on-device baseline, not a statistical
// language-ID model — swappable behind core::ILanguageDetector for a real model later (Strategy
// pattern, see docs/architecture/01-requirements-and-architecture.md §4).
//
// detect() always returns the full word-level breakdown, including a single segment spanning
// the whole range for monolingual text; per domain.hpp's "languageSegments empty if
// monolingual" convention, it is the caller's job to collapse a single-segment result to an
// empty vector before storing it on a TranscriptSegment.
//
// Word-level timing is not available from a TranscriptSegment alone (no per-word timestamps),
// so each LanguageSegment's TimeRange is linearly interpolated across the segment's overall
// range by word position — a documented approximation, superseded once a provider supplies
// real per-word timestamps.
class HeuristicLanguageDetector final : public core::ILanguageDetector {
public:
    core::Result<std::vector<core::LanguageSegment>> detect(
        const core::TranscriptSegment& segment) override;
};

}  // namespace meeting_sdk::speech
