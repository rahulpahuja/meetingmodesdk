package com.meetingsdk.api

import com.meetingsdk.model.Meeting
import com.meetingsdk.model.SearchResult

/**
 * On-device translation. Cloud-mode gating happens in the native core (GatedTranslator) —
 * this interface only ever reaches an already-gated translator, never a raw provider.
 */
interface TranslationService {
    fun translate(text: String, targetBcp47: String): Result<String>
}

/**
 * Encrypted meeting persistence. Not thread-safe: callers serialize access to one instance
 * (matches the storage-thread contract in docs/architecture/
 * 02-interfaces-and-data-models.md §3.5).
 */
interface MeetingRepository {
    fun saveSimpleMeeting(id: String, text: String): Boolean

    /**
     * Runs meeting_sdk::orchestration::MeetingPipeline end-to-end over a short, deterministic
     * synthetic utterance and persists the result — see msdk_repository_run_demo_meeting's doc
     * comment in bindings/c/include/meeting_sdk_c/meeting_sdk.h for exactly what is and isn't
     * real about this (STT/diarization are demo fakes; language detection and intelligence
     * extraction are the SDK's real heuristic implementations).
     */
    fun runDemoMeeting(id: String): Boolean
    fun getMeeting(id: String): Meeting?
    fun listMeetingIds(): List<String>
    fun removeMeeting(id: String): Boolean
    fun close()
}

/** Keyword search (meeting_sdk::search::InvertedIndexSearch) over every saved meeting. */
interface SearchService {
    fun search(queryText: String): List<SearchResult>
}
