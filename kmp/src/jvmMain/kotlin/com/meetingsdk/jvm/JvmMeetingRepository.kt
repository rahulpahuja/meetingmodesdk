package com.meetingsdk.jvm

import com.meetingsdk.api.MeetingRepository
import com.meetingsdk.model.Meeting
import com.meetingsdk.model.MeetingState
import com.meetingsdk.model.TranscriptSegment
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json

// Mirrors the JSON shape documented in bindings/c/include/meeting_sdk_c/meeting_sdk.h. Kept
// separate from the domain Meeting/TranscriptSegment classes so serialization annotations
// don't leak into the platform-neutral model.
@Serializable
private data class SegmentJson(
    val id: String,
    val text: String,
    val speaker: String,
    val startMs: Long,
    val endMs: Long,
)

@Serializable
private data class MeetingJson(val id: String, val state: String, val transcript: List<SegmentJson>)

class JvmMeetingRepository(dbPath: String) : MeetingRepository {
    private val handle: Long = NativeBridge.repositoryOpen(dbPath)

    init {
        check(handle != 0L) { "failed to open repository at '$dbPath'" }
    }

    override fun saveSimpleMeeting(id: String, text: String): Boolean =
        NativeBridge.repositorySaveSimpleMeeting(handle, id, text)

    override fun getMeeting(id: String): Meeting? {
        val json = NativeBridge.repositoryGetMeetingJson(handle, id) ?: return null
        val dto = Json.decodeFromString<MeetingJson>(json)
        return Meeting(
            id = dto.id,
            state = MeetingState.valueOf(dto.state.uppercase()),
            transcript = dto.transcript.map { TranscriptSegment(it.id, it.speaker, it.text, it.startMs, it.endMs) },
        )
    }

    override fun listMeetingIds(): List<String> {
        val json = NativeBridge.repositoryListMeetingIdsJson(handle) ?: return emptyList()
        return Json.decodeFromString<List<String>>(json)
    }

    override fun removeMeeting(id: String): Boolean = NativeBridge.repositoryRemoveMeeting(handle, id)

    override fun close() = NativeBridge.repositoryClose(handle)
}
