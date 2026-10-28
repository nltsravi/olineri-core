export type SDKFeature =
  | "speechRecognition"
  | "pronunciationAssessment"
  | "phonemeFeedback"
  | "wordFeedback"
  | "sentenceFeedback"
  | "characterMapping";

export type PhonemeStatus =
  | "correct"
  | "substitution"
  | "omission"
  | "insertion"
  | "uncertain";

export interface ResourceLimits {
  maxConcurrentInferenceJobs?: number;
  releaseIdleModels?: boolean;
}

export interface SDKInitConfig {
  storageDirectory?: string;
  enabledFeatures?: SDKFeature[];
  resourceLimits?: ResourceLimits;
}

export interface PackageInstallOptions {
  source: "catalog" | "local";
  localPath?: string;
  checksum?: string;
}

export interface InstalledLanguageInfo {
  packageId: string;
  languageTag: string;
  displayName: string;
  version: string;
  installedSizeBytes: number;
}

export interface PhonemeResult {
  expectedPhoneme: string;
  status: PhonemeStatus;
  score: number;
  charStartUtf8: number;
  charEndUtf8: number;
  detectedPhoneme?: string;
  feedbackAdvice?: string;
}

export interface AssessmentOptions {
  granularity?: "phoneme" | "word" | "sentence";
  includeCharacterMapping?: boolean;
}

export type AudioInput =
  | { type: "pcmFloatArray"; samples: number[]; sampleRate?: number }
  | { type: "localFilePath"; path: string }
  | { type: "base64Pcm"; data: string; sampleRate?: number };

export interface AssessmentRequest {
  language: string;
  referenceText: string;
  audio: AudioInput;
  options?: AssessmentOptions;
}

export interface AssessmentResult {
  language: string;
  referenceText: string;
  overallScore: number;
  phonemes: PhonemeResult[];
  status: "success" | "uncertain" | "error";
  errorDetails?: string;
}
