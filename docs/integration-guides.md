# Hybrid & Native Mobile Integration Guides

This guide covers integrating the offline **Olineri Pronunciation SDK** (`olineri-core`) into:
1. [Android (Kotlin / JNI)](#1-android-integration-kotlin--jni)
2. [iOS (Swift / Swift Package Manager)](#2-ios-integration-swift--spm)
3. [React Native (TypeScript)](#3-react-native-integration)
4. [Flutter (Dart / FFI)](#4-flutter-integration)
5. [Capacitor / Ionic](#5-capacitor--ionic-integration)

---

## 1. Android Integration (Kotlin / JNI)

### 1.1 Dependency Setup
In your app module's `build.gradle.kts`:

```kotlin
dependencies {
    implementation("com.olineri.core:olineri-android:0.1.0")
}
```

### 1.2 SDK Usage

```kotlin
import com.olineri.core.OlineriRuntime
import com.olineri.core.PhonemeStatus

// 1. Initialize Runtime with app-private storage
val runtime = OlineriRuntime.initialize(context.filesDir.absolutePath)

// 2. Install verified language package (e.g. Tamil)
val packageId = runtime.installPackage("/sdcard/Download/lang-ta")

// 3. Create active inference session
val session = runtime.createSession("ta-IN")

// 4. Assess Spoken Audio
val audioPcmSamples: FloatArray = captureMicrophonePcm() // 16 kHz Float32 array
val result = session.assess(
    referenceText = "பழம்",
    audioSamples = audioPcmSamples,
    sampleRate = 16000
)

println("Overall Score: ${result.overallScore}")
for (ph in result.phonemes) {
    println("Target: ${ph.expectedPhoneme}, Status: ${ph.status}, Score: ${ph.score}")
    if (ph.status == PhonemeStatus.SUBSTITUTION) {
        // Highlight corresponding character in UI using [ph.charStartUtf8, ph.charEndUtf8]
    }
}

// 5. Cleanup when finished
session.close()
runtime.close()
```

---

## 2. iOS Integration (Swift / SPM)

### 2.1 Package Setup
In Xcode, select **File > Add Packages...** and add the repository URL or local package reference:
`path/to/olineri-core/bindings/ios`

### 2.2 SDK Usage

```swift
import OlineriSDK

do {
    // 1. Initialize Runtime
    let docsUrl = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    let runtime = try OlineriRuntime(storageDirectory: docsUrl.path)

    // 2. Install package
    let pkgPath = Bundle.main.path(forResource: "lang-ta", ofType: nil)!
    let pkgId = try runtime.installPackage(from: pkgPath)

    // 3. Create Session
    let session = try runtime.createSession(language: "ta-IN")

    // 4. Assess Pronunciation
    let audioSamples: [Float] = recordPcmSamples() // 16 kHz Float32 PCM
    let result = try session.assess(
        text: "பழம்",
        audioSamples: audioSamples,
        sampleRate: 16000
    )

    print("Overall Score: \(result.overallScore)")
    for ph in result.phonemes {
        print("Phoneme: \(ph.expectedPhoneme), Status: \(ph.status), Score: \(ph.score)")
    }
} catch {
    print("Pronunciation assessment error: \(error)")
}
```

---

## 3. React Native Integration

### 3.1 Installation

```bash
npm install @olineri/react-native-sdk
```

### 3.2 Usage

```typescript
import PronunciationSDK from "@olineri/react-native-sdk";

// 1. Initialize SDK
const sdk = await PronunciationSDK.initialize({
  storageDirectory: "managed-by-native-sdk",
  enabledFeatures: [
    "speechRecognition",
    "pronunciationAssessment",
    "phonemeFeedback",
    "characterMapping"
  ],
  resourceLimits: {
    maxConcurrentInferenceJobs: 1,
    releaseIdleModels: true
  }
});

// 2. Install language package from local assets
await sdk.languages.install("ta-IN", {
  source: "local",
  localPath: "/path/to/lang-ta"
});

// 3. Assess Spoken Pronunciation
const result = await sdk.assessPronunciation({
  language: "ta-IN",
  referenceText: "பழம்",
  audio: {
    type: "pcmFloatArray",
    samples: recordedAudioPcmArray, // Float array [-1.0, 1.0]
    sampleRate: 16000
  },
  options: {
    granularity: "phoneme",
    includeCharacterMapping: true
  }
});

console.log("Overall Score:", result.overallScore);
result.phonemes.forEach(p => {
  console.log(`Sound: ${p.expectedPhoneme}, Status: ${p.status}, Score: ${p.score}`);
});
```

---

## 4. Flutter Integration

### 4.1 Dependency Setup
In `pubspec.yaml`:

```yaml
dependencies:
  flutter:
    sdk: flutter
  olineri_flutter:
    path: path/to/olineri-core/bindings/flutter
```

### 4.2 SDK Usage

```dart
import 'dart:typed_data';
import 'package:olineri_flutter/olineri_flutter.dart';

Future<void> runAssessment() async {
  // 1. Initialize Runtime
  final runtime = await OlineriRuntime.initialize(
    storageDirectory: "/data/user/0/com.example/app_flutter/olineri_models",
  );

  // 2. Install Package
  final pkgId = await runtime.installPackage(packagePath: "/path/to/lang-ta");

  // 3. Create Session
  final session = await runtime.createSession(languageTag: "ta-IN");

  // 4. Assess Pronunciation
  final Float32List audioSamples = captureAudioBuffer();
  final result = await session.assess(
    referenceText: "பழம்",
    audioSamples: audioSamples,
    sampleRate: 16000,
  );

  print("Overall Score: ${result.overallScore}");
  for (final ph in result.phonemes) {
    print("Phoneme: ${ph.expectedPhoneme}, Status: ${ph.status}, Score: ${ph.score}");
  }

  // Cleanup
  session.close();
  runtime.close();
}
```

---

## 5. Capacitor / Ionic Integration

### 5.1 Installation

```bash
npm install @olineri/capacitor-plugin
npx cap sync
```

### 5.2 Usage

```typescript
import { Olineri } from '@olineri/capacitor-plugin';

// 1. Initialize Runtime
await Olineri.initializeRuntime({});

// 2. Install Package
await Olineri.installPackage({ packagePath: '/path/to/lang-ta' });

// 3. Assess Pronunciation
const result = await Olineri.assessPronunciation({
  language: 'ta-IN',
  referenceText: 'பழம்',
  samples: recordedPcmFloats,
  sampleRate: 16000,
});

console.log('Result:', result.overallScore, result.phonemes);
```
