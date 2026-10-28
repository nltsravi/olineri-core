import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Opaque handles
final class OlineriRuntimeOpaque extends Opaque {}
final class OlineriSessionOpaque extends Opaque {}
final class OlineriResultOpaque extends Opaque {}

// Structs
final class OlineriPhonemeFeedbackStruct extends Struct {
  @Array(16)
  external Array<Uint8> expectedPhoneme;

  @Int32()
  external int status;

  @Float()
  external double score;

  @Uint32()
  external int charStartUtf8;

  @Uint32()
  external int charEndUtf8;
}

// Function typedefs
typedef OlineriRuntimeCreateC = Int32 Function(
    Pointer<Utf8> storageDir, Pointer<Pointer<OlineriRuntimeOpaque>> outRuntime);
typedef OlineriRuntimeCreateDart = int Function(
    Pointer<Utf8> storageDir, Pointer<Pointer<OlineriRuntimeOpaque>> outRuntime);

typedef OlineriRuntimeDestroyC = Void Function(Pointer<OlineriRuntimeOpaque> runtime);
typedef OlineriRuntimeDestroyDart = void Function(Pointer<OlineriRuntimeOpaque> runtime);

typedef OlineriPackageInstallC = Int32 Function(
    Pointer<OlineriRuntimeOpaque> runtime,
    Pointer<Utf8> packagePath,
    Pointer<Utf8> outPackageId,
    Uint32 maxIdLen);
typedef OlineriPackageInstallDart = int Function(
    Pointer<OlineriRuntimeOpaque> runtime,
    Pointer<Utf8> packagePath,
    Pointer<Utf8> outPackageId,
    int maxIdLen);

typedef OlineriPackageUninstallC = Int32 Function(
    Pointer<OlineriRuntimeOpaque> runtime, Pointer<Utf8> packageId);
typedef OlineriPackageUninstallDart = int Function(
    Pointer<OlineriRuntimeOpaque> runtime, Pointer<Utf8> packageId);

typedef OlineriSessionCreateC = Int32 Function(
    Pointer<OlineriRuntimeOpaque> runtime,
    Pointer<Utf8> languageTag,
    Pointer<Pointer<OlineriSessionOpaque>> outSession);
typedef OlineriSessionCreateDart = int Function(
    Pointer<OlineriRuntimeOpaque> runtime,
    Pointer<Utf8> languageTag,
    Pointer<Pointer<OlineriSessionOpaque>> outSession);

typedef OlineriSessionDestroyC = Void Function(Pointer<OlineriSessionOpaque> session);
typedef OlineriSessionDestroyDart = void Function(Pointer<OlineriSessionOpaque> session);

typedef OlineriAssessPronunciationC = Int32 Function(
    Pointer<OlineriSessionOpaque> session,
    Pointer<Utf8> referenceText,
    Pointer<Float> audioSamples,
    Uint32 sampleCount,
    Uint32 sampleRate,
    Pointer<Pointer<OlineriResultOpaque>> outResult);
typedef OlineriAssessPronunciationDart = int Function(
    Pointer<OlineriSessionOpaque> session,
    Pointer<Utf8> referenceText,
    Pointer<Float> audioSamples,
    int sampleCount,
    int sampleRate,
    Pointer<Pointer<OlineriResultOpaque>> outResult);

typedef OlineriResultGetOverallScoreC = Float Function(Pointer<OlineriResultOpaque> result);
typedef OlineriResultGetOverallScoreDart = double Function(Pointer<OlineriResultOpaque> result);

typedef OlineriResultGetPhonemeCountC = Uint32 Function(Pointer<OlineriResultOpaque> result);
typedef OlineriResultGetPhonemeCountDart = int Function(Pointer<OlineriResultOpaque> result);

typedef OlineriResultGetPhonemeFeedbackC = Int32 Function(
    Pointer<OlineriResultOpaque> result,
    Uint32 index,
    Pointer<OlineriPhonemeFeedbackStruct> outFeedback);
typedef OlineriResultGetPhonemeFeedbackDart = int Function(
    Pointer<OlineriResultOpaque> result,
    int index,
    Pointer<OlineriPhonemeFeedbackStruct> outFeedback);

typedef OlineriResultDestroyC = Void Function(Pointer<OlineriResultOpaque> result);
typedef OlineriResultDestroyDart = void Function(Pointer<OlineriResultOpaque> result);

class OlineriFFIBindings {
  final DynamicLibrary _lib;

  late final OlineriRuntimeCreateDart runtimeCreate;
  late final OlineriRuntimeDestroyDart runtimeDestroy;
  late final OlineriPackageInstallDart packageInstall;
  late final OlineriPackageUninstallDart packageUninstall;
  late final OlineriSessionCreateDart sessionCreate;
  late final OlineriSessionDestroyDart sessionDestroy;
  late final OlineriAssessPronunciationDart assessPronunciation;
  late final OlineriResultGetOverallScoreDart resultGetOverallScore;
  late final OlineriResultGetPhonemeCountDart resultGetPhonemeCount;
  late final OlineriResultGetPhonemeFeedbackDart resultGetPhonemeFeedback;
  late final OlineriResultDestroyDart resultDestroy;

  OlineriFFIBindings() : _lib = _loadLibrary() {
    runtimeCreate = _lib.lookupFunction<OlineriRuntimeCreateC, OlineriRuntimeCreateDart>('olineri_runtime_create');
    runtimeDestroy = _lib.lookupFunction<OlineriRuntimeDestroyC, OlineriRuntimeDestroyDart>('olineri_runtime_destroy');
    packageInstall = _lib.lookupFunction<OlineriPackageInstallC, OlineriPackageInstallDart>('olineri_package_install');
    packageUninstall = _lib.lookupFunction<OlineriPackageUninstallC, OlineriPackageUninstallDart>('olineri_package_uninstall');
    sessionCreate = _lib.lookupFunction<OlineriSessionCreateC, OlineriSessionCreateDart>('olineri_session_create');
    sessionDestroy = _lib.lookupFunction<OlineriSessionDestroyC, OlineriSessionDestroyDart>('olineri_session_destroy');
    assessPronunciation = _lib.lookupFunction<OlineriAssessPronunciationC, OlineriAssessPronunciationDart>('olineri_assess_pronunciation');
    resultGetOverallScore = _lib.lookupFunction<OlineriResultGetOverallScoreC, OlineriResultGetOverallScoreDart>('olineri_result_get_overall_score');
    resultGetPhonemeCount = _lib.lookupFunction<OlineriResultGetPhonemeCountC, OlineriResultGetPhonemeCountDart>('olineri_result_get_phoneme_count');
    resultGetPhonemeFeedback = _lib.lookupFunction<OlineriResultGetPhonemeFeedbackC, OlineriResultGetPhonemeFeedbackDart>('olineri_result_get_phoneme_feedback');
    resultDestroy = _lib.lookupFunction<OlineriResultDestroyC, OlineriResultDestroyDart>('olineri_result_destroy');
  }

  static DynamicLibrary _loadLibrary() {
    if (Platform.isAndroid) {
      return DynamicLibrary.open('libolineri_core.so');
    } else if (Platform.isIOS || Platform.isMacOS) {
      return DynamicLibrary.process();
    }
    throw UnsupportedError('Unsupported platform for Olineri FFI');
  }
}
