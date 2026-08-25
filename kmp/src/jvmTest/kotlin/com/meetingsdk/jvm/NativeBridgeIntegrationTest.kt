package com.meetingsdk.jvm

import com.meetingsdk.model.MeetingState
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue

// End-to-end proof of the native bindings: Kotlin -> JNI (bindings/kotlin/jni) -> C API
// (bindings/c) -> C++ core (meeting-sdk). Exhaustive behavioral testing of the underlying
// C++ classes already lives in meeting-sdk's own GoogleTest suite; this only proves the
// cross-language boundary works.
class NativeBridgeIntegrationTest {
    @Test
    fun translateRoundTripsThroughNativeCode() {
        val service = JvmTranslationService()
        val result = service.translate("namaste, kaise hain aap", "en")
        assertTrue(result.isSuccess)
        assertEquals("hello, how hain you", result.getOrThrow())
    }

    @Test
    fun translateOfUnsupportedLanguageFails() {
        val service = JvmTranslationService()
        assertTrue(service.translate("hello", "fr").isFailure)
    }

    @Test
    fun repositorySavesAndReadsBackAMeeting() {
        val repo = JvmMeetingRepository(":memory:")
        try {
            assertTrue(repo.saveSimpleMeeting("m1", "hello from the JVM"))

            val meeting = repo.getMeeting("m1")
            assertEquals("m1", meeting?.id)
            assertEquals(MeetingState.COMPLETED, meeting?.state)
            assertEquals(1, meeting?.transcript?.size)
            assertEquals("hello from the JVM", meeting?.transcript?.get(0)?.text)
        } finally {
            repo.close()
        }
    }

    @Test
    fun repositoryListsAndRemovesMeetings() {
        val repo = JvmMeetingRepository(":memory:")
        try {
            repo.saveSimpleMeeting("m1", "first")
            repo.saveSimpleMeeting("m2", "second")
            assertEquals(setOf("m1", "m2"), repo.listMeetingIds().toSet())

            assertTrue(repo.removeMeeting("m1"))
            assertNull(repo.getMeeting("m1"))
            assertEquals(listOf("m2"), repo.listMeetingIds())
        } finally {
            repo.close()
        }
    }

    @Test
    fun getMeetingOnMissingIdReturnsNull() {
        val repo = JvmMeetingRepository(":memory:")
        try {
            assertNull(repo.getMeeting("nonexistent"))
        } finally {
            repo.close()
        }
    }
}
