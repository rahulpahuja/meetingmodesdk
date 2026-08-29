package com.meetingsdk.android

import com.meetingsdk.jvm.NativeBridge
import com.meetingsdk.model.ActionItem
import com.meetingsdk.model.Decision
import com.meetingsdk.model.Meeting
import com.meetingsdk.model.MeetingState
import com.meetingsdk.model.ProviderMode
import com.meetingsdk.model.Question
import com.meetingsdk.model.SearchResult
import com.meetingsdk.model.Summary
import com.meetingsdk.model.Topic
import com.meetingsdk.model.TranscriptSegment
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json

// Mirrors the JSON shapes documented in bindings/c/include/meeting_sdk_c/meeting_sdk.h — see
// kmp/src/jvmMain/kotlin/com/meetingsdk/jvm/JvmMeetingRepository.kt, which this is adapted from.
@Serializable
private data class SegmentJson(
    val id: String,
    val text: String,
    val speaker: String,
    val startMs: Long,
    val endMs: Long,
)

@Serializable
private data class SummaryJson(val text: String, val keyPoints: List<String>)

@Serializable
private data class ActionItemJson(val action: String, val owner: String?, val confidence: Float)

@Serializable
private data class DecisionJson(val text: String, val confidence: Float)

@Serializable
private data class TopicJson(val label: String)

@Serializable
private data class QuestionJson(val text: String, val resolved: Boolean)

@Serializable
private data class MeetingJson(
    val id: String,
    val state: String,
    val transcript: List<SegmentJson>,
    val summary: SummaryJson? = null,
    val actionItems: List<ActionItemJson> = emptyList(),
    val decisions: List<DecisionJson> = emptyList(),
    val topics: List<TopicJson> = emptyList(),
    val questions: List<QuestionJson> = emptyList(),
)

@Serializable
private data class SearchResultJson(val meetingId: String, val segmentId: String, val score: Int)

// Shared by NativeMeetingRepository.getMeeting() and NativeMeetingPipeline.stop() — both receive
// the same {"id":...,"state":...,"transcript":[...],...} JSON shape from the native side.
private fun parseMeetingJson(json: String): Meeting {
    val dto = Json.decodeFromString<MeetingJson>(json)
    return Meeting(
        id = dto.id,
        state = MeetingState.valueOf(dto.state.uppercase()),
        transcript = dto.transcript.map { TranscriptSegment(it.id, it.speaker, it.text, it.startMs, it.endMs) },
        summary = dto.summary?.let { Summary(it.text, it.keyPoints) },
        actionItems = dto.actionItems.map { ActionItem(it.action, it.owner, it.confidence) },
        decisions = dto.decisions.map { Decision(it.text, it.confidence) },
        topics = dto.topics.map { Topic(it.label) },
        questions = dto.questions.map { Question(it.text, it.resolved) },
    )
}

/**
 * Encrypted meeting persistence (SqliteMeetingRepository + SodiumEncryptor under the hood) plus
 * the demo pipeline runner and keyword search, all reached through [NativeBridge]. Not
 * thread-safe: callers serialize access to one instance, same contract as the JVM binding.
 *
 * The native side uses storage::InMemoryKeyProvider — the SDK's own dev/test key provider, not
 * Android Keystore-backed. meeting-sdk/README.md is explicit that a real deployment's
 * IKeyProvider wraps keys via Android Keystore in a Milestone 9/10 platform bridge; this app
 * doesn't fake that, it's honestly the same dev-grade key handling meeting-sdk's own tests use.
 */
class NativeMeetingRepository(dbPath: String) {
    val handle: Long = NativeBridge.repositoryOpen(dbPath)

    init {
        check(handle != 0L) { "failed to open repository at '$dbPath'" }
    }

    fun saveSimpleMeeting(id: String, text: String): Boolean =
        NativeBridge.repositorySaveSimpleMeeting(handle, id, text)

    /**
     * Runs meeting_sdk::orchestration::MeetingPipeline end-to-end over a short, deterministic
     * synthetic utterance and persists the result. STT and diarization are the same hand-written
     * demo fakes meeting-sdk's own pipeline tests use (no on-device model for either ships in
     * this SDK yet); language detection and intelligence extraction are real.
     */
    fun runDemoMeeting(id: String): Boolean = NativeBridge.repositoryRunDemoMeeting(handle, id)

    fun getMeeting(id: String): Meeting? {
        val json = NativeBridge.repositoryGetMeetingJson(handle, id) ?: return null
        return parseMeetingJson(json)
    }

    fun listMeetingIds(): List<String> {
        val json = NativeBridge.repositoryListMeetingIdsJson(handle) ?: return emptyList()
        return Json.decodeFromString<List<String>>(json)
    }

    fun removeMeeting(id: String): Boolean = NativeBridge.repositoryRemoveMeeting(handle, id)

    fun close() = NativeBridge.repositoryClose(handle)
}

/** Keyword search (meeting_sdk::search::InvertedIndexSearch) over every meeting [repository] holds. */
class NativeSearchService(private val repository: NativeMeetingRepository) {
    fun search(queryText: String): List<SearchResult> {
        if (queryText.isBlank()) return emptyList()
        val json = NativeBridge.repositorySearchJson(repository.handle, queryText) ?: return emptyList()
        return Json.decodeFromString<List<SearchResultJson>>(json)
            .map { SearchResult(it.meetingId, it.segmentId, it.score) }
    }
}

/**
 * Live meeting_sdk::orchestration::MeetingPipeline session for STT sources that capture and
 * transcribe as one atomic step (e.g. Android's SpeechRecognizer) and can only hand back
 * recognized text, not raw audio frames — see MeetingPipeline::ingestTranscribedSegment's own
 * doc comment. Unlike [NativeMeetingRepository.runDemoMeeting], this carries real speech: only
 * language detection and meeting intelligence extraction are the SDK's heuristic engines, same as
 * everywhere else in this app — nothing here is a transcription fixture.
 *
 * One instance per meeting: create() -> start() -> ingestUtterance() per recognized phrase ->
 * stop() (persists into [repository]). Call destroy() when done, whether or not stop() was
 * reached (e.g. the user cancels mid-recording).
 */
class NativeMeetingPipeline(repository: NativeMeetingRepository, meetingId: String) {
    private val handle: Long = NativeBridge.pipelineCreate(repository.handle, meetingId)

    init {
        check(handle != 0L) { "failed to create pipeline for meeting '$meetingId'" }
    }

    fun start(): Boolean = NativeBridge.pipelineStart(handle)

    fun ingestUtterance(text: String, startMs: Long, endMs: Long, speakerId: String, confidence: Float): Boolean =
        NativeBridge.pipelineIngestUtterance(handle, text, startMs, endMs, speakerId, confidence)

    fun stop(): Meeting? = NativeBridge.pipelineStop(handle)?.let { parseMeetingJson(it) }

    fun destroy() = NativeBridge.pipelineDestroy(handle)
}

/**
 * On-device translation (translation::DictionaryTranslator, gated OnDevice by msdk_translate).
 * Disabled mode is enforced here at the call boundary rather than natively: the narrow demo C
 * API always constructs an OnDevice-gated GatedTranslator (see msdk_translate's doc comment), so
 * a host app that wants Disabled to mean something real simply doesn't call it — which is what
 * this class does when told translation is off, instead of pretending to thread a mode through.
 */
class NativeTranslationService {
    fun translate(text: String, targetBcp47: String, mode: ProviderMode): Result<String> {
        if (mode == ProviderMode.DISABLED) {
            return Result.failure(IllegalStateException("translation is disabled in Settings"))
        }
        val translated = NativeBridge.translate(text, targetBcp47)
        return if (translated != null) {
            Result.success(translated)
        } else {
            Result.failure(IllegalArgumentException("translation failed for target '$targetBcp47'"))
        }
    }
}
