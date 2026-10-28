package com.olineri.core

import java.io.Closeable

/**
 * Phoneme assessment classification.
 */
enum class PhonemeStatus(val code: Int) {
    CORRECT(0),
    SUBSTITUTION(1),
    OMISSION(2),
    INSERTION(3),
    UNCERTAIN(4);

    companion object {
        fun fromCode(code: Int): PhonemeStatus =
            values().firstOrNull { it.code == code } ?: UNCERTAIN
    }
}

/**
 * Phoneme-level diagnostic and articulatory feedback.
 */
data class PhonemeFeedback(
    val expectedPhoneme: String,
    val statusCode: Int,
    val score: Float,
    val charStartUtf8: Int,
    val charEndUtf8: Int,
    val detectedPhoneme: String = "",
    val feedbackInstruction: String = ""
) {
    val status: PhonemeStatus get() = PhonemeStatus.fromCode(statusCode)
}

/**
 * Metadata for an installed language package.
 */
data class InstalledPackageInfo(
    val packageId: String,
    val languageTag: String,
    val version: String
)

/**
 * Overall pronunciation assessment output.
 */
data class OlineriAssessmentResult(
    val overallScore: Float,
    val phonemes: List<PhonemeFeedback>,
    val recognizedText: String = ""
) {
    constructor(overallScore: Float, phonemes: List<PhonemeFeedback>) : this(overallScore, phonemes, "")
}

/**
 * Thread-safe active inference session for a specific language.
 */
class OlineriSession internal constructor(
    private val sessionHandle: Long,
    val language: String
) : Closeable {

    private var isClosed = false

    /**
     * Assesses spoken pronunciation offline.
     * @param referenceText Expected Brahmic script text (e.g. "பழம்", "नमः", "कमल", "అమ్మ").
     * @param audioSamples PCM Float audio normalized to [-1.0, 1.0].
     * @param sampleRate Audio sampling rate (16000 Hz recommended).
     */
    fun assess(referenceText: String, audioSamples: FloatArray, sampleRate: Int = 16000): OlineriAssessmentResult {
        check(!isClosed) { "OlineriSession has been closed." }
        return nativeAssessPronunciation(sessionHandle, referenceText, audioSamples, sampleRate)
            ?: throw IllegalStateException("Pronunciation assessment failed in native core.")
    }

    override fun close() {
        if (!isClosed) {
            nativeDestroy(sessionHandle)
            isClosed = true
        }
    }

    private external fun nativeDestroy(handle: Long)
    private external fun nativeAssessPronunciation(
        handle: Long,
        referenceText: String,
        audioSamples: FloatArray,
        sampleRate: Int
    ): OlineriAssessmentResult?
}

/**
 * Master offline runtime managing language packages and inference sessions.
 */
class OlineriRuntime private constructor(
    private val runtimeHandle: Long,
    val storageDir: String
) : Closeable {

    private var isClosed = false

    /**
     * Installs a verified language package from local disk storage.
     * @param packageDirectoryPath Unpacked package directory or archive location.
     * @return Package ID string (e.g. "lang-ta", "lang-sa").
     */
    fun installPackage(packageDirectoryPath: String): String {
        check(!isClosed) { "OlineriRuntime has been closed." }
        return nativeInstallPackage(runtimeHandle, packageDirectoryPath)
            ?: throw IllegalArgumentException("Failed to install package from: $packageDirectoryPath")
    }

    /**
     * Removes an installed language package and frees storage.
     */
    fun uninstallPackage(packageId: String): Boolean {
        check(!isClosed) { "OlineriRuntime has been closed." }
        return nativeUninstallPackage(runtimeHandle, packageId)
    }

    /**
     * Rolls back an updated language package to its previous working version.
     */
    fun rollbackPackage(packageId: String): Boolean {
        check(!isClosed) { "OlineriRuntime has been closed." }
        return nativeRollbackPackage(runtimeHandle, packageId)
    }

    /**
     * Lists all currently installed language packages in the runtime storage.
     */
    fun listInstalledPackages(): List<InstalledPackageInfo> {
        check(!isClosed) { "OlineriRuntime has been closed." }
        return nativeListPackages(runtimeHandle) ?: emptyList()
    }

    /**
     * Registers a trusted Ed25519 public key (hex) for verifying signed language packages.
     */
    fun addTrustedKey(keyId: String, publicKeyHex: String): Boolean {
        check(!isClosed) { "OlineriRuntime has been closed." }
        return nativeAddTrustedKey(runtimeHandle, keyId, publicKeyHex)
    }

    /**
     * Releases idle model weights and cached memory back to the OS (e.g. on ComponentCallbacks2.onTrimMemory).
     */
    fun trimMemory(): Boolean {
        check(!isClosed) { "OlineriRuntime has been closed." }
        return nativeTrimMemory(runtimeHandle)
    }

    /**
     * Creates an assessment session for an installed language.
     * @param languageTag BCP-47 language tag (e.g. "ta-IN", "sa-IN", "hi-IN", "te-IN").
     */
    fun createSession(languageTag: String): OlineriSession {
        check(!isClosed) { "OlineriRuntime has been closed." }
        val sessionHandle = nativeCreateSession(runtimeHandle, languageTag)
        if (sessionHandle == 0L) {
            throw IllegalArgumentException("Cannot create session for language '$languageTag'. Package may not be installed.")
        }
        return OlineriSession(sessionHandle, languageTag)
    }

    override fun close() {
        if (!isClosed) {
            nativeDestroy(runtimeHandle)
            isClosed = true
        }
    }

    private external fun nativeDestroy(handle: Long)
    private external fun nativeInstallPackage(handle: Long, packagePath: String): String?
    private external fun nativeUninstallPackage(handle: Long, packageId: String): Boolean
    private external fun nativeRollbackPackage(handle: Long, packageId: String): Boolean
    private external fun nativeListPackages(handle: Long): List<InstalledPackageInfo>?
    private external fun nativeAddTrustedKey(handle: Long, keyId: String, publicKeyHex: String): Boolean
    private external fun nativeTrimMemory(handle: Long): Boolean
    private external fun nativeCreateSession(handle: Long, languageTag: String): Long

    companion object {
        init {
            try {
                System.loadLibrary("olineri_core")
            } catch (_: UnsatisfiedLinkError) {
                // Static link into olineri_jni when built via bindings/android/CMakeLists.txt
            }
            System.loadLibrary("olineri_jni")
        }

        /**
         * Returns the native Olineri Core SDK version string (e.g. "0.1.0").
         */
        @JvmStatic
        fun version(): String = nativeVersion()

        /**
         * Initializes the offline Olineri SDK runtime.
         */
        @JvmStatic
        fun initialize(storageDirectory: String): OlineriRuntime {
            val handle = nativeCreate(storageDirectory)
            if (handle == 0L) {
                throw IllegalStateException("Failed to initialize Olineri native runtime at: $storageDirectory")
            }
            return OlineriRuntime(handle, storageDirectory)
        }

        @JvmStatic
        private external fun nativeVersion(): String

        @JvmStatic
        private external fun nativeCreate(storageDir: String): Long
    }
}
