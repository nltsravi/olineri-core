package com.vaani.reactnative

import com.facebook.react.bridge.*
import com.vaani.core.VaaniRuntime
import com.vaani.core.VaaniSession
import java.io.File

class VaaniModule(reactContext: ReactApplicationContext) : ReactContextBaseJavaModule(reactContext) {

    private var runtime: VaaniRuntime? = null
    private val activeSessions = mutableMapOf<String, VaaniSession>()

    override fun getName(): String = "VaaniNativeBridge"

    @ReactMethod
    fun initializeRuntime(config: ReadableMap, promise: Promise) {
        try {
            val storageDir = if (config.hasKey("storageDirectory")) {
                config.getString("storageDirectory") ?: reactApplicationContext.filesDir.absolutePath
            } else {
                File(reactApplicationContext.filesDir, "vaani_models").apply { mkdirs() }.absolutePath
            }
            runtime = VaaniRuntime.initialize(storageDir)
            promise.resolve(true)
        } catch (e: Exception) {
            promise.reject("INIT_ERROR", e.message, e)
        }
    }

    @ReactMethod
    fun installPackage(languageTag: String, options: ReadableMap, promise: Promise) {
        val rt = runtime ?: return promise.reject("NOT_INITIALIZED", "Runtime is not initialized")
        try {
            val source = options.getString("source") ?: "local"
            if (source == "local") {
                val localPath = options.getString("localPath")
                    ?: return promise.reject("INVALID_ARGS", "localPath is required for local source")
                val pkgId = rt.installPackage(localPath)
                promise.resolve(pkgId)
            } else {
                promise.reject("CATALOG_UNAVAILABLE", "Remote catalog download must be handled via explicit developer endpoint.")
            }
        } catch (e: Exception) {
            promise.reject("INSTALL_ERROR", e.message, e)
        }
    }

    @ReactMethod
    fun assessPronunciation(request: ReadableMap, promise: Promise) {
        val rt = runtime ?: return promise.reject("NOT_INITIALIZED", "Runtime is not initialized")
        try {
            val language = request.getString("language")
                ?: return promise.reject("INVALID_ARGS", "language is required")
            val text = request.getString("referenceText")
                ?: return promise.reject("INVALID_ARGS", "referenceText is required")
            val audioMap = request.getMap("audio")
                ?: return promise.reject("INVALID_ARGS", "audio object is required")

            val audioType = audioMap.getString("type") ?: "pcmFloatArray"
            val sampleRate = if (audioMap.hasKey("sampleRate")) audioMap.getInt("sampleRate") else 16000

            val floatArray: FloatArray = if (audioType == "pcmFloatArray") {
                val samplesArr = audioMap.getArray("samples")
                    ?: return promise.reject("INVALID_ARGS", "samples array missing")
                FloatArray(samplesArr.size()) { i -> samplesArr.getDouble(i).toFloat() }
            } else {
                return promise.reject("UNSUPPORTED_AUDIO", "Only pcmFloatArray is currently supported in bridge")
            }

            // Get or create session
            val session = activeSessions.getOrPut(language) {
                rt.createSession(language)
            }

            val result = session.assess(text, floatArray, sampleRate)

            val resultMap = Arguments.createMap().apply {
                putString("language", language)
                putString("referenceText", text)
                putDouble("overallScore", result.overallScore.toDouble())
                putString("status", "success")

                val phonemesArr = Arguments.createArray()
                for (ph in result.phonemes) {
                    val phMap = Arguments.createMap().apply {
                        putString("expectedPhoneme", ph.expectedPhoneme)
                        putString("status", ph.status.name.lowercase())
                        putDouble("score", ph.score.toDouble())
                        putInt("charStartUtf8", ph.charStartUtf8)
                        putInt("charEndUtf8", ph.charEndUtf8)
                    }
                    phonemesArr.pushMap(phMap)
                }
                putArray("phonemes", phonemesArr)
            }

            promise.resolve(resultMap)
        } catch (e: Exception) {
            promise.reject("ASSESS_ERROR", e.message, e)
        }
    }

    @ReactMethod
    fun shutdownRuntime(promise: Promise) {
        try {
            activeSessions.values.forEach { it.close() }
            activeSessions.clear()
            runtime?.close()
            runtime = null
            promise.resolve(true)
        } catch (e: Exception) {
            promise.reject("SHUTDOWN_ERROR", e.message, e)
        }
    }
}
