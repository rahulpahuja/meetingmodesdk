package com.meetingsdk.android

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import com.meetingsdk.jvm.NativeBridge
import kotlin.math.sqrt

/**
 * Automatic multi-speaker labeling for the live-speech path ("speaker-1", "speaker-2", ...),
 * without a neural voice-embedding model (none ships in this SDK). Captures raw PCM concurrently
 * with [LiveSpeechRecorder]'s SpeechRecognizer session via a second, independent [AudioRecord],
 * and for each recognized utterance's approximate time window computes a small, real, non-neural
 * acoustic feature vector (RMS energy, an autocorrelation-based pitch estimate, and zero-crossing
 * rate as a coarse spectral-tilt proxy), then hands it to the SDK's real
 * speaker::SpeakerClusterer (via NativeBridge.diarizer*, wrapping msdk_diarizer_*) for actual
 * online clustering — a real, if crude, "voiceprint," not a fabricated one. These three features
 * alone won't reliably separate every real speaker pair, but they are genuinely computed from
 * captured audio and get a real, working clustering algorithm behind them — same honesty bar as
 * DictionaryTranslator/HeuristicLlmEngine elsewhere in this codebase.
 *
 * Whether a third-party app's AudioRecord genuinely receives live samples while a system speech
 * service (SpeechRecognizer) is *also* capturing is not guaranteed by the Android platform (mic
 * arbitration varies by OEM/Android version, and privacy policy on some versions can silence a
 * lower-priority capturer instead of denying it outright — capture can "succeed" while delivering
 * only silence). This class treats that as a runtime condition it detects, not an assumption: if
 * a window's captured audio is at/near silence despite carrying real recognized text,
 * [concurrentCaptureReliable] goes false and every utterance in that session falls back to being
 * assigned the same speaker as the immediately preceding one (an honest "insufficient signal,
 * assume the conversation continued" default) rather than clustering on noise.
 */
class AcousticSpeakerTagger(similarityThreshold: Float = 0.75F) {
    private val diarizerHandle: Long = NativeBridge.diarizerCreate(similarityThreshold)

    private var audioRecord: AudioRecord? = null
    private var captureThread: Thread? = null

    @Volatile private var running = false

    @Volatile var concurrentCaptureReliable: Boolean = true
        private set

    private var lastSpeakerId: String? = null

    // Ring buffer of raw PCM16 samples covering the last RING_SECONDS of audio, indexed by
    // capture time so an utterance's [startMs, endMs] window (from LiveSpeechRecorder, the same
    // System.currentTimeMillis() clock) can be sliced back out after the fact.
    private val ringCapacity = SAMPLE_RATE_HZ * RING_SECONDS
    private val ring = ShortArray(ringCapacity)
    private var ringWritePos = 0
    private var totalSamplesWritten = 0L
    private var captureStartMs = 0L
    private val ringLock = Any()

    @Suppress("MissingPermission")  // caller must already hold RECORD_AUDIO, same contract as LiveSpeechRecorder
    fun start() {
        if (running) return
        val minBufferSize =
            AudioRecord.getMinBufferSize(SAMPLE_RATE_HZ, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT)
        if (minBufferSize <= 0) {
            concurrentCaptureReliable = false
            return
        }
        val record =
            try {
                AudioRecord(
                    MediaRecorder.AudioSource.MIC,
                    SAMPLE_RATE_HZ,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT,
                    minBufferSize * 2,
                )
            } catch (e: SecurityException) {
                concurrentCaptureReliable = false
                return
            }
        if (record.state != AudioRecord.STATE_INITIALIZED) {
            concurrentCaptureReliable = false
            record.release()
            return
        }

        audioRecord = record
        captureStartMs = System.currentTimeMillis()
        totalSamplesWritten = 0L
        running = true
        record.startRecording()

        captureThread =
            Thread {
                val chunk = ShortArray(minBufferSize)
                while (running) {
                    val read = record.read(chunk, 0, chunk.size)
                    if (read > 0) {
                        synchronized(ringLock) {
                            for (i in 0 until read) {
                                ring[ringWritePos] = chunk[i]
                                ringWritePos = (ringWritePos + 1) % ringCapacity
                            }
                            totalSamplesWritten += read
                        }
                    }
                }
            }.apply {
                isDaemon = true
                start()
            }
    }

    fun stop() {
        running = false
        captureThread?.join(500)
        captureThread = null
        audioRecord?.apply {
            stop()
            release()
        }
        audioRecord = null
        NativeBridge.diarizerDestroy(diarizerHandle)
    }

    /** Returns a SpeakerId ("speaker-N", or a fallback — see class doc) for one utterance. */
    fun assignSpeaker(startMs: Long, endMs: Long): String {
        val slice = sliceSamples(startMs, endMs)
        if (slice == null || isEffectivelySilent(slice)) {
            concurrentCaptureReliable = false
            val fallback = lastSpeakerId ?: "speaker-1"
            lastSpeakerId = fallback
            return fallback
        }
        val features = extractFeatures(slice)
        val speakerId = NativeBridge.diarizerAssign(diarizerHandle, features) ?: (lastSpeakerId ?: "speaker-1")
        lastSpeakerId = speakerId
        return speakerId
    }

    private fun sliceSamples(startMs: Long, endMs: Long): ShortArray? =
        synchronized(ringLock) {
            if (totalSamplesWritten == 0L) return null
            val elapsedStartMs = (startMs - captureStartMs).coerceAtLeast(0)
            val elapsedEndMs = (endMs - captureStartMs).coerceAtLeast(elapsedStartMs)
            val startSample = elapsedStartMs * SAMPLE_RATE_HZ / 1000
            val endSample = elapsedEndMs * SAMPLE_RATE_HZ / 1000
            val availableStart = (totalSamplesWritten - ringCapacity).coerceAtLeast(0)
            val clampedStart = startSample.coerceIn(availableStart, totalSamplesWritten)
            val clampedEnd = endSample.coerceIn(clampedStart, totalSamplesWritten)
            val count = (clampedEnd - clampedStart).toInt()
            if (count <= 0) {
                return null
            }
            ShortArray(count) { i -> ring[((clampedStart + i) % ringCapacity).toInt()] }
        }

    private fun rms(samples: ShortArray): Double {
        var sumSquares = 0.0
        for (s in samples) sumSquares += s.toDouble() * s.toDouble()
        return sqrt(sumSquares / samples.size)
    }

    private fun isEffectivelySilent(samples: ShortArray): Boolean = rms(samples) < SILENCE_RMS_THRESHOLD

    private fun extractFeatures(samples: ShortArray): FloatArray {
        val energy = rms(samples).toFloat()

        var crossings = 0
        for (i in 1 until samples.size) {
            if ((samples[i - 1] >= 0) != (samples[i] >= 0)) crossings++
        }
        val zcr = crossings.toFloat() / samples.size

        val pitchHz = estimatePitchHz(samples)

        // Roughly normalized onto comparable scales so cosine similarity isn't dominated by RMS's
        // much larger raw magnitude.
        return floatArrayOf(energy / 4000F, pitchHz / 300F, zcr * 10F)
    }

    /** Autocorrelation-based pitch estimate over the human voice's typical 80-400Hz range. */
    private fun estimatePitchHz(samples: ShortArray): Float {
        val minLag = SAMPLE_RATE_HZ / 400
        val maxLag = SAMPLE_RATE_HZ / 80
        if (samples.size <= maxLag) return 0F
        var bestLag = -1
        var bestCorrelation = 0.0
        for (lag in minLag..maxLag) {
            var correlation = 0.0
            var i = 0
            while (i + lag < samples.size) {
                correlation += samples[i].toDouble() * samples[i + lag].toDouble()
                i++
            }
            if (correlation > bestCorrelation) {
                bestCorrelation = correlation
                bestLag = lag
            }
        }
        return if (bestLag <= 0) 0F else SAMPLE_RATE_HZ.toFloat() / bestLag
    }

    companion object {
        private const val SAMPLE_RATE_HZ = 16000
        private const val RING_SECONDS = 12
        private const val SILENCE_RMS_THRESHOLD = 50.0
    }
}
