#include <jni.h>
#include <string>
#include <vector>
#include "vaani/vaani_c_api.h"

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_vaani_core_VaaniRuntime_nativeCreate(JNIEnv* env, jobject /* this */, jstring storageDir) {
    const char* dir = env->GetStringUTFChars(storageDir, nullptr);
    vaani_runtime_handle_t runtime = nullptr;
    vaani_status_t status = vaani_runtime_create(dir, &runtime);
    env->ReleaseStringUTFChars(storageDir, dir);

    if (status != VAANI_OK) {
        return 0;
    }
    return reinterpret_cast<jlong>(runtime);
}

JNIEXPORT void JNICALL
Java_com_vaani_core_VaaniRuntime_nativeDestroy(JNIEnv* /* env */, jobject /* this */, jlong handle) {
    if (handle != 0) {
        vaani_runtime_destroy(reinterpret_cast<vaani_runtime_handle_t>(handle));
    }
}

JNIEXPORT jstring JNICALL
Java_com_vaani_core_VaaniRuntime_nativeInstallPackage(JNIEnv* env, jobject /* this */, jlong handle, jstring packagePath) {
    if (handle == 0) return nullptr;

    const char* path = env->GetStringUTFChars(packagePath, nullptr);
    char out_id[64] = {0};
    vaani_status_t status = vaani_package_install(
        reinterpret_cast<vaani_runtime_handle_t>(handle),
        path,
        out_id,
        sizeof(out_id)
    );
    env->ReleaseStringUTFChars(packagePath, path);

    if (status != VAANI_OK) {
        return nullptr;
    }
    return env->NewStringUTF(out_id);
}

JNIEXPORT jboolean JNICALL
Java_com_vaani_core_VaaniRuntime_nativeUninstallPackage(JNIEnv* env, jobject /* this */, jlong handle, jstring packageId) {
    if (handle == 0) return JNI_FALSE;

    const char* id = env->GetStringUTFChars(packageId, nullptr);
    vaani_status_t status = vaani_package_uninstall(
        reinterpret_cast<vaani_runtime_handle_t>(handle),
        id
    );
    env->ReleaseStringUTFChars(packageId, id);
    return (status == VAANI_OK) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_vaani_core_VaaniSession_nativeCreate(JNIEnv* env, jobject /* this */, jlong runtimeHandle, jstring languageTag) {
    if (runtimeHandle == 0) return 0;

    const char* lang = env->GetStringUTFChars(languageTag, nullptr);
    vaani_session_handle_t session = nullptr;
    vaani_status_t status = vaani_session_create(
        reinterpret_cast<vaani_runtime_handle_t>(runtimeHandle),
        lang,
        &session
    );
    env->ReleaseStringUTFChars(languageTag, lang);

    if (status != VAANI_OK) {
        return 0;
    }
    return reinterpret_cast<jlong>(session);
}

JNIEXPORT void JNICALL
Java_com_vaani_core_VaaniSession_nativeDestroy(JNIEnv* /* env */, jobject /* this */, jlong sessionHandle) {
    if (sessionHandle != 0) {
        vaani_session_destroy(reinterpret_cast<vaani_session_handle_t>(sessionHandle));
    }
}

JNIEXPORT jobject JNICALL
Java_com_vaani_core_VaaniSession_nativeAssessPronunciation(
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

    vaani_result_handle_t result = nullptr;
    vaani_status_t status = vaani_assess_pronunciation(
        reinterpret_cast<vaani_session_handle_t>(sessionHandle),
        text,
        samples,
        static_cast<uint32_t>(sample_count),
        static_cast<uint32_t>(sampleRate),
        &result
    );

    env->ReleaseFloatArrayElements(audioSamples, samples, JNI_ABORT);
    env->ReleaseStringUTFChars(referenceText, text);

    if (status != VAANI_OK || result == nullptr) {
        return nullptr;
    }

    float overall_score = vaani_result_get_overall_score(result);
    uint32_t phoneme_count = vaani_result_get_phoneme_count(result);

    // Find Kotlin classes
    jclass resultClass = env->FindClass("com/vaani/core/VaaniAssessmentResult");
    jmethodID resultCtor = env->GetMethodID(resultClass, "<init>", "(FLjava/util/List;)V");

    jclass feedbackClass = env->FindClass("com/vaani/core/PhonemeFeedback");
    jmethodID feedbackCtor = env->GetMethodID(feedbackClass, "<init>", "(Ljava/lang/String;IFII)V");

    jclass listClass = env->FindClass("java/util/ArrayList");
    jmethodID listCtor = env->GetMethodID(listClass, "<init>", "()V");
    jmethodID listAdd = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    jobject feedbackList = env->NewObject(listClass, listCtor);

    for (uint32_t i = 0; i < phoneme_count; ++i) {
        vaani_phoneme_feedback_t fb;
        if (vaani_result_get_phoneme_feedback(result, i, &fb) == VAANI_OK) {
            jstring expPhoneme = env->NewStringUTF(fb.expected_phoneme);
            jobject fbObj = env->NewObject(
                feedbackClass,
                feedbackCtor,
                expPhoneme,
                static_cast<jint>(fb.status),
                fb.score,
                static_cast<jint>(fb.char_start_utf8),
                static_cast<jint>(fb.char_end_utf8)
            );
            env->CallBooleanMethod(feedbackList, listAdd, fbObj);
            env->DeleteLocalRef(expPhoneme);
            env->DeleteLocalRef(fbObj);
        }
    }

    jobject resultObj = env->NewObject(resultClass, resultCtor, overall_score, feedbackList);

    vaani_result_destroy(result);
    return resultObj;
}

} // extern "C"
