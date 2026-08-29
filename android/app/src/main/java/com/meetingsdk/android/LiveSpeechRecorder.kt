package com.meetingsdk.android

import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.speech.RecognitionListener
import android.speech.RecognizerIntent
import android.speech.SpeechRecognizer

/**
 * Wraps Android's on-device SpeechRecognizer for one continuous "listening session" made up of
 * possibly many recognized utterances, each a real, live transcription — not a fixture. Requires
 * RECORD_AUDIO already granted and must be driven from the main thread (SpeechRecognizer's own
 * contract).
 *
 * Only uses SpeechRecognizer.createOnDeviceSpeechRecognizer() (API 31+, requires the device to
 * actually have an on-device model installed) — never the general createSpeechRecognizer(), whose
 * EXTRA_PREFER_OFFLINE is only a hint and can silently fall back to network recognition. If
 * on-device recognition genuinely isn't available, [onUnavailable] fires so the UI can say so
 * plainly, rather than this class quietly using the cloud — the same "never silently upgrade"
 * principle translation::GatedTranslator already applies elsewhere in this codebase.
 */
class LiveSpeechRecorder(
    private val context: Context,
    private val onPartial: (String) -> Unit,
    private val onUtterance: (text: String, startMs: Long, endMs: Long) -> Unit,
    private val onUnavailable: () -> Unit,
    private val onError: (String) -> Unit,
) {
    private var recognizer: SpeechRecognizer? = null
    private var utteranceStartMs: Long = 0L
    private var listening = false

    val isListening: Boolean get() = listening

    fun start() {
        if (listening) return
        val onDeviceAvailable =
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && SpeechRecognizer.isOnDeviceRecognitionAvailable(context)
        if (!onDeviceAvailable) {
            onUnavailable()
            return
        }

        val r = SpeechRecognizer.createOnDeviceSpeechRecognizer(context)
        recognizer = r
        r.setRecognitionListener(
            object : RecognitionListener {
                override fun onReadyForSpeech(params: Bundle?) = Unit
                override fun onBeginningOfSpeech() = Unit
                override fun onRmsChanged(rmsdB: Float) = Unit
                override fun onBufferReceived(buffer: ByteArray?) = Unit
                override fun onEndOfSpeech() = Unit
                override fun onEvent(eventType: Int, params: Bundle?) = Unit

                override fun onError(error: Int) {
                    if (!listening) return
                    when (error) {
                        // No speech heard this cycle — keep the session open rather than
                        // treating silence as fatal.
                        SpeechRecognizer.ERROR_NO_MATCH, SpeechRecognizer.ERROR_SPEECH_TIMEOUT -> {
                            utteranceStartMs = System.currentTimeMillis()
                            r.startListening(buildIntent())
                        }
                        else -> {
                            listening = false
                            onError("SpeechRecognizer error code $error")
                        }
                    }
                }

                override fun onResults(results: Bundle?) {
                    val text = results?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)?.firstOrNull()
                    val endMs = System.currentTimeMillis()
                    if (!text.isNullOrBlank()) {
                        onUtterance(text, utteranceStartMs, endMs)
                    }
                    if (listening) {
                        utteranceStartMs = endMs
                        r.startListening(buildIntent())
                    }
                }

                override fun onPartialResults(partialResults: Bundle?) {
                    partialResults?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)?.firstOrNull()
                        ?.let(onPartial)
                }
            },
        )

        listening = true
        utteranceStartMs = System.currentTimeMillis()
        r.startListening(buildIntent())
    }

    fun stop() {
        listening = false
        recognizer?.stopListening()
        recognizer?.destroy()
        recognizer = null
    }

    private fun buildIntent(): Intent =
        Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH).apply {
            putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
            putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true)
        }
}
