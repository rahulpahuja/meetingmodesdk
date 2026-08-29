package com.meetingsdk.jvm

import com.meetingsdk.api.MeetingRepository
import com.meetingsdk.api.SearchService
import com.meetingsdk.model.ActionItem
import com.meetingsdk.model.Decision
import com.meetingsdk.model.Meeting
import com.meetingsdk.model.MeetingState
import com.meetingsdk.model.Question
import com.meetingsdk.model.SearchResult
import com.meetingsdk.model.Summary
import com.meetingsdk.model.Topic
import com.meetingsdk.model.TranscriptSegment
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json

// Mirrors the JSON shapes documented in bindings/c/include/meeting_sdk_c/meeting_sdk.h. Kept
// separate from the domain classes so serialization annotations don't leak into the
// platform-neutral model.
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

class JvmMeetingRepository(dbPath: String) : MeetingRepository {
    internal val handle: Long = NativeBridge.repositoryOpen(dbPath)

    init {
        check(handle != 0L) { "failed to open repository at '$dbPath'" }
    }

    override fun saveSimpleMeeting(id: String, text: String): Boolean =
        NativeBridge.repositorySaveSimpleMeeting(handle, id, text)

    override fun runDemoMeeting(id: String): Boolean = NativeBridge.repositoryRunDemoMeeting(handle, id)

    override fun getMeeting(id: String): Meeting? {
        val json = NativeBridge.repositoryGetMeetingJson(handle, id) ?: return null
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

    override fun listMeetingIds(): List<String> {
        val json = NativeBridge.repositoryListMeetingIdsJson(handle) ?: return emptyList()
        return Json.decodeFromString<List<String>>(json)
    }

    override fun removeMeeting(id: String): Boolean = NativeBridge.repositoryRemoveMeeting(handle, id)

    override fun close() = NativeBridge.repositoryClose(handle)
}

/** Searches over the meetings held by a given [JvmMeetingRepository] — same native handle, no
 * separate connection. */
class JvmSearchService(private val repository: JvmMeetingRepository) : SearchService {
    override fun search(queryText: String): List<SearchResult> {
        val json = NativeBridge.repositorySearchJson(repository.handle, queryText) ?: return emptyList()
        return Json.decodeFromString<List<SearchResultJson>>(json)
            .map { SearchResult(it.meetingId, it.segmentId, it.score) }
    }
}
