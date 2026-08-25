package com.meetingsdk.model

/**
 * Platform-neutral domain models exposed by the KMP API — no native types (pointers, JNI
 * handles) ever appear here; native bindings (jvmMain now, androidMain/iosMain later)
 * translate at the boundary. Mirrors a subset of meeting_sdk::core (see
 * ../../../../../../docs/architecture/02-interfaces-and-data-models.md): only the fields the
 * C API (bindings/c) actually exposes today. Speakers, summaries, action items, decisions,
 * topics, and questions are not wired through the C API yet — they extend this model the same
 * way transcript did, not by changing its shape.
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

data class Meeting(
    val id: String,
    val state: MeetingState,
    val transcript: List<TranscriptSegment>,
)
