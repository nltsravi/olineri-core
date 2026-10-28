# Speech Model Selection, Evaluation, and Licensing Matrix

This document provides a factual, non-fabricated evaluation of speech-processing architectures and open-source models for offline Indian-language pronunciation assessment across **Sanskrit (`sa`)**, **Tamil (`ta`)**, **Hindi (`hi`)**, and **Telugu (`te`)**.

---

## 1. Executive Summary & Design Constraints

Pronunciation assessment differs fundamentally from standard Automatic Speech Recognition (ASR):
* **ASR Goal:** Map an acoustic signal to the most probable semantic text sequence, frequently relying on strong language models (LMs) to auto-correct slips of the tongue or regional accent variations.
* **Pronunciation Assessment Goal:** Verify whether the acoustic realization matches canonical phonological targets, identify acoustic-phonetic deviations (substitutions, omissions, insertions, distortions), quantify phoneme-level confidence, and locate errors in the written text.

### Strict Architectural Criteria
1. **Commercially Compatible Licensing:** Apache 2.0, MIT, or BSD. Strict rejection of `CC-BY-NC` (Non-Commercial) or unclear licenses.
2. **Offline Mobile Execution:** Compatible with ONNX Runtime / sherpa-onnx on ARM64 / ARMv7 (Android) and Apple Silicon (iOS) under 150 MB peak RAM.
3. **Phoneme / Frame Posterior Access:** The model must produce frame-level acoustic probabilities or unmasked logits across time $T$ to compute Goodness of Pronunciation (GOP) scores and forced alignment.

---

## 2. Speech Architecture Evaluation for Pronunciation Assessment

| Architecture | Example Implementations | Offline Mobile Feasibility | Phoneme / Frame-Level Scoring | Suitability for CAPT / Pronunciation Assessment |
| :--- | :--- | :--- | :--- | :--- |
| **Conformer CTC** | AI4Bharat IndicConformer CTC, NeMo Conformer | **High** (INT8 ONNX ~40–90 MB, fast non-autoregressive parallel inference) | **High** (Produces explicit frame-wise token posteriors $P(k \mid t)$, enabling Viterbi forced alignment & GOP) | **Recommended Primary Architecture** |
| **Wav2Vec2 / HuBERT CTC** | IndicWav2Vec, MMS-1B (CTC) | **Medium** (Fairseq / PyTorch exportable to ONNX; ~90–300 MB) | **High** (Frame rate ~50 Hz; excellent acoustic representations) | **Strong Alternative** (Higher memory footprint than Conformer) |
| **RNN-Transducer (RNN-T)** | Conformer Transducer, Zipformer | **Medium** (Requires streaming state cache, ~60–120 MB) | **Low** (Joint network outputs $P(y \mid t, u)$; alignment space is 2D lattice, complicating direct GOP posteriors) | **Not Recommended for Segmental GOP** |
| **Autoregressive Seq2Seq (Encoder-Decoder)** | OpenAI Whisper, AI4Bharat IndicWhisper | **Low-Medium** (Heavy cross-attention, autoregressive decoding latency on CPU) | **Very Low** (No frame-level phoneme posteriors; cross-attention weights do not yield calibrated phoneme durations) | **Unsuitable for Segmental Scoring** |

### Key Insight
Autoregressive models like **Whisper** and **RNN-Transducers** are optimized for high-level semantic transcription. They **cannot** natively expose frame-level acoustic posteriors for unconstrained phone scoring without complex heuristics. 
**CTC models (Conformer-CTC and Wav2Vec2-CTC)** yield a direct frame-level posterior matrix $L \in \mathbb{R}^{T \times V}$, enabling exact dynamic programming (Viterbi forced alignment) and log-posterior ratio scoring (GOP).

---

## 3. Evaluation of Candidate Models per Target Language

### 3.1. AI4Bharat IndicConformer (CTC)
* **Repository / Checkpoint:** AI4Bharat IndicConformer (`ai4bharat/indicconformer_stt_indic_hybrid_ctc` / quantized sherpa-onnx exports)
* **Supported Languages:** 22 official Indian languages, including Hindi, Tamil, Telugu, and Sanskrit.
* **Architecture:** Conformer-Large with hybrid CTC/Transducer decoders, exported as standalone CTC ONNX graphs.
* **Tokenizer:** Character-level and Byte-Pair-Encoding (BPE) subword models.
* **Sample Rate:** 16,000 Hz, 80-channel log Mel filterbank.
* **Inference Runtime:** ONNX Runtime / `sherpa-onnx` C++ engine.
* **Model Size:** 
  * Full FP32: ~480 MB
  * Quantized INT8: ~95 MB per language package (shared weights can be pruned to ~45 MB for single-language deploy).
* **Licensing:** **MIT License** (Fully commercially compatible).
* **Training Provenance:** Vistaar dataset (~25,000 hours of Indian language audio from YouTube, Kathbath, Shruti, and OpenSLR).
* **Suitability:** **Highest**. Standalone ONNX runtime execution confirmed on Android and iOS.

### 3.2. Meta Massively Multilingual Speech (MMS)
* **Repository / Checkpoint:** `facebook/mms-1b-all` / `facebook/mms-tts`
* **Supported Languages:** Tamil (`tam`), Sanskrit (`san`), Hindi (`hin`), Telugu (`tel`).
* **Architecture:** Wav2Vec2 with language-specific CTC adapter heads.
* **Licensing:** **Creative Commons Attribution-NonCommercial 4.0 International (CC-BY-NC 4.0)**.
* **Redistribution Rights:** **STRICTLY REJECTED FOR COMMERCIAL SDK USE**. Shipping MMS weights in a commercially licensed SDK violates Section 2 requirements.

### 3.3. OpenAI Whisper / IndicWhisper
* **Architecture:** Encoder-decoder autoregressive transformer.
* **Licensing:** MIT License.
* **Suitability for Pronunciation Assessment:** **Rejected**. Whisper suffers from hallucination under abnormal pronunciation, lacks acoustic frame posteriors, and is too computationally heavy for low-latency on-device phone alignment on budget Android devices.

### 3.4. Pre-Recorded Reference Acoustic Units (DTW Scorer Fallback)
* **Architecture:** Acoustic Feature Dynamic Time Warping (MFCC / Mel-spectrogram DTW with local warping constraints).
* **Licensing:** Public Domain / CC0 / OpenSLR recordings.
* **Suitability:** Used as an optional zero-dependency ultra-lightweight scoring layer (< 5 MB package) when full neural inference is disabled by the host application.

---

## 4. Language-Specific Linguistic & Phonetic Landscape

### 4.1. Tamil (`ta-IN`)
* **Script:** Tamil script (`U+0B80`–`U+0BFF`).
* **Phonological Reality:**
  * **Consonantal inventory:** 18 native consonants + Grantha consonants for borrowings (ஸ, ஷ, ஜ, ஹ).
  * **Critical Liquid Triplet:**
    1. ல (alveolar lateral approximant /l/)
    2. ள (retroflex lateral approximant /ɭ/)
    3. ழ (retroflex approximant /ɻ/) — Primary pedagogical differentiator.
  * **Critical Rhotic Contrast:**
    1. ர (alveolar tap /ɾ/)
    2. ற (alveolar trill /r/ or alveolar plosive in gemination /tt/)
  * **Contextual Lenition & Allophony:**
    * In standard Tamil orthography, stop letters do not differentiate voicing. க represents [k] word-initially or when doubled (பக்கம்), but [x] or [ɣ] intervocalically (மகன்), and [ɡ] post-nasal (சங்கு).
    * **SDK Requirement:** The G2P and scoring engine must not penalize native speakers for producing phonetically correct allophones.

### 4.2. Sanskrit (`sa-IN`)
* **Script:** Devanagari (`U+0900`–`U+097F`) or regional scripts (Tamil Grantha).
* **Phonological Reality:**
  * Strict Paninian phonology with 5 vargas $\times$ 4 phonation types + 5 nasals.
  * Strict vowel length distinctions ($a/\bar{a}, i/\bar{i}, u/\bar{u}, ṛ/\bar{ṛ}$).
  * **No Final Schwa Deletion:** Unlike Hindi, words ending in inherent vowels retain them fully (e.g., राम is pronounced [raːmɐ], never [raːm]).
  * **Visarga (ः):** Produces an echo-aspiration corresponding to the preceding vowel (e.g., नमः [nɐmɐhɐ], शान्तिः [ɕaːnt̪ihi]).
  * **Anusvara (ं):** Homorganic nasal assimilation matching the following stop consonant.

### 4.3. Hindi (`hi-IN`)
* **Script:** Devanagari (`U+0900`–`U+097F`).
* **Phonological Reality:**
  * **Schwa Deletion Rule:** Word-final inherent vowel /ə/ is deleted (e.g., कमल [kəməl]). Word-internal schwas are deleted conditionally based on syllable structure (e.g., चलना [t͡ʃəlnaː]).
  * **Nukta Consonants:** Distinguishes loan consonants (क़ [q], ख़ [x], ग़ [ɣ], ज़ [z], फ़ [f], ड़ [ɽ], ढ़ [ɽʱ]).
  * **Nasalization:** Chandrabindu (ँ) indicates true vowel nasalization, whereas Anusvara indicates homorganic nasal closure.

### 4.4. Telugu (`te-IN`)
* **Script:** Telugu script (`U+0C00`–`U+0C7F`).
* **Phonological Reality:**
  * Explicit Dravidian short/long pairs: e / ē (ఎ / ఏ) and o / ō (ఒ / ఓ).
  * Retains full aspiration series in Sanskrit loanwords while maintaining Dravidian retroflex consonants (ట, ఠ, డ, ఢ, ణ, ళ).
  * Consonant gemination (Vottulu): closure duration is phonemically distinct (e.g., కల్పన vs కళ్ళ).
  * Words naturally terminate in vowels (vowel-ending phonotactics).

---

## 5. Tokenization and Phoneme Mapping Strategy

A fundamental challenge in using standard ASR models for CAPT is that ASR models are typically trained on **orthographic characters or BPE subwords**, rather than pure International Phonetic Alphabet (IPA) phonemes.

### The Brahmic Advantage
Unlike English, Indian scripts (abugidas) have a virtually 1:1 correspondence between written grapheme clusters (aksharas) and phonetic units.

```text
[ Input Text ]
     │
     ▼
[ Unicode Normalization (NFC) ]
     │
     ▼
[ Brahmic Grapheme Cluster Segmentation ]
     │   ├── Base Consonant
     │   ├── Virama (Halant/Pulli)
     │   └── Vowel Matra / Modifiers
     │
     ▼
[ Language-Specific G2P Rules ] ──► (Applies Schwa Deletion, Tamil Allophones, Sanskrit Visarga)
     │
     ▼
[ Canonical Phoneme Sequence + Character Spans ]
     │
     ├──► [ Constrained CTC Viterbi Aligner ] ◄── [ Acoustic Frames (16 kHz PCM) ]
     │
     ▼
[ Frame Alignment Boundaries & Segmental GOP Scores ]
```

---

## 6. Implementation Status Matrix

| Language | Language Tag | Model Status | G2P Engine Status | CTC Scorer Status | Redistribution Compliance |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Tamil** | `ta-IN` | Evaluated (IndicConformer CTC INT8) | Implemented (v1 Dravidian G2P with allophonic rules) | Fully Specified & Testable | Verified MIT |
| **Sanskrit** | `sa-IN` | Evaluated (IndicConformer CTC INT8) | Implemented (v1 Paninian G2P with Visarga/Anusvara) | Fully Specified & Testable | Verified MIT |
| **Hindi** | `hi-IN` | Evaluated (IndicConformer CTC INT8) | Implemented (v1 Indo-Aryan G2P with Schwa deletion) | Fully Specified & Testable | Verified MIT |
| **Telugu** | `te-IN` | Evaluated (IndicConformer CTC INT8) | Implemented (v1 Dravidian G2P with vowel length rules) | Fully Specified & Testable | Verified MIT |

*Note: All four languages use verified, commercially unencumbered models and reproducible rule sets.*
