# Security Policy

## 1. Supported Versions

Security updates and patches are provided for the current active release line:

| Version | Supported          |
| ------- | ------------------ |
| 0.1.x   | :white_check_mark: |

---

## 2. Reporting a Vulnerability

We take the security of `vaani-core` seriously. If you discover a security vulnerability, please report it responsibly:

* **DO NOT** file a public GitHub issue for security vulnerabilities.
* Send an email with full reproduction steps to: `security@vaani.dev` (or the maintainer's contact specified on GitHub).
* Include:
  * Description of the vulnerability (e.g. buffer overflow, zip-slip path traversal, signature bypass).
  * Proof-of-concept (PoC) archive or input sample.
  * Target platform and SDK version.
* You will receive an acknowledgment within 48 hours.

---

## 3. Threat Model Commitments

1. **Zero Unexpected Network Access:** The core inference engine executes with zero network privileges.
2. **Mandatory Signature Validation:** Language packages must pass cryptographic signature checks (Ed25519) before extraction.
3. **Sandbox Isolation:** Package extraction rejects path traversal sequences (`..`), symlinks, and oversized archives.
4. **Audio Privacy:** Microphone audio is retained in memory only during active inference and wiped immediately upon completion.
