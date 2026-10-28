import { NativeModules, Platform } from "react-native";
import {
  SDKInitConfig,
  InstalledLanguageInfo,
  PackageInstallOptions,
  AssessmentRequest,
  AssessmentResult,
  SDKFeature,
} from "./types";

const { VaaniNativeBridge } = NativeModules;

export class LanguageManager {
  async listInstalled(): Promise<InstalledLanguageInfo[]> {
    if (!VaaniNativeBridge) {
      throw new Error("VaaniNativeBridge is not linked in this environment.");
    }
    return await VaaniNativeBridge.listInstalledLanguages();
  }

  async install(
    languageTag: string,
    options: PackageInstallOptions
  ): Promise<string> {
    if (!VaaniNativeBridge) {
      throw new Error("VaaniNativeBridge is not linked in this environment.");
    }
    return await VaaniNativeBridge.installPackage(languageTag, options);
  }

  async uninstall(packageId: string): Promise<boolean> {
    if (!VaaniNativeBridge) {
      throw new Error("VaaniNativeBridge is not linked in this environment.");
    }
    return await VaaniNativeBridge.uninstallPackage(packageId);
  }

  async getCapabilities(languageTag: string): Promise<SDKFeature[]> {
    if (!VaaniNativeBridge) {
      throw new Error("VaaniNativeBridge is not linked in this environment.");
    }
    return await VaaniNativeBridge.getPackageCapabilities(languageTag);
  }
}

export class PronunciationSDK {
  public readonly languages: LanguageManager;

  private constructor() {
    this.languages = new LanguageManager();
  }

  static async initialize(config: SDKInitConfig = {}): Promise<PronunciationSDK> {
    if (!VaaniNativeBridge) {
      // In web or mock testing environment, provide safe mock fallback
      console.warn("VaaniNativeBridge not available. Running in mock/headless mode.");
    } else {
      await VaaniNativeBridge.initializeRuntime(config);
    }
    return new PronunciationSDK();
  }

  async assessPronunciation(
    request: AssessmentRequest
  ): Promise<AssessmentResult> {
    if (!VaaniNativeBridge) {
      throw new Error("Vaani native speech engine is not accessible on this device.");
    }
    return await VaaniNativeBridge.assessPronunciation(request);
  }

  async release(): Promise<void> {
    if (VaaniNativeBridge) {
      await VaaniNativeBridge.shutdownRuntime();
    }
  }
}

export * from "./types";
export default PronunciationSDK;
