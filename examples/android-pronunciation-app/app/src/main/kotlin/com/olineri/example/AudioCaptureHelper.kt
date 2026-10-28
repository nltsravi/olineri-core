package com.olineri.example

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import kotlin.math.PI
import kotlin.math.sin

/**
 * Demonstrates how to capture or synthesize 16 kHz mono Float32 PCM audio
 * (`FloatArray` normalized to `[-1.0f, 1.0f]`) required by `OlineriSession.assess()`.
 */
object AudioCaptureHelper {

    const val SAMPLE_RATE_HZ = 16000

    /**
     * Records live microphone audio into a 16 kHz mono FloatArray.
     * Must be called from a background thread while `RECORD_AUDIO` permission is granted.
     */
    @SuppressLint("MissingPermission")
    fun recordMicrophoneClip(durationMs: Int = 1500): FloatArray {
        val totalSamples = (SAMPLE_RATE_HZ * durationMs) / 1000
        val minBufBytes = AudioRecord.getMinBufferSize(
            SAMPLE_RATE_HZ,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_FLOAT
        ).coerceAtLeast(totalSamples * 4)

        val recorder = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            SAMPLE_RATE_HZ,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_FLOAT,
            minBufBytes
        )

        val buffer = FloatArray(totalSamples)
        try {
            recorder.startRecording()
            var offset = 0
            while (offset < totalSamples) {
                val read = recorder.read(
                    buffer,
                    offset,
                    totalSamples - offset,
                    AudioRecord.READ_BLOCKING
                )
                if (read <= 0) break
                offset += read
            }
        } finally {
            try {
                recorder.stop()
            } catch (_: Exception) {
            }
            recorder.release()
        }
        return buffer
    }

    /**
     * Generates a clean 1.0-second 16 kHz mono Float32 test waveform (`[-0.5, 0.5]`)
     * so developers can test the full SDK pipeline on emulators without a microphone.
     */
    fun generateTestWaveform(durationMs: Int = 1000): FloatArray {
        val sampleCount = (SAMPLE_RATE_HZ * durationMs) / 1000
        return FloatArray(sampleCount) { i ->
            val t = i.toFloat() / SAMPLE_RATE_HZ.toFloat()
            (0.45 * sin(2.0 * PI * 220.0 * t) + 0.15 * sin(2.0 * PI * 660.0 * t)).toFloat()
        }
    }
}

