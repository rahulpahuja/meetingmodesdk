package com.meetingsdk.api

import com.meetingsdk.model.Meeting

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
    fun getMeeting(id: String): Meeting?
    fun listMeetingIds(): List<String>
    fun removeMeeting(id: String): Boolean
    fun close()
}
