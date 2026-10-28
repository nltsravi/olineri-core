import Foundation
import COlineri

/// Classification of a detected pronunciation segment.
public enum PhonemeStatus: Int, Sendable {
    case correct = 0
    case substitution = 1
    case omission = 2
    case insertion = 3
    case uncertain = 4

    public var description: String {
        switch self {
        case .correct: return "Correct"
        case .substitution: return "Substitution Error"
        case .omission: return "Sound Omitted"
        case .insertion: return "Sound Inserted"
        case .uncertain: return "Uncertain / Indeterminate"
        }
    }
}

/// Detailed phoneme feedback with grapheme-cluster coordinates.
public struct PhonemeFeedback: Sendable {
    public let expectedPhoneme: String
    public let status: PhonemeStatus
    public let score: Float
    public let charStartUtf8: Int
    public let charEndUtf8: Int

    public init(expectedPhoneme: String, status: PhonemeStatus, score: Float, charStartUtf8: Int, charEndUtf8: Int) {
        self.expectedPhoneme = expectedPhoneme
        self.status = status
        self.score = score
        self.charStartUtf8 = charStartUtf8
        self.charEndUtf8 = charEndUtf8
    }
}

/// Comprehensive offline pronunciation assessment result.
public struct OlineriAssessmentResult: Sendable {
    public let overallScore: Float
    public let phonemes: [PhonemeFeedback]

    public init(overallScore: Float, phonemes: [PhonemeFeedback]) {
        self.overallScore = overallScore
        self.phonemes = phonemes
    }
}

/// Typed Swift errors produced by the native speech engine.
public enum OlineriError: Error, LocalizedError {
    case initializationFailed(String)
    case packageNotFound(String)
    case invalidPackage(String)
    case sessionCreationFailed(String)
    case assessmentFailed(String)

    public var errorDescription: String? {
        switch self {
        case .initializationFailed(let msg): return "Runtime initialization failed: \(msg)"
        case .packageNotFound(let pkg): return "Package '\(pkg)' is not installed."
        case .invalidPackage(let reason): return "Invalid language package: \(reason)"
        case .sessionCreationFailed(let lang): return "Failed to create inference session for language: \(lang)"
        case .assessmentFailed(let msg): return "Pronunciation assessment failed: \(msg)"
        }
    }
}

/// Thread-safe language inference session.
public final class OlineriSession: @unchecked Sendable {
    private var handle: olineri_session_handle_t?
    public let language: String

    internal init(handle: olineri_session_handle_t, language: String) {
        self.handle = handle
        self.language = language
    }

    deinit {
        if let handle = handle {
            olineri_session_destroy(handle)
        }
    }

    /// Assesses spoken audio against expected reference text.
    /// - Parameters:
    ///   - text: Expected Brahmic script text (e.g. "பழம்", "नमः", "कमल", "అమ్మ").
    ///   - audioSamples: Array of single-channel float PCM samples normalized to [-1.0, 1.0].
    ///   - sampleRate: Audio sample rate in Hz (default 16000).
    public func assess(text: String, audioSamples: [Float], sampleRate: UInt32 = 16000) throws -> OlineriAssessmentResult {
        guard let handle = handle else {
            throw OlineriError.assessmentFailed("Session is closed.")
        }

        var resultHandle: olineri_result_handle_t?
        let status = audioSamples.withUnsafeBufferPointer { buffer in
            olineri_assess_pronunciation(
                handle,
                text,
                buffer.baseAddress,
                UInt32(buffer.count),
                sampleRate,
                &resultHandle
            )
        }

        guard status == OLINERI_OK, let res = resultHandle else {
            throw OlineriError.assessmentFailed("Engine returned status code: \(status.rawValue)")
        }

        defer { olineri_result_destroy(res) }

        let overallScore = olineri_result_get_overall_score(res)
        let phonemeCount = olineri_result_get_phoneme_count(res)

        var feedbacks: [PhonemeFeedback] = []
        feedbacks.reserveCapacity(Int(phonemeCount))

        for i in 0..<phonemeCount {
            var fb = olineri_phoneme_feedback_t()
            if olineri_result_get_phoneme_feedback(res, i, &fb) == OLINERI_OK {
                let exp = String(cString: &fb.expected_phoneme.0)
                let status = PhonemeStatus(rawValue: Int(fb.status.rawValue)) ?? .uncertain
                feedbacks.append(PhonemeFeedback(
                    expectedPhoneme: exp,
                    status: status,
                    score: fb.score,
                    charStartUtf8: Int(fb.char_start_utf8),
                    charEndUtf8: Int(fb.char_end_utf8)
                ))
            }
        }

        return OlineriAssessmentResult(overallScore: overallScore, phonemes: feedbacks)
    }
}

/// Master runtime managing offline packages and inference models.
public final class OlineriRuntime: @unchecked Sendable {
    private var handle: olineri_runtime_handle_t?
    public let storageDirectory: String

    public init(storageDirectory: String) throws {
        self.storageDirectory = storageDirectory
        var h: olineri_runtime_handle_t?
        let status = olineri_runtime_create(storageDirectory, &h)
        guard status == OLINERI_OK, let validHandle = h else {
            throw OlineriError.initializationFailed("Native code \(status.rawValue)")
        }
        self.handle = validHandle
    }

    deinit {
        if let handle = handle {
            olineri_runtime_destroy(handle)
        }
    }

    /// Installs a verified language package from a local directory.
    public func installPackage(from directoryPath: String) throws -> String {
        guard let handle = handle else {
            throw OlineriError.initializationFailed("Runtime is closed.")
        }

        var outId = [CChar](repeating: 0, count: 64)
        let status = olineri_package_install(handle, directoryPath, &outId, 64)
        guard status == OLINERI_OK else {
            throw OlineriError.invalidPackage("Failed with code \(status.rawValue)")
        }
        return String(cString: outId)
    }

    /// Uninstalls an existing language package.
    public func uninstallPackage(packageId: String) throws {
        guard let handle = handle else {
            throw OlineriError.initializationFailed("Runtime is closed.")
        }
        let status = olineri_package_uninstall(handle, packageId)
        guard status == OLINERI_OK else {
            throw OlineriError.packageNotFound(packageId)
        }
    }

    /// Creates an active inference session for the given language.
    public func createSession(language: String) throws -> OlineriSession {
        guard let handle = handle else {
            throw OlineriError.initializationFailed("Runtime is closed.")
        }

        var sessionHandle: olineri_session_handle_t?
        let status = olineri_session_create(handle, language, &sessionHandle)
        guard status == OLINERI_OK, let s = sessionHandle else {
            throw OlineriError.sessionCreationFailed(language)
        }
        return OlineriSession(handle: s, language: language)
    }
}
