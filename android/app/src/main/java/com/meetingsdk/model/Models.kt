package com.meetingsdk.model

/**
 * Platform-neutral domain models mirroring meeting_sdk::core — see
 * kmp/src/commonMain/kotlin/com/meetingsdk/model/Models.kt, which this is a copy of. Duplicated
 * here rather than depended on because kmp's build.gradle.kts scopes androidTarget() to
 * Milestone 9 (Android capture/permissions/lifecycle work not yet in scope); this app is its own
 * standalone consumer of the C API until that KMP target exists.
 */

enum class MeetingState { IDLE, RECORDING, PAUSED, PROCESSING, COMPLETED, FAILED }

enum class ProviderMode { ON_DEVICE, CLOUD, DISABLED }

data class TranscriptSegment(
    val id: String,
    val speakerId: String,
    val text: String,
    val startEpochMs: Long,
    val endEpochMs: Long,
)

data class Summary(val text: String, val keyPoints: List<String>)

data class ActionItem(val action: String, val owner: String?, val confidence: Float)

data class Decision(val text: String, val confidence: Float)

data class Topic(val label: String)

data class Question(val text: String, val resolved: Boolean)

data class Meeting(
    val id: String,
    val state: MeetingState,
    val transcript: List<TranscriptSegment>,
    val summary: Summary? = null,
    val actionItems: List<ActionItem> = emptyList(),
    val decisions: List<Decision> = emptyList(),
    val topics: List<Topic> = emptyList(),
    val questions: List<Question> = emptyList(),
)

data class SearchResult(val meetingId: String, val segmentId: String, val score: Int)
