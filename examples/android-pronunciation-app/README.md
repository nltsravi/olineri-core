# Olineri Android Sample App (`examples/android-pronunciation-app`)

This sample Android application demonstrates every function of the **Olineri Offline Pronunciation Assessment SDK** ([`OlineriSDK.kt`](../../bindings/android/src/main/kotlin/com/olineri/core/OlineriSDK.kt) and [`olineri_jni.cpp`](../../bindings/android/src/main/cpp/olineri_jni.cpp)).

## Project Structure

- [`settings.gradle.kts`](settings.gradle.kts) — Links `:app` directly to `:olineri-android` (`../../bindings/android`).
- [`app/src/main/AndroidManifest.xml`](app/src/main/AndroidManifest.xml) — Requests only `android.permission.RECORD_AUDIO` (zero `INTERNET` permission, proving 100% offline execution).
- [`MainActivity.kt`](app/src/main/kotlin/com/olineri/example/MainActivity.kt) — Interactive screen walking through all SDK lifecycle, package management, session, and assessment APIs.
- [`DemoPackageProvisioner.kt`](app/src/main/kotlin/com/olineri/example/DemoPackageProvisioner.kt) — Stages verified language packages (`lang-ta`, `lang-sa`, `lang-hi`, `lang-te`) with SHA-256 `manifest.json` and model artifacts.
- [`AudioCaptureHelper.kt`](app/src/main/kotlin/com/olineri/example/AudioCaptureHelper.kt) — Captures 16 kHz mono `ENCODING_PCM_FLOAT` (`FloatArray`) from `AudioRecord` or synthesizes a test waveform for emulator testing.

## SDK Functions Demonstrated

| Step | SDK Function | Where in Sample App | Purpose |
| :--- | :--- | :--- | :--- |
| **1. Version & Init** | `OlineriRuntime.version()`<br>`OlineriRuntime.initialize(storageDir)` | `MainActivity.initSdkRuntime()` | Queries native C++17 engine version and initializes app-private storage. |
| **2. Security Key** | `runtime.addTrustedKey(keyId, publicKeyHex)` | `MainActivity.initSdkRuntime()` | Registers an Ed25519 publisher public key used to verify signed language packages. |
| **3. Package Lifecycle** | `runtime.installPackage(path)`<br>`runtime.listInstalledPackages()`<br>`runtime.rollbackPackage(pkgId)`<br>`runtime.uninstallPackage(pkgId)` | `MainActivity.installSelectedLanguagePackage()`<br>`MainActivity.listPackages()`<br>`MainActivity.uninstallSelectedLanguagePackage()` | Verifies SHA-256 artifact hashes and installs/lists/removes language models atomically. |
| **4. Session Management** | `runtime.createSession(languageTag)`<br>`session.close()` | `MainActivity.createLanguageSession()` | Loads the G2P rules (`ta-IN`, `sa-IN`, `hi-IN`, `te-IN`) and acoustic model for inference. |
| **5. Assessment** | `session.assess(referenceText, audioSamples, 16000)` | `MainActivity.runPronunciationAssessment()` | Runs Brahmic G2P + Constrained CTC Viterbi alignment + Goodness of Pronunciation (GOP) scoring. |
| **6. Character Mapping** | `PhonemeFeedback.charStartUtf8` / `charEndUtf8` | `MainActivity.renderAssessmentResult()`<br>`MainActivity.utf8ByteOffsetToCharIndex()` | Maps UTF-8 byte spans from the C ABI to UTF-16 `SpannableString` indices to color-code exact Tamil/Devanagari/Telugu characters. |
| **7. Memory & Teardown** | `runtime.trimMemory()`<br>`runtime.close()` | `MainActivity.onTrimMemory()`<br>`MainActivity.onDestroy()` | Releases idle model weights on Android low-memory callbacks and cleans up native handles on exit. |

## Opening in Android Studio

1. Open Android Studio and select **Open**.
2. Choose the `examples/android-pronunciation-app` directory.
3. Ensure Android SDK 34 and NDK + CMake 3.22.1+ are installed in SDK Manager.
4. Run the `app` configuration on an emulator (use **Assess (Test Audio)**) or physical Android device (use **Record Mic (1.5s)**).

