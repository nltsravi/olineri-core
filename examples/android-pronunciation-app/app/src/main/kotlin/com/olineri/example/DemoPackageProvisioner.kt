package com.olineri.example

import java.io.File
import java.security.MessageDigest

/**
 * Helper that prepares offline language package directories on local storage
 * so the demo app can demonstrate `runtime.installPackage()`, `runtime.listInstalledPackages()`,
 * `runtime.rollbackPackage()`, and `runtime.uninstallPackage()`.
 *
 * In a production app, language packages (`lang-ta`, `lang-sa`, `lang-hi`, `lang-te`)
 * are either bundled in APK assets/expansion files or downloaded to disk and verified
 * cryptographically via SHA-256 and Ed25519 signatures.
 */
object DemoPackageProvisioner {

    data class LanguageSpec(
        val packageId: String,
        val languageTag: String,
        val script: String,
        val displayName: String,
        val sampleWords: List<Pair<String, String>> // (Native Script, Phonetic Focus Description)
    )

    val SUPPORTED_LANGUAGES = listOf(
        LanguageSpec(
            packageId = "lang-ta",
            languageTag = "ta-IN",
            script = "Taml",
            displayName = "Tamil (தமிழ்)",
            sampleWords = listOf(
                "பழம்" to "pɐɻɐm (fruit) — Retroflex approximant ழ /ɻ/",
                "பலம்" to "pɐlɐm (strength) — Alveolar lateral ல /l/",
                "அறம்" to "ɐrɐm (virtue) — Alveolar trill ற /r/",
                "தமிழ்" to "tɐmɪɻ (Tamil) — Allophonic voicing & retroflex coda"
            )
        ),
        LanguageSpec(
            packageId = "lang-sa",
            languageTag = "sa-IN",
            script = "Deva",
            displayName = "Sanskrit (संस्कृतम्)",
            sampleWords = listOf(
                "नमः" to "nɐmɐhɐ (salutations) — Visarga echo-vowel /hɐ/",
                "रामः" to "rɑːmɐhɐ (Rama) — Strict inherent vowel retention",
                "शान्तिः" to "ɕɑːntɪhɪ (peace) — Palatal sibilant & Visarga /hɪ/"
            )
        ),
        LanguageSpec(
            packageId = "lang-hi",
            languageTag = "hi-IN",
            script = "Deva",
            displayName = "Hindi (हिन्दी)",
            sampleWords = listOf(
                "कमल" to "kəməl (lotus) — Word-final schwa deletion",
                "फल" to "pʰəl (fruit) — Aspirated labial stop /pʰ/",
                "चलना" to "tʃəlnɑː (to walk) — Medial schwa deletion"
            )
        ),
        LanguageSpec(
            packageId = "lang-te",
            languageTag = "te-IN",
            script = "Telu",
            displayName = "Telugu (తెలుగు)",
            sampleWords = listOf(
                "అమ్మ" to "ɐmmɐ (mother) — Consonant gemination (vottulu)",
                "ఎండ" to "eɳɖɐ (sunshine) — Short /e/ & retroflex cluster",
                "తెలుగు" to "telugu (Telugu) — Vowel harmony & open syllables"
            )
        )
    )

    /**
     * Creates a valid language package directory containing `manifest.json` and
     * `model/acoustic.bin` with a matching SHA-256 digest.
     */
    fun preparePackageDirectory(cacheDir: File, spec: LanguageSpec, version: String = "1.0.0"): File {
        val pkgDir = File(cacheDir, "staged_packages/${spec.packageId}_v$version")
        val modelDir = File(pkgDir, "model")
        modelDir.mkdirs()

        val modelFile = File(modelDir, "acoustic.bin")
        val dummyWeights = "olineri_acoustic_model_${spec.packageId}_$version".toByteArray(Charsets.UTF_8)
        modelFile.writeBytes(dummyWeights)

        val sha256Hex = sha256(dummyWeights)

        val manifestJson = """
            {
              "schemaVersion": "1.0.0",
              "packageId": "${spec.packageId}",
              "language": "${spec.languageTag}",
              "script": "${spec.script}",
              "version": "$version",
              "minSdkVersion": "0.1.0",
              "displayName": "${spec.displayName}",
              "publisher": {
                "name": "Olineri Official",
                "publicKeyId": "olineri-release-key-1"
              },
              "capabilities": [
                "speechRecognition",
                "pronunciationAssessment",
                "phonemeFeedback",
                "characterMapping"
              ],
              "model": {
                "id": "indicconformer-ctc-${spec.languageTag}",
                "license": "MIT"
              },
              "artifacts": [
                {
                  "path": "model/acoustic.bin",
                  "sizeBytes": ${dummyWeights.size},
                  "sha256": "$sha256Hex"
                }
              ]
            }
        """.trimIndent()

        File(pkgDir, "manifest.json").writeText(manifestJson, Charsets.UTF_8)
        return pkgDir
    }

    private fun sha256(bytes: ByteArray): String {
        val digest = MessageDigest.getInstance("SHA-256").digest(bytes)
        return digest.joinToString("") { "%02x".format(it) }
    }
}

