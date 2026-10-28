package com.olineri.example

import android.Manifest
import android.content.ComponentCallbacks2
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Bundle
import android.text.SpannableString
import android.text.Spanned
import android.text.style.BackgroundColorSpan
import android.text.style.ForegroundColorSpan
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.Spinner
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.olineri.core.OlineriAssessmentResult
import com.olineri.core.OlineriRuntime
import com.olineri.core.OlineriSession
import com.olineri.core.PhonemeStatus
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.Executors

/**
 * Interactive reference Activity demonstrating every function in the Olineri Android SDK:
 *
 * 1. `OlineriRuntime.version()` & `OlineriRuntime.initialize(storageDir)`
 * 2. `runtime.addTrustedKey(keyId, publicKeyHex)`
 * 3. `runtime.installPackage(path)`, `runtime.listInstalledPackages()`,
 *    `runtime.rollbackPackage(id)`, `runtime.uninstallPackage(id)`
 * 4. `runtime.createSession(languageTag)` & `session.close()`
 * 5. `session.assess(referenceText, audioSamples, sampleRate)`
 * 6. Mapping UTF-8 byte offsets (`charStartUtf8`..`charEndUtf8`) to Android `SpannableString`
 *    indices for Brahmic script character highlighting
 * 7. `runtime.trimMemory()` & `runtime.close()`
 */
class MainActivity : AppCompatActivity() {

    private var runtime: OlineriRuntime? = null
    private var activeSession: OlineriSession? = null
    private var selectedLangSpec = DemoPackageProvisioner.SUPPORTED_LANGUAGES.first()

    private val backgroundExecutor = Executors.newSingleThreadExecutor()

    // UI References
    private lateinit var tvSdkStatus: TextView
    private lateinit var spinnerLanguage: Spinner
    private lateinit var spinnerSampleWord: Spinner
    private lateinit var etReferenceText: EditText
    private lateinit var tvHighlightedWord: TextView
    private lateinit var tvOverallScore: TextView
    private lateinit var layoutPhonemeCards: LinearLayout
    private lateinit var tvConsoleLog: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        bindViews()
        setupSpinners()
        setupClickListeners()

        // Automatically initialize the runtime on startup so the user can explore immediately
        initSdkRuntime()
    }

    private fun bindViews() {
        tvSdkStatus = findViewById(R.id.tvSdkStatus)
        spinnerLanguage = findViewById(R.id.spinnerLanguage)
        spinnerSampleWord = findViewById(R.id.spinnerSampleWord)
        etReferenceText = findViewById(R.id.etReferenceText)
        tvHighlightedWord = findViewById(R.id.tvHighlightedWord)
        tvOverallScore = findViewById(R.id.tvOverallScore)
        layoutPhonemeCards = findViewById(R.id.layoutPhonemeCards)
        tvConsoleLog = findViewById(R.id.tvConsoleLog)
    }

    private fun setupSpinners() {
        val langNames = DemoPackageProvisioner.SUPPORTED_LANGUAGES.map {
            "${it.displayName} [${it.languageTag}]"
        }
        spinnerLanguage.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_dropdown_item,
            langNames
        )

        spinnerLanguage.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, pos: Int, id: Long) {
                selectedLangSpec = DemoPackageProvisioner.SUPPORTED_LANGUAGES[pos]
                updateSampleWordSpinner()
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
        updateSampleWordSpinner()
    }

    private fun updateSampleWordSpinner() {
        val wordLabels = selectedLangSpec.sampleWords.map { "${it.first} — ${it.second}" }
        spinnerSampleWord.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_dropdown_item,
            wordLabels
        )
        spinnerSampleWord.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, pos: Int, id: Long) {
                val word = selectedLangSpec.sampleWords[pos].first
                etReferenceText.setText(word)
                tvHighlightedWord.text = word
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
    }

    private fun setupClickListeners() {
        // Step 1: Initialize Runtime & Add Trusted Key
        findViewById<Button>(R.id.btnInitRuntime).setOnClickListener {
            initSdkRuntime()
        }

        // Step 2: Install Language Package
        findViewById<Button>(R.id.btnInstallPackage).setOnClickListener {
            installSelectedLanguagePackage()
        }

        // Step 2b: List Installed Packages
        findViewById<Button>(R.id.btnListPackages).setOnClickListener {
            listPackages()
        }

        // Step 2c: Uninstall Selected Package
        findViewById<Button>(R.id.btnUninstallPackage).setOnClickListener {
            uninstallSelectedLanguagePackage()
        }

        // Step 3: Create Language Session
        findViewById<Button>(R.id.btnCreateSession).setOnClickListener {
            createLanguageSession()
        }

        // Step 4a: Assess with Synthesized Test Audio (works on Emulator & Device)
        findViewById<Button>(R.id.btnAssessTestAudio).setOnClickListener {
            val samples = AudioCaptureHelper.generateTestWaveform(durationMs = 1000)
            logCall("AudioCaptureHelper.generateTestWaveform(1000ms) -> FloatArray[${samples.size}]")
            runPronunciationAssessment(samples)
        }

        // Step 4b: Record Live Microphone & Assess
        findViewById<Button>(R.id.btnRecordMic).setOnClickListener {
            recordAndAssessFromMic()
        }

        // Step 5: Trim Memory & Teardown
        findViewById<Button>(R.id.btnTrimMemory).setOnClickListener {
            trimSdkMemory()
        }
    }

    // =========================================================================
    // SDK Function 1: OlineriRuntime.version() & OlineriRuntime.initialize()
    // =========================================================================
    private fun initSdkRuntime() {
        try {
            activeSession?.close()
            activeSession = null
            runtime?.close()

            val sdkVersion = OlineriRuntime.version()
            val storagePath = File(filesDir, "olineri_storage").absolutePath
            val rt = OlineriRuntime.initialize(storagePath)
            runtime = rt

            // Register trusted Ed25519 publisher key for package verification
            val keyAdded = rt.addTrustedKey(
                keyId = "olineri-release-key-1",
                publicKeyHex = "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"
            )

            updateStatusHeader()
            logCall(
                "OlineriRuntime.version() -> \"$sdkVersion\"\n" +
                "OlineriRuntime.initialize(\"$storagePath\") -> OK\n" +
                "runtime.addTrustedKey(\"olineri-release-key-1\", ...) -> $keyAdded"
            )
        } catch (e: Exception) {
            logCall("ERROR in initialize: ${e.message}")
        }
    }

    // =========================================================================
    // SDK Function 2: Package Management (install, list, uninstall)
    // =========================================================================
    private fun installSelectedLanguagePackage() {
        val rt = runtime ?: run {
            logCall("Initialize runtime first!")
            return
        }
        try {
            // Prepare a local verified package directory in cacheDir
            val stagedDir = DemoPackageProvisioner.preparePackageDirectory(
                cacheDir = cacheDir,
                spec = selectedLangSpec,
                version = "1.0.0"
            )
            val installedPkgId = rt.installPackage(stagedDir.absolutePath)
            logCall(
                "runtime.installPackage(\"${stagedDir.name}\")\n" +
                "  -> Verified SHA-256 & installed packageId = \"$installedPkgId\""
            )
            updateStatusHeader()
        } catch (e: Exception) {
            logCall("ERROR installing package: ${e.message}")
        }
    }

    private fun listPackages() {
        val rt = runtime ?: run {
            logCall("Initialize runtime first!")
            return
        }
        val installed = rt.listInstalledPackages()
        if (installed.isEmpty()) {
            logCall("runtime.listInstalledPackages() -> [] (0 packages installed)")
        } else {
            val summary = installed.joinToString(", ") { "${it.packageId} (${it.languageTag} v${it.version})" }
            logCall("runtime.listInstalledPackages() -> [$summary]")
        }
        updateStatusHeader()
    }

    private fun uninstallSelectedLanguagePackage() {
        val rt = runtime ?: run {
            logCall("Initialize runtime first!")
            return
        }
        // Close active session if it uses this language
        if (activeSession?.language == selectedLangSpec.languageTag) {
            activeSession?.close()
            activeSession = null
            logCall("session.close() -> Closed active ${selectedLangSpec.languageTag} session before uninstall")
        }

        val removed = rt.uninstallPackage(selectedLangSpec.packageId)
        logCall("runtime.uninstallPackage(\"${selectedLangSpec.packageId}\") -> $removed")
        updateStatusHeader()
    }

    // =========================================================================
    // SDK Function 3: Session Lifecycle (createSession & close)
    // =========================================================================
    private fun createLanguageSession() {
        val rt = runtime ?: run {
            logCall("Initialize runtime first!")
            return
        }
        try {
            // Ensure package is installed first for smooth UX
            val installed = rt.listInstalledPackages().any { it.languageTag == selectedLangSpec.languageTag }
            if (!installed) {
                installSelectedLanguagePackage()
            }

            activeSession?.close()
            val session = rt.createSession(selectedLangSpec.languageTag)
            activeSession = session
            logCall("runtime.createSession(\"${selectedLangSpec.languageTag}\") -> Active OlineriSession ready")
            updateStatusHeader()
        } catch (e: Exception) {
            logCall("ERROR creating session: ${e.message}")
        }
    }

    // =========================================================================
    // SDK Function 4: Pronunciation Assessment & Character Mapping
    // =========================================================================
    private fun runPronunciationAssessment(audioSamples: FloatArray) {
        if (activeSession == null || activeSession?.language != selectedLangSpec.languageTag) {
            createLanguageSession()
        }
        val session = activeSession ?: return
        val referenceText = etReferenceText.text.toString().trim()
        if (referenceText.isEmpty()) {
            logCall("Please enter reference text in ${selectedLangSpec.displayName}")
            return
        }

        try {
            val startNs = System.nanoTime()
            val result: OlineriAssessmentResult = session.assess(
                referenceText = referenceText,
                audioSamples = audioSamples,
                sampleRate = AudioCaptureHelper.SAMPLE_RATE_HZ
            )
            val elapsedMs = (System.nanoTime() - startNs) / 1_000_000.0

            logCall(
                "session.assess(referenceText=\"$referenceText\", samples=${audioSamples.size}, rate=16000)\n" +
                "  -> Completed in ${"%.2f".format(elapsedMs)} ms | Overall GOP Score: ${"%.1f".format(result.overallScore)}/100 " +
                "| Phonemes: ${result.phonemes.size}"
            )

            renderAssessmentResult(referenceText, result)
        } catch (e: Exception) {
            logCall("ERROR in session.assess(): ${e.message}")
        }
    }

    private fun recordAndAssessFromMic() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
            != PackageManager.PERMISSION_GRANTED
        ) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), 1001)
            logCall("Requesting android.permission.RECORD_AUDIO...")
            return
        }

        logCall("Recording 1.5s of 16kHz Float32 PCM audio from microphone...")
        backgroundExecutor.execute {
            val recorded = AudioCaptureHelper.recordMicrophoneClip(durationMs = 1500)
            runOnUiThread {
                logCall("Captured ${recorded.size} samples @ 16kHz. Running session.assess()...")
                runPronunciationAssessment(recorded)
            }
        }
    }

    /**
     * Renders both the character-highlighted word (using `charStartUtf8` and `charEndUtf8`)
     * and individual phoneme feedback cards.
     */
    private fun renderAssessmentResult(referenceText: String, result: OlineriAssessmentResult) {
        tvOverallScore.text = String.format(
            Locale.US,
            "Overall Score: %.1f / 100 (%d phonemes)",
            result.overallScore,
            result.phonemes.size
        )

        // 1. Highlight characters in the reference string using UTF-8 byte offsets
        val spannable = SpannableString(referenceText)
        for (ph in result.phonemes) {
            val startChar = utf8ByteOffsetToCharIndex(referenceText, ph.charStartUtf8)
            val endChar = utf8ByteOffsetToCharIndex(referenceText, ph.charEndUtf8)
            if (startChar in 0 until endChar && endChar <= referenceText.length) {
                val color = when (ph.status) {
                    PhonemeStatus.CORRECT -> Color.parseColor("#1B5E20")      // Deep Green
                    PhonemeStatus.SUBSTITUTION -> Color.parseColor("#E65100") // Deep Orange
                    PhonemeStatus.OMISSION, PhonemeStatus.INSERTION -> Color.parseColor("#B71C1C") // Red
                    PhonemeStatus.UNCERTAIN -> Color.parseColor("#455A64")    // Slate
                }
                val bgColor = when (ph.status) {
                    PhonemeStatus.CORRECT -> Color.parseColor("#E8F5E9")
                    PhonemeStatus.SUBSTITUTION -> Color.parseColor("#FFF3E0")
                    PhonemeStatus.OMISSION, PhonemeStatus.INSERTION -> Color.parseColor("#FFEBEE")
                    PhonemeStatus.UNCERTAIN -> Color.parseColor("#ECEFF1")
                }
                spannable.setSpan(ForegroundColorSpan(color), startChar, endChar, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
                spannable.setSpan(BackgroundColorSpan(bgColor), startChar, endChar, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
            }
        }
        tvHighlightedWord.text = spannable

        // 2. Populate Phoneme Breakdown Cards
        layoutPhonemeCards.removeAllViews()
        for ((index, ph) in result.phonemes.withIndex()) {
            val startChar = utf8ByteOffsetToCharIndex(referenceText, ph.charStartUtf8)
            val endChar = utf8ByteOffsetToCharIndex(referenceText, ph.charEndUtf8)
            val graphemeSlice = if (startChar in 0 until endChar && endChar <= referenceText.length) {
                referenceText.substring(startChar, endChar)
            } else {
                "?"
            }

            val cardView = TextView(this).apply {
                setPadding(24, 16, 24, 16)
                textSize = 13f
                val advice = if (ph.feedbackInstruction.isNotBlank()) "\n   Tip: ${ph.feedbackInstruction}" else ""
                text = String.format(
                    Locale.US,
                    "#%d  Grapheme \"%s\" [UTF-8 bytes %d..%d] → Target IPA: %s | %s (%.1f%%)%s",
                    index + 1,
                    graphemeSlice,
                    ph.charStartUtf8,
                    ph.charEndUtf8,
                    ph.expectedPhoneme,
                    ph.status.name,
                    ph.score,
                    advice
                )
            }
            layoutPhonemeCards.addView(cardView)
        }
    }

    /**
     * Converts a UTF-8 byte offset (`charStartUtf8` / `charEndUtf8` from Olineri C ABI)
     * into a UTF-16 character index for Android `String` / `SpannableString`.
     */
    private fun utf8ByteOffsetToCharIndex(text: String, targetUtf8ByteOffset: Int): Int {
        var byteCount = 0
        var charIndex = 0
        while (charIndex < text.length && byteCount < targetUtf8ByteOffset) {
            val codePoint = text.codePointAt(charIndex)
            byteCount += when {
                codePoint <= 0x7F -> 1
                codePoint <= 0x7FF -> 2
                codePoint <= 0xFFFF -> 3
                else -> 4
            }
            charIndex += Character.charCount(codePoint)
        }
        return charIndex
    }

    // =========================================================================
    // SDK Function 5: Low-Memory Handling & Cleanup (trimMemory & close)
    // =========================================================================
    private fun trimSdkMemory() {
        val rt = runtime ?: return
        val ok = rt.trimMemory()
        logCall("runtime.trimMemory() -> $ok (Unloaded idle acoustic weights from RAM)")
    }

    override fun onTrimMemory(level: Int) {
        super.onTrimMemory(level)
        if (level >= ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW) {
            runtime?.trimMemory()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        backgroundExecutor.shutdownNow()
        activeSession?.close()
        runtime?.close()
    }

    private fun updateStatusHeader() {
        val rt = runtime
        if (rt == null) {
            tvSdkStatus.text = "SDK Status: Uninitialized"
            return
        }
        val pkgs = rt.listInstalledPackages()
        val sessionLang = activeSession?.language ?: "None"
        tvSdkStatus.text = "Olineri v${OlineriRuntime.version()} (100% Offline) | " +
            "Installed Packages: ${pkgs.size} | Active Session: $sessionLang"
    }

    private fun logCall(message: String) {
        val timestamp = SimpleDateFormat("HH:mm:ss.SSS", Locale.US).format(Date())
        val entry = "[$timestamp] $message\n\n"
        tvConsoleLog.text = entry + tvConsoleLog.text.toString()
    }
}

