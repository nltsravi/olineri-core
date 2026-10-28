library olineri_flutter;

import 'dart:ffi';
import 'dart:typed_data';
import 'package:ffi/ffi.dart';
import 'src/olineri_ffi_bindings.dart';

enum PhonemeStatus {
  correct,
  substitution,
  omission,
  insertion,
  uncertain;

  static PhonemeStatus fromCode(int code) {
    switch (code) {
      case 0: return PhonemeStatus.correct;
      case 1: return PhonemeStatus.substitution;
      case 2: return PhonemeStatus.omission;
      case 3: return PhonemeStatus.insertion;
      default: return PhonemeStatus.uncertain;
    }
  }
}

class PhonemeFeedback {
  final String expectedPhoneme;
  final PhonemeStatus status;
  final double score;
  final int charStartUtf8;
  final int charEndUtf8;

  PhonemeFeedback({
    required this.expectedPhoneme,
    required this.status,
    required this.score,
    required this.charStartUtf8,
    required this.charEndUtf8,
  });
}

class OlineriAssessmentResult {
  final double overallScore;
  final List<PhonemeFeedback> phonemes;

  OlineriAssessmentResult({
    required this.overallScore,
    required this.phonemes,
  });
}

class OlineriSession {
  final Pointer<OlineriSessionOpaque> _handle;
  final OlineriFFIBindings _bindings;
  final String language;
  bool _isClosed = false;

  OlineriSession._(this._handle, this._bindings, this.language);

  Future<OlineriAssessmentResult> assess({
    required String referenceText,
    required Float32List audioSamples,
    int sampleRate = 16000,
  }) async {
    if (_isClosed) throw StateError("OlineriSession is closed");

    final textUtf8 = referenceText.toNativeUtf8();
    final samplesPtr = calloc<Float>(audioSamples.length);
    samplesPtr.asTypedList(audioSamples.length).setAll(0, audioSamples);

    final outResult = calloc<Pointer<OlineriResultOpaque>>();

    try {
      final status = _bindings.assessPronunciation(
        _handle,
        textUtf8,
        samplesPtr,
        audioSamples.length,
        sampleRate,
        outResult,
      );

      if (status != 0 || outResult.value == nullptr) {
        throw StateError("Pronunciation assessment failed with code: $status");
      }

      final resultPtr = outResult.value;
      try {
        final overallScore = _bindings.resultGetOverallScore(resultPtr);
        final phonemeCount = _bindings.resultGetPhonemeCount(resultPtr);

        final phonemes = <PhonemeFeedback>[];
        final fbStruct = calloc<OlineriPhonemeFeedbackStruct>();

        try {
          for (int i = 0; i < phonemeCount; ++i) {
            if (_bindings.resultGetPhonemeFeedback(resultPtr, i, fbStruct) == 0) {
              final expPhonemeList = <int>[];
              for (int j = 0; j < 16; ++j) {
                final b = fbStruct.ref.expectedPhoneme[j];
                if (b == 0) break;
                expPhonemeList.add(b);
              }
              final expPhonemeStr = String.fromCharCodes(expPhonemeList);

              phonemes.add(PhonemeFeedback(
                expectedPhoneme: expPhonemeStr,
                status: PhonemeStatus.fromCode(fbStruct.ref.status),
                score: fbStruct.ref.score,
                charStartUtf8: fbStruct.ref.charStartUtf8,
                charEndUtf8: fbStruct.ref.charEndUtf8,
              ));
            }
          }
        } finally {
          calloc.free(fbStruct);
        }

        return OlineriAssessmentResult(
          overallScore: overallScore,
          phonemes: phonemes,
        );
      } finally {
        _bindings.resultDestroy(resultPtr);
      }
    } finally {
      calloc.free(textUtf8);
      calloc.free(samplesPtr);
      calloc.free(outResult);
    }
  }

  void close() {
    if (!_isClosed) {
      _bindings.sessionDestroy(_handle);
      _isClosed = true;
    }
  }
}

class OlineriRuntime {
  final Pointer<OlineriRuntimeOpaque> _handle;
  final OlineriFFIBindings _bindings;
  final String storageDirectory;
  bool _isClosed = false;

  OlineriRuntime._(this._handle, this._bindings, this.storageDirectory);

  static Future<OlineriRuntime> initialize({required String storageDirectory}) async {
    final bindings = OlineriFFIBindings();
    final dirUtf8 = storageDirectory.toNativeUtf8();
    final outHandle = calloc<Pointer<OlineriRuntimeOpaque>>();

    try {
      final status = bindings.runtimeCreate(dirUtf8, outHandle);
      if (status != 0 || outHandle.value == nullptr) {
        throw StateError("Failed to initialize Olineri runtime: code $status");
      }
      return OlineriRuntime._(outHandle.value, bindings, storageDirectory);
    } finally {
      calloc.free(dirUtf8);
      calloc.free(outHandle);
    }
  }

  Future<String> installPackage({required String packagePath}) async {
    if (_isClosed) throw StateError("OlineriRuntime is closed");

    final pathUtf8 = packagePath.toNativeUtf8();
    final outIdUtf8 = calloc<Uint8>(64);

    try {
      final status = _bindings.packageInstall(
        _handle,
        pathUtf8,
        outIdUtf8.cast<Utf8>(),
        64,
      );
      if (status != 0) {
        throw ArgumentError("Failed to install package from $packagePath: code $status");
      }
      return outIdUtf8.cast<Utf8>().toDartString();
    } finally {
      calloc.free(pathUtf8);
      calloc.free(outIdUtf8);
    }
  }

  Future<bool> uninstallPackage({required String packageId}) async {
    if (_isClosed) throw StateError("OlineriRuntime is closed");

    final idUtf8 = packageId.toNativeUtf8();
    try {
      final status = _bindings.packageUninstall(_handle, idUtf8);
      return status == 0;
    } finally {
      calloc.free(idUtf8);
    }
  }

  Future<OlineriSession> createSession({required String languageTag}) async {
    if (_isClosed) throw StateError("OlineriRuntime is closed");

    final langUtf8 = languageTag.toNativeUtf8();
    final outSession = calloc<Pointer<OlineriSessionOpaque>>();

    try {
      final status = _bindings.sessionCreate(_handle, langUtf8, outSession);
      if (status != 0 || outSession.value == nullptr) {
        throw ArgumentError("Failed to create session for language $languageTag: code $status");
      }
      return OlineriSession._(outSession.value, _bindings, languageTag);
    } finally {
      calloc.free(langUtf8);
      calloc.free(outSession);
    }
  }

  void close() {
    if (!_isClosed) {
      _bindings.runtimeDestroy(_handle);
      _isClosed = true;
    }
  }
}
