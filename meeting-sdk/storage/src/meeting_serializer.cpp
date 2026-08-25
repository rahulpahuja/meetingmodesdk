#include "meeting_sdk/storage/meeting_serializer.hpp"

#include <chrono>
#include <cstring>
#include <optional>
#include <string>

namespace meeting_sdk::storage {
namespace {

class Writer {
public:
    void u32(std::uint32_t v) { append(&v, sizeof(v)); }
    void i64(std::int64_t v) { append(&v, sizeof(v)); }
    void f32(float v) { append(&v, sizeof(v)); }
    void boolean(bool v) { buffer_.push_back(static_cast<std::uint8_t>(v ? 1 : 0)); }

    void str(const std::string& s) {
        u32(static_cast<std::uint32_t>(s.size()));
        buffer_.insert(buffer_.end(), s.begin(), s.end());
    }
    void timestamp(const core::Timestamp& t) {
        i64(std::chrono::duration_cast<std::chrono::milliseconds>(t.value.time_since_epoch()).count());
    }
    void timeRange(const core::TimeRange& r) {
        timestamp(r.start);
        timestamp(r.end);
    }
    void language(const core::Language& l) {
        str(l.bcp47Code);
        f32(l.confidence);
    }
    void optionalStr(const std::optional<std::string>& s) {
        boolean(s.has_value());
        if (s) {
            str(*s);
        }
    }
    void optionalTimestamp(const std::optional<core::Timestamp>& t) {
        boolean(t.has_value());
        if (t) {
            timestamp(*t);
        }
    }

    const std::vector<std::uint8_t>& bytes() const noexcept { return buffer_; }

private:
    void append(const void* data, std::size_t size) {
        const auto* bytePtr = static_cast<const std::uint8_t*>(data);
        buffer_.insert(buffer_.end(), bytePtr, bytePtr + size);
    }
    std::vector<std::uint8_t> buffer_;
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& data) : data_(data) {}

    core::Result<std::uint32_t> u32() {
        auto v = readRaw<std::uint32_t>();
        if (!v) return corrupt();
        return *v;
    }
    core::Result<std::int64_t> i64() {
        auto v = readRaw<std::int64_t>();
        if (!v) return corrupt();
        return *v;
    }
    core::Result<float> f32() {
        auto v = readRaw<float>();
        if (!v) return corrupt();
        return *v;
    }
    core::Result<bool> boolean() {
        if (pos_ >= data_.size()) return corrupt();
        return data_[pos_++] != 0;
    }
    core::Result<std::string> str() {
        auto len = u32();
        if (!len) return len.error();
        if (pos_ + len.value() > data_.size()) return corrupt();
        std::string s(data_.begin() + static_cast<std::ptrdiff_t>(pos_),
                       data_.begin() + static_cast<std::ptrdiff_t>(pos_ + len.value()));
        pos_ += len.value();
        return s;
    }
    core::Result<core::Timestamp> timestamp() {
        auto ms = i64();
        if (!ms) return ms.error();
        return core::Timestamp{std::chrono::system_clock::time_point{} +
                                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                                    std::chrono::milliseconds(ms.value()))};
    }
    core::Result<core::TimeRange> timeRange() {
        auto start = timestamp();
        if (!start) return start.error();
        auto end = timestamp();
        if (!end) return end.error();
        return core::TimeRange{start.value(), end.value()};
    }
    core::Result<core::Language> language() {
        auto code = str();
        if (!code) return code.error();
        auto conf = f32();
        if (!conf) return conf.error();
        return core::Language{.bcp47Code = code.value(), .confidence = conf.value()};
    }
    core::Result<std::optional<std::string>> optionalStr() {
        auto has = boolean();
        if (!has) return has.error();
        if (!has.value()) return std::optional<std::string>{std::nullopt};
        auto s = str();
        if (!s) return s.error();
        return std::optional<std::string>{s.value()};
    }
    core::Result<std::optional<core::Timestamp>> optionalTimestamp() {
        auto has = boolean();
        if (!has) return has.error();
        if (!has.value()) return std::optional<core::Timestamp>{std::nullopt};
        auto t = timestamp();
        if (!t) return t.error();
        return std::optional<core::Timestamp>{t.value()};
    }

private:
    template <typename T>
    std::optional<T> readRaw() {
        if (pos_ + sizeof(T) > data_.size()) return std::nullopt;
        T v;
        std::memcpy(&v, data_.data() + pos_, sizeof(T));
        pos_ += sizeof(T);
        return v;
    }
    static core::Error corrupt() {
        return core::Error{
            .category = core::ErrorCategory::Storage,
            .code = "storage.corrupt_record",
            .message = "meeting record is truncated or corrupt",
        };
    }

    const std::vector<std::uint8_t>& data_;
    std::size_t pos_ = 0;
};

}  // namespace

std::vector<std::uint8_t> MeetingSerializer::serialize(const core::Meeting& meeting) {
    Writer w;
    w.str(meeting.id.value);
    w.u32(static_cast<std::uint32_t>(meeting.state));
    w.timeRange(meeting.range);

    w.u32(static_cast<std::uint32_t>(meeting.speakers.size()));
    for (const auto& speaker : meeting.speakers) {
        w.str(speaker.id.value);
        w.optionalStr(speaker.displayName);
        w.u32(static_cast<std::uint32_t>(speaker.embeddings.size()));
        for (const auto& emb : speaker.embeddings) {
            w.str(emb.speaker.value);
            w.u32(static_cast<std::uint32_t>(emb.vector.size()));
            for (float f : emb.vector) w.f32(f);
            w.timestamp(emb.capturedAt);
        }
    }

    w.u32(static_cast<std::uint32_t>(meeting.transcript.size()));
    for (const auto& seg : meeting.transcript) {
        w.str(seg.id.value);
        w.timeRange(seg.range);
        w.str(seg.speaker.value);
        w.str(seg.text);
        w.language(seg.detectedLanguage);
        w.f32(seg.confidence);
        w.u32(static_cast<std::uint32_t>(seg.languageSegments.size()));
        for (const auto& ls : seg.languageSegments) {
            w.timeRange(ls.range);
            w.language(ls.language);
        }
    }

    w.boolean(meeting.summary.has_value());
    if (meeting.summary) {
        w.str(meeting.summary->text);
        w.u32(static_cast<std::uint32_t>(meeting.summary->keyPoints.size()));
        for (const auto& kp : meeting.summary->keyPoints) w.str(kp);
    }

    w.u32(static_cast<std::uint32_t>(meeting.actionItems.size()));
    for (const auto& item : meeting.actionItems) {
        w.str(item.action);
        w.optionalStr(item.owner);
        w.optionalTimestamp(item.deadline);
        w.str(item.sourceSegment.value);
        w.f32(item.confidence);
    }

    w.u32(static_cast<std::uint32_t>(meeting.decisions.size()));
    for (const auto& d : meeting.decisions) {
        w.str(d.text);
        w.str(d.sourceSegment.value);
        w.f32(d.confidence);
    }

    w.u32(static_cast<std::uint32_t>(meeting.topics.size()));
    for (const auto& t : meeting.topics) {
        w.str(t.label);
        w.u32(static_cast<std::uint32_t>(t.relatedSegments.size()));
        for (const auto& seg : t.relatedSegments) w.str(seg.value);
    }

    w.u32(static_cast<std::uint32_t>(meeting.questions.size()));
    for (const auto& q : meeting.questions) {
        w.str(q.text);
        w.str(q.sourceSegment.value);
        w.boolean(q.resolved);
    }

    return w.bytes();
}

core::Result<core::Meeting> MeetingSerializer::deserialize(const std::vector<std::uint8_t>& bytes) {
    Reader r(bytes);
    core::Meeting meeting;

    auto id = r.str();
    if (!id) return id.error();
    meeting.id = core::MeetingId{id.value()};

    auto state = r.u32();
    if (!state) return state.error();
    meeting.state = static_cast<core::MeetingState>(state.value());

    auto range = r.timeRange();
    if (!range) return range.error();
    meeting.range = range.value();

    auto speakerCount = r.u32();
    if (!speakerCount) return speakerCount.error();
    for (std::uint32_t i = 0; i < speakerCount.value(); ++i) {
        core::Speaker speaker;
        auto sid = r.str();
        if (!sid) return sid.error();
        speaker.id = core::SpeakerId{sid.value()};

        auto displayName = r.optionalStr();
        if (!displayName) return displayName.error();
        speaker.displayName = displayName.value();

        auto embCount = r.u32();
        if (!embCount) return embCount.error();
        for (std::uint32_t j = 0; j < embCount.value(); ++j) {
            core::SpeakerEmbedding emb;
            auto embSpeaker = r.str();
            if (!embSpeaker) return embSpeaker.error();
            emb.speaker = core::SpeakerId{embSpeaker.value()};

            auto vecLen = r.u32();
            if (!vecLen) return vecLen.error();
            emb.vector.reserve(vecLen.value());
            for (std::uint32_t k = 0; k < vecLen.value(); ++k) {
                auto f = r.f32();
                if (!f) return f.error();
                emb.vector.push_back(f.value());
            }

            auto capturedAt = r.timestamp();
            if (!capturedAt) return capturedAt.error();
            emb.capturedAt = capturedAt.value();

            speaker.embeddings.push_back(std::move(emb));
        }
        meeting.speakers.push_back(std::move(speaker));
    }

    auto segCount = r.u32();
    if (!segCount) return segCount.error();
    for (std::uint32_t i = 0; i < segCount.value(); ++i) {
        core::TranscriptSegment seg;
        auto segId = r.str();
        if (!segId) return segId.error();
        seg.id = core::SegmentId{segId.value()};

        auto segRange = r.timeRange();
        if (!segRange) return segRange.error();
        seg.range = segRange.value();

        auto segSpeaker = r.str();
        if (!segSpeaker) return segSpeaker.error();
        seg.speaker = core::SpeakerId{segSpeaker.value()};

        auto text = r.str();
        if (!text) return text.error();
        seg.text = text.value();

        auto lang = r.language();
        if (!lang) return lang.error();
        seg.detectedLanguage = lang.value();

        auto conf = r.f32();
        if (!conf) return conf.error();
        seg.confidence = conf.value();

        auto lsCount = r.u32();
        if (!lsCount) return lsCount.error();
        for (std::uint32_t j = 0; j < lsCount.value(); ++j) {
            auto lsRange = r.timeRange();
            if (!lsRange) return lsRange.error();
            auto lsLang = r.language();
            if (!lsLang) return lsLang.error();
            seg.languageSegments.push_back(core::LanguageSegment{lsRange.value(), lsLang.value()});
        }
        meeting.transcript.push_back(std::move(seg));
    }

    auto hasSummary = r.boolean();
    if (!hasSummary) return hasSummary.error();
    if (hasSummary.value()) {
        core::Summary summary;
        auto text = r.str();
        if (!text) return text.error();
        summary.text = text.value();

        auto kpCount = r.u32();
        if (!kpCount) return kpCount.error();
        for (std::uint32_t i = 0; i < kpCount.value(); ++i) {
            auto kp = r.str();
            if (!kp) return kp.error();
            summary.keyPoints.push_back(kp.value());
        }
        meeting.summary = std::move(summary);
    }

    auto actionCount = r.u32();
    if (!actionCount) return actionCount.error();
    for (std::uint32_t i = 0; i < actionCount.value(); ++i) {
        core::ActionItem item;
        auto action = r.str();
        if (!action) return action.error();
        item.action = action.value();

        auto owner = r.optionalStr();
        if (!owner) return owner.error();
        item.owner = owner.value();

        auto deadline = r.optionalTimestamp();
        if (!deadline) return deadline.error();
        item.deadline = deadline.value();

        auto src = r.str();
        if (!src) return src.error();
        item.sourceSegment = core::SegmentId{src.value()};

        auto conf = r.f32();
        if (!conf) return conf.error();
        item.confidence = conf.value();

        meeting.actionItems.push_back(std::move(item));
    }

    auto decisionCount = r.u32();
    if (!decisionCount) return decisionCount.error();
    for (std::uint32_t i = 0; i < decisionCount.value(); ++i) {
        core::Decision d;
        auto text = r.str();
        if (!text) return text.error();
        d.text = text.value();
        auto src = r.str();
        if (!src) return src.error();
        d.sourceSegment = core::SegmentId{src.value()};
        auto conf = r.f32();
        if (!conf) return conf.error();
        d.confidence = conf.value();
        meeting.decisions.push_back(std::move(d));
    }

    auto topicCount = r.u32();
    if (!topicCount) return topicCount.error();
    for (std::uint32_t i = 0; i < topicCount.value(); ++i) {
        core::Topic t;
        auto label = r.str();
        if (!label) return label.error();
        t.label = label.value();
        auto relCount = r.u32();
        if (!relCount) return relCount.error();
        for (std::uint32_t j = 0; j < relCount.value(); ++j) {
            auto seg = r.str();
            if (!seg) return seg.error();
            t.relatedSegments.push_back(core::SegmentId{seg.value()});
        }
        meeting.topics.push_back(std::move(t));
    }

    auto questionCount = r.u32();
    if (!questionCount) return questionCount.error();
    for (std::uint32_t i = 0; i < questionCount.value(); ++i) {
        core::Question q;
        auto text = r.str();
        if (!text) return text.error();
        q.text = text.value();
        auto src = r.str();
        if (!src) return src.error();
        q.sourceSegment = core::SegmentId{src.value()};
        auto resolved = r.boolean();
        if (!resolved) return resolved.error();
        q.resolved = resolved.value();
        meeting.questions.push_back(std::move(q));
    }

    return meeting;
}

}  // namespace meeting_sdk::storage
