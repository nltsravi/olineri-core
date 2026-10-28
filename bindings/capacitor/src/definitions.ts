export interface OlineriCapacitorPlugin {
  initializeRuntime(options: { storageDirectory?: string }): Promise<{ success: boolean }>;
  installPackage(options: { packagePath: string }): Promise<{ packageId: string }>;
  uninstallPackage(options: { packageId: string }): Promise<{ success: boolean }>;
  assessPronunciation(options: {
    language: string;
    referenceText: string;
    samples: number[];
    sampleRate?: number;
  }): Promise<{
    language: string;
    referenceText: string;
    overallScore: number;
    phonemes: Array<{
      expectedPhoneme: string;
      status: string;
      score: number;
      charStartUtf8: number;
      charEndUtf8: number;
    }>;
  }>;
}
