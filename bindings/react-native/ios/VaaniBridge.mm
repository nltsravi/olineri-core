#import <React/RCTBridgeModule.h>
#include "vaani/vaani_c_api.h"
#include <vector>
#include <string>

@interface VaaniNativeBridge : NSObject <RCTBridgeModule>
@end

@implementation VaaniNativeBridge {
    vaani_runtime_handle_t _runtime;
    vaani_session_handle_t _session;
    std::string _currentLanguage;
}

RCT_EXPORT_MODULE();

RCT_EXPORT_METHOD(initializeRuntime:(NSDictionary *)config
                  resolver:(RCTPromiseResolveBlock)resolve
                  rejecter:(RCTPromiseRejectBlock)reject)
{
    NSString *storageDir = config[@"storageDirectory"];
    if (!storageDir) {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        storageDir = [paths[0] stringByAppendingPathComponent:@"vaani_models"];
        [[NSFileManager defaultManager] createDirectoryAtPath:storageDir withIntermediateDirectories:YES attributes:nil error:nil];
    }

    vaani_status_t status = vaani_runtime_create([storageDir UTF8String], &_runtime);
    if (status != VAANI_OK) {
        reject(@"INIT_ERROR", @"Failed to initialize Vaani runtime", nil);
        return;
    }
    resolve(@(YES));
}

RCT_EXPORT_METHOD(installPackage:(NSString *)languageTag
                  options:(NSDictionary *)options
                  resolver:(RCTPromiseResolveBlock)resolve
                  rejecter:(RCTPromiseRejectBlock)reject)
{
    if (!_runtime) {
        reject(@"NOT_INITIALIZED", @"Runtime not initialized", nil);
        return;
    }

    NSString *source = options[@"source"] ?: @"local";
    if ([source isEqualToString:@"local"]) {
        NSString *localPath = options[@"localPath"];
        if (!localPath) {
            reject(@"INVALID_ARGS", @"localPath required", nil);
            return;
        }

        char out_id[64] = {0};
        vaani_status_t status = vaani_package_install(_runtime, [localPath UTF8String], out_id, sizeof(out_id));
        if (status != VAANI_OK) {
            reject(@"INSTALL_ERROR", @"Failed to install package", nil);
            return;
        }
        resolve([NSString stringWithUTF8String:out_id]);
    } else {
        reject(@"CATALOG_UNAVAILABLE", @"Remote downloads must be initiated by host application", nil);
    }
}

RCT_EXPORT_METHOD(assessPronunciation:(NSDictionary *)request
                  resolver:(RCTPromiseResolveBlock)resolve
                  rejecter:(RCTPromiseRejectBlock)reject)
{
    if (!_runtime) {
        reject(@"NOT_INITIALIZED", @"Runtime not initialized", nil);
        return;
    }

    NSString *language = request[@"language"];
    NSString *referenceText = request[@"referenceText"];
    NSDictionary *audioDict = request[@"audio"];

    if (!language || !referenceText || !audioDict) {
        reject(@"INVALID_ARGS", @"Missing required fields", nil);
        return;
    }

    NSArray *samplesArr = audioDict[@"samples"];
    if (!samplesArr) {
        reject(@"UNSUPPORTED_AUDIO", @"Float PCM samples array required", nil);
        return;
    }

    NSUInteger count = [samplesArr count];
    std::vector<float> samples(count);
    for (NSUInteger i = 0; i < count; ++i) {
        samples[i] = [samplesArr[i] floatValue];
    }

    // Switch or create session if needed
    std::string langStr = [language UTF8String];
    if (!_session || _currentLanguage != langStr) {
        if (_session) {
            vaani_session_destroy(_session);
            _session = nullptr;
        }
        vaani_status_t s_status = vaani_session_create(_runtime, langStr.c_str(), &_session);
        if (s_status != VAANI_OK) {
            reject(@"SESSION_ERROR", @"Failed to create session for language", nil);
            return;
        }
        _currentLanguage = langStr;
    }

    vaani_result_handle_t result = nullptr;
    vaani_status_t a_status = vaani_assess_pronunciation(
        _session,
        [referenceText UTF8String],
        samples.data(),
        static_cast<uint32_t>(count),
        16000,
        &result
    );

    if (a_status != VAANI_OK || !result) {
        reject(@"ASSESS_ERROR", @"Assessment failed", nil);
        return;
    }

    float overallScore = vaani_result_get_overall_score(result);
    uint32_t phonemeCount = vaani_result_get_phoneme_count(result);

    NSMutableArray *phonemes = [NSMutableArray arrayWithCapacity:phonemeCount];
    for (uint32_t i = 0; i < phonemeCount; ++i) {
        vaani_phoneme_feedback_t fb;
        if (vaani_result_get_phoneme_feedback(result, i, &fb) == VAANI_OK) {
            NSString *statusStr = @"uncertain";
            switch (fb.status) {
                case VAANI_PHONEME_CORRECT: statusStr = @"correct"; break;
                case VAANI_PHONEME_SUBSTITUTION: statusStr = @"substitution"; break;
                case VAANI_PHONEME_OMISSION: statusStr = @"omission"; break;
                case VAANI_PHONEME_INSERTION: statusStr = @"insertion"; break;
                default: break;
            }
            [phonemes addObject:@{
                @"expectedPhoneme": [NSString stringWithUTF8String:fb.expected_phoneme],
                @"status": statusStr,
                @"score": @(fb.score),
                @"charStartUtf8": @(fb.char_start_utf8),
                @"charEndUtf8": @(fb.char_end_utf8)
            }];
        }
    }

    vaani_result_destroy(result);

    resolve(@{
        @"language": language,
        @"referenceText": referenceText,
        @"overallScore": @(overallScore),
        @"status": @"success",
        @"phonemes": phonemes
    });
}

RCT_EXPORT_METHOD(shutdownRuntime:(RCTPromiseResolveBlock)resolve
                  rejecter:(RCTPromiseRejectBlock)reject)
{
    if (_session) {
        vaani_session_destroy(_session);
        _session = nullptr;
    }
    if (_runtime) {
        vaani_runtime_destroy(_runtime);
        _runtime = nullptr;
    }
    resolve(@(YES));
}

@end
