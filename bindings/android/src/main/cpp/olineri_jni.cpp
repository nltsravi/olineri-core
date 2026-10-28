#include <jni.h>
#include <string>
#include <vector>
#include "olineri/olineri_c_api.h"

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_olineri_core_OlineriRuntime_nativeVersion(JNIEnv* env, jclass /* clazz */) {
    return env->NewStringUTF(olineri_version());
}

JNIEXPORT jlong JNICALL
Java_com_olineri_core_OlineriRuntime_nativeCreate(JNIEnv* env, jobject /* this */, jstring storageDir) {
    const char* dir = env->GetStringUTFChars(storageDir, nullptr);
    olineri_runtime_handle_t runtime = nullptr;
    olineri_status_t status = olineri_runtime_create(dir, &runtime);
    env->ReleaseStringUTFChars(storageDir, dir);

    if (status != OLINERI_OK) {
        return 0;
    }
    return reinterpret_cast<jlong>(runtime);
}

JNIEXPORT void JNICALL
Java_com_olineri_core_OlineriRuntime_nativeDestroy(JNIEnv* /* env */, jobject /* this */, jlong handle) {
    if (handle != 0) {
        olineri_runtime_destroy(reinterpret_cast<olineri_runtime_handle_t>(handle));
    }
}

JNIEXPORT jstring JNICALL
Java_com_olineri_core_OlineriRuntime_nativeInstallPackage(JNIEnv* env, jobject /* this */, jlong handle, jstring packagePath) {
    if (handle == 0) return nullptr;

    const char* path = env->GetStringUTFChars(packagePath, nullptr);
    char out_id[64] = {0};
    olineri_status_t status = olineri_package_install(
        reinterpret_cast<olineri_runtime_handle_t>(handle),
        path,
        out_id,
        sizeof(out_id)
    );
    env->ReleaseStringUTFChars(packagePath, path);

    if (status != OLINERI_OK) {
        return nullptr;
    }
    return env->NewStringUTF(out_id);
}

JNIEXPORT jboolean JNICALL
Java_com_olineri_core_OlineriRuntime_nativeUninstallPackage(JNIEnv* env, jobject /* this */, jlong handle, jstring packageId) {
    if (handle == 0) return JNI_FALSE;

    const char* id = env->GetStringUTFChars(packageId, nullptr);
    olineri_status_t status = olineri_package_uninstall(
        reinterpret_cast<olineri_runtime_handle_t>(handle),
        id
    );
    env->ReleaseStringUTFChars(packageId, id);
    return (status == OLINERI_OK) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_olineri_core_OlineriRuntime_nativeRollbackPackage(JNIEnv* env, jobject /* this */, jlong handle, jstring packageId) {
    if (handle == 0) return JNI_FALSE;

    const char* id = env->GetStringUTFChars(packageId, nullptr);
    olineri_status_t status = olineri_package_rollback(
        reinterpret_cast<olineri_runtime_handle_t>(handle),
        id
    );
    env->ReleaseStringUTFChars(packageId, id);
    return (status == OLINERI_OK) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jobject JNICALL
Java_com_olineri_core_OlineriRuntime_nativeListPackages(JNIEnv* env, jobject /* this */, jlong handle) {
    if (handle == 0) return nullptr;

    auto rt = reinterpret_cast<olineri_runtime_handle_t>(handle);
    uint32_t count = olineri_package_count(rt);

    jclass listClass = env->FindClass("java/util/ArrayList");
    jmethodID listCtor = env->GetMethodID(listClass, "<init>", "()V");
    jmethodID listAdd = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");
    jobject pkgList = env->NewObject(listClass, listCtor);

    jclass infoClass = env->FindClass("com/olineri/core/InstalledPackageInfo");
    jmethodID infoCtor = env->GetMethodID(infoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

    for (uint32_t i = 0; i < count; ++i) {
        char pkg_id[64] = {0};
        char lang_tag[32] = {0};
        char version[32] = {0};
        if (olineri_package_get_at(rt, i, pkg_id, sizeof(pkg_id), lang_tag, sizeof(lang_tag), version, sizeof(version)) == OLINERI_OK) {
            jstring jPkgId = env->NewStringUTF(pkg_id);
            jstring jLangTag = env->NewStringUTF(lang_tag);
            jstring jVersion = env->NewStringUTF(version);
            jobject infoObj = env->NewObject(infoClass, infoCtor, jPkgId, jLangTag, jVersion);
            env->CallBooleanMethod(pkgList, listAdd, infoObj);
            env->DeleteLocalRef(jPkgId);
            env->DeleteLocalRef(jLangTag);
            env->DeleteLocalRef(jVersion);
            env->DeleteLocalRef(infoObj);
        }
    }
    return pkgList;
}

JNIEXPORT jboolean JNICALL
Java_com_olineri_core_OlineriRuntime_nativeAddTrustedKey(JNIEnv* env, jobject /* this */, jlong handle, jstring keyId, jstring publicKeyHex) {
    if (handle == 0) return JNI_FALSE;

    const char* id = env->GetStringUTFChars(keyId, nullptr);
    const char* hex = env->GetStringUTFChars(publicKeyHex, nullptr);
    olineri_status_t status = olineri_runtime_add_trusted_key(
        reinterpret_cast<olineri_runtime_handle_t>(handle),
        id,
        hex
    );
    env->ReleaseStringUTFChars(publicKeyHex, hex);
    env->ReleaseStringUTFChars(keyId, id);
    return (status == OLINERI_OK) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_olineri_core_OlineriRuntime_nativeTrimMemory(JNIEnv* /* env */, jobject /* this */, jlong handle) {
    if (handle == 0) return JNI_FALSE;
    olineri_status_t status = olineri_runtime_trim_memory(reinterpret_cast<olineri_runtime_handle_t>(handle));
    return (status == OLINERI_OK) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_olineri_core_OlineriRuntime_nativeCreateSession(JNIEnv* env, jobject /* this */, jlong runtimeHandle, jstring languageTag) {
    if (runtimeHandle == 0) return 0;

    const char* lang = env->GetStringUTFChars(languageTag, nullptr);
    olineri_session_handle_t session = nullptr;
    olineri_status_t status = olineri_session_create(
        reinterpret_cast<olineri_runtime_handle_t>(runtimeHandle),
        lang,
        &session
    );
    env->ReleaseStringUTFChars(languageTag, lang);

    if (status != OLINERI_OK) {
        return 0;
    }
    return reinterpret_cast<jlong>(session);
}

JNIEXPORT jlong JNICALL
Java_com_olineri_core_OlineriSession_nativeCreate(JNIEnv* env, jobject thiz, jlong runtimeHandle, jstring languageTag) {
    return Java_com_olineri_core_OlineriRuntime_nativeCreateSession(env, thiz, runtimeHandle, languageTag);
}

JNIEXPORT void JNICALL
Java_com_olineri_core_OlineriSession_nativeDestroy(JNIEnv* /* env */, jobject /* this */, jlong sessionHandle) {
    if (sessionHandle != 0) {
        olineri_session_destroy(reinterpret_cast<olineri_session_handle_t>(sessionHandle));
    }
}

JNIEXPORT jobject JNICALL
Java_com_olineri_core_OlineriSession_nativeAssessPronunciation(
    JNIEnv* env,
    jobject /* this */,
    jlong sessionHandle,
    jstring referenceText,
    jfloatArray audioSamples,
    jint sampleRate
) {
    if (sessionHandle == 0) return nullptr;

    const char* text = env->GetStringUTFChars(referenceText, nullptr);
    jsize sample_count = env->GetArrayLength(audioSamples);
    jfloat* samples = env->GetFloatArrayElements(audioSamples, nullptr);

    olineri_result_handle_t result = nullptr;
    olineri_status_t status = olineri_assess_pronunciation(
        reinterpret_cast<olineri_session_handle_t>(sessionHandle),
        text,
        samples,
        static_cast<uint32_t>(sample_count),
        static_cast<uint32_t>(sampleRate),
        &result
    );

    env->ReleaseFloatArrayElements(audioSamples, samples, JNI_ABORT);
    env->ReleaseStringUTFChars(referenceText, text);

    if (status != OLINERI_OK || result == nullptr) {
        return nullptr;
    }

    float overall_score = olineri_result_get_overall_score(result);
    uint32_t phoneme_count = olineri_result_get_phoneme_count(result);
    const char* rec_text = olineri_result_get_recognized_text(result);

    // Find Kotlin classes
    jclass resultClass = env->FindClass("com/olineri/core/OlineriAssessmentResult");
    jmethodID resultCtor = env->GetMethodID(resultClass, "<init>", "(FLjava/util/List;Ljava/lang/String;)V");

    jclass feedbackClass = env->FindClass("com/olineri/core/PhonemeFeedback");
    jmethodID feedbackCtor = env->GetMethodID(feedbackClass, "<init>", "(Ljava/lang/String;IFIILjava/lang/String;Ljava/lang/String;)V");

    jclass listClass = env->FindClass("java/util/ArrayList");
    jmethodID listCtor = env->GetMethodID(listClass, "<init>", "()V");
    jmethodID listAdd = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    jobject feedbackList = env->NewObject(listClass, listCtor);

    for (uint32_t i = 0; i < phoneme_count; ++i) {
        olineri_phoneme_feedback_t fb;
        if (olineri_result_get_phoneme_feedback(result, i, &fb) == OLINERI_OK) {
            jstring expPhoneme = env->NewStringUTF(fb.expected_phoneme ? fb.expected_phoneme : "");
            jstring detPhoneme = env->NewStringUTF(fb.detected_phoneme ? fb.detected_phoneme : "");
            jstring fbInstr = env->NewStringUTF(fb.feedback_instruction ? fb.feedback_instruction : "");
            jobject fbObj = env->NewObject(
                feedbackClass,
                feedbackCtor,
                expPhoneme,
                static_cast<jint>(fb.status),
                fb.score,
                static_cast<jint>(fb.char_start_utf8),
                static_cast<jint>(fb.char_end_utf8),
                detPhoneme,
                fbInstr
            );
            env->CallBooleanMethod(feedbackList, listAdd, fbObj);
            env->DeleteLocalRef(expPhoneme);
            env->DeleteLocalRef(detPhoneme);
            env->DeleteLocalRef(fbInstr);
            env->DeleteLocalRef(fbObj);
        }
    }

    jstring jRecText = env->NewStringUTF(rec_text ? rec_text : "");
    jobject resultObj = env->NewObject(resultClass, resultCtor, overall_score, feedbackList, jRecText);
    env->DeleteLocalRef(jRecText);

    olineri_result_destroy(result);
    return resultObj;
}

} // extern "C"
