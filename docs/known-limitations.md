# Vaani Core SDK — Known Limitations & Technical Constraints

In adherence to strict engineering honesty, this document outlines known limitations, trade-offs, and boundary conditions in **v0.1.0**.

---

## 1. Acoustic & Environmental Constraints

1. **Acoustic Noise & Low-SNR Environments:**
   - Pronunciation assessment relies on frame-level acoustic posteriors. In environments with heavy background babble, ambient music, or signal-to-noise ratio (SNR) $< 12\text{ dB}$, acoustic likelihoods drop.
   - **Mitigation:** The engine classifies segments with low posterior confidence as `VAANI_PHONEME_UNCERTAIN` rather than asserting a false substitution. Applications should prompt the user to record in a quieter setting.
2. **Audio Sample Rate and Channels:**
   - The IndicConformer CTC model requires 16,000 Hz, 16-bit mono PCM. Audio recorded at 44.1 kHz or 48 kHz must be resampled prior to inference.

---

## 2. Dialectal & Regional Variations

Indian languages feature rich dialectal variations that do not necessarily represent learner errors:

1. **Tamil:**
   - In spoken colloquial Tamil (as opposed to Senthamil), intervocalic stops often undergo lenition (`/k/` $\to$ `[x]` or `[ɣ]`), and word-final nasals frequently nasalize the preceding vowel rather than forming a closed coda.
   - In Sri Lankan / Jaffna Tamil, retroflex sounds and archaic contrasts are preserved differently than in Tamil Nadu dialects.
2. **Hindi:**
   - Regional Hindi speakers from eastern regions (Bihar, UP) may pronounce dental `/s/` for palatal `/ʃ/` (श) or vice-versa.
   - Schwa deletion in Sanskrit loanwords (Tatsama) differs from colloquial Hindustani (Tadbhav). While `vaani-core` applies standard 2-tier rules, irregular Sanskrit borrowings may retain internal schwas.
3. **Telugu:**
   - Coastal Andhra, Rayalaseema, and Telangana dialects exhibit variations in vowel openness and aspirate strength. Spoken Telugu frequently elides certain unaccented vowels in fast conversational speech.

---

## 3. INT8 Quantization Trade-offs

- The IndicConformer model is quantized from FP32 to INT8 to reduce model size from ~380 MB to **~48–52 MB**, enabling deployment on resource-constrained mobile hardware.
- While recognition accuracy (WER) drops by $< 1.2\%$ relative to FP32, low-energy phonemic contrasts (such as subtle aspiration on voiceless stops like `/p/` vs `/pʰ/`) exhibit slightly tighter posterior margins in INT8.
- Threshold calibrations have been tuned to prevent false negatives.

---

## 4. Script & Transliteration Limitations

- **Sanskrit:** The initial engine processes Sanskrit in Devanagari script. While Tamil-script Sanskrit (Grantha conventions) is structurally supported by the C ABI architecture, full Grantha ligatures require unicode decomposition preprocessing.
- **English/Latin Transliteration (IAST / ISO 15919):** Not included in the core v0.1.0 runtime; applications must convert IAST to native Brahmic scripts prior to calling `vaani_assess_pronunciation`.

---

## 5. Pediatric Speech (Children's Voices)

- Acoustic models trained primarily on adult voices exhibit higher pitch variance when evaluating children under age 8 due to shorter vocal tract lengths and higher fundamental frequency ($F_0$).
- Fine-tuning packages on consented pediatric speech data will be offered in subsequent language updates.
