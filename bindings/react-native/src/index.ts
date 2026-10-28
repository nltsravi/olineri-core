import { NativeModules, Platform } from "react-native";
import {
  SDKInitConfig,
  InstalledLanguageInfo,
  PackageInstallOptions,
  AssessmentRequest,
  AssessmentResult,
  SDKFeature,
} from "./types";

const { OlineriNativeBridge } = NativeModules;

export class LanguageManager {
  async listInstalled(): Promise<InstalledLanguageInfo[]> {
    if (!OlineriNativeBridge) {
      throw new Error("OlineriNativeBridge is not linked in this environment.");
    }
    return await OlineriNativeBridge.listInstalledLanguages();
  }

  async install(
    languageTag: string,
    options: PackageInstallOptions
  ): Promise<string> {
    if (!OlineriNativeBridge) {
      throw new Error("OlineriNativeBridge is not linked in this environment.");
    }
    return await OlineriNativeBridge.installPackage(languageTag, options);
  }

  async uninstall(packageId: string): Promise<boolean> {
    if (!OlineriNativeBridge) {
      throw new Error("OlineriNativeBridge is not linked in this environment.");
    }
    return await OlineriNativeBridge.uninstallPackage(packageId);
  }

  async getCapabilities(languageTag: string): Promise<SDKFeature[]> {
    if (!OlineriNativeBridge) {
      throw new Error("OlineriNativeBridge is not linked in this environment.");
    }
    return await OlineriNativeBridge.getPackageCapabilities(languageTag);
  }
}

export class PronunciationSDK {
  public readonly languages: LanguageManager;

  private constructor() {
    this.languages = new LanguageManager();
  }

  static async initialize(config: SDKInitConfig = {}): Promise<PronunciationSDK> {
    if (!OlineriNativeBridge) {
      // In web or mock testing environment, provide safe mock fallback
      console.warn("OlineriNativeBridge not available. Running in mock/headless mode.");
    } else {
      await OlineriNativeBridge.initializeRuntime(config);
    }
    return new PronunciationSDK();
  }

  async assessPronunciation(
    request: AssessmentRequest
  ): Promise<AssessmentResult> {
    if (!OlineriNativeBridge) {
      throw new Error("Olineri native speech engine is not accessible on this device.");
    }
    return await OlineriNativeBridge.assessPronunciation(request);
  }

  async release(): Promise<void> {
    if (OlineriNativeBridge) {
      await OlineriNativeBridge.shutdownRuntime();
    }
  }
}

export const OlineriSDK = PronunciationSDK;
export * from "./types";
export default PronunciationSDK;
