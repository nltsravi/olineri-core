import { WebPlugin } from '@capacitor/core';
import type { OlineriCapacitorPlugin } from './definitions';

export class OlineriWeb extends WebPlugin implements OlineriCapacitorPlugin {
  async initializeRuntime(_options: { storageDirectory?: string }): Promise<{ success: boolean }> {
    console.log('[OlineriWeb] Runtime initialized in Web/WASM simulation mode');
    return { success: true };
  }

  async installPackage(options: { packagePath: string }): Promise<{ packageId: string }> {
    console.log(`[OlineriWeb] Package registered: ${options.packagePath}`);
    return { packageId: 'lang-mock' };
  }

  async uninstallPackage(_options: { packageId: string }): Promise<{ success: boolean }> {
    return { success: true };
  }

  async assessPronunciation(options: {
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
  }> {
    return {
      language: options.language,
      referenceText: options.referenceText,
      overallScore: 88.5,
      phonemes: [
        {
          expectedPhoneme: '/ɐ/',
          status: 'correct',
          score: 92.0,
          charStartUtf8: 0,
          charEndUtf8: 3,
        },
      ],
    };
  }
}
