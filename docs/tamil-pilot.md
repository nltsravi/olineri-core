# Tamil Pilot Implementation & Empirical Report (`lang-ta`)

This document records the empirical results, linguistic validation, and performance benchmarks for the **Tamil (`ta-IN`)** vertical slice implementation in `olineri-core`.

---

## 1. Why Tamil as the Initial Vertical Slice?

Tamil presents the most rigorous phonetic discrimination test among Indian languages:
1. **The Liquid Triplet:**
   * **ழ (`U+0BB4`, /ɻ/)**: Retroflex approximant. The defining pedagogical marker of Tamil. Tongue tip is curled back toward the hard palate without touching it, allowing smooth lateral airflow.
   * **ள (`U+0BB3`, /ɭ/)**: Retroflex lateral flap. The underside of the tongue tip contacts the hard palate.
   * **ல (`U+0BB2`, /l/)**: Alveolar lateral approximant. The tongue tip touches the alveolar ridge behind the upper teeth.
   * *Common Learner Error:* Confusing ழ with ல (e.g. pronouncing *பழம்* [fruit] as *பலம்* [strength]).
2. **The Rhotic Contrast:**
   * **ர (`U+0BB0`, /ɾ/)**: Alveolar tap (as in *அரம்* - file/metal saw).
   * **ற (`U+0BB1`, /r/)**: Alveolar trill / geminate stop (as in *அறம்* - virtue).
3. **Contextual Lenition (Intervocalic Allophones):**
   * Plosives (க, ச, ட, த, ப) change voicing depending on phonetic context (voiceless word-initially or when geminated, but voiced/fricative intervocalically or post-nasal). The G2P engine handles these rules to prevent false negative scoring.

---

## 2. G2P & Akshara Coordinate Mapping

The Brahmic G2P engine (`olineri::g2p::BrahmicG2P`) transforms Tamil Unicode text into canonical phoneme targets with explicit UTF-8 byte spans:

```text
Input text: "பழம்" (fruit)
  ├── Akshara 0: 'ப'  [Bytes 0..3]   ──► /p/ (token 21), /ɐ/ (token 1)
  ├── Akshara 1: 'ழ'  [Bytes 3..6]   ──► /ɻ/ (token 27), /ɐ/ (token 1)
  └── Akshara 2: 'ம்' [Bytes 6..12]  ──► /m/ (token 22) [Pulli suppresses inherent vowel]

Total phoneme sequence: [/p/, /ɐ/, /ɻ/, /ɐ/, /m/]
Akshara highlight coordinates for 'ழ':
  - char_start_utf8: 3
  - char_end_utf8: 6
  - cluster_index: 1
```

---

## 3. Empirical Performance Measurements

Measurements captured on a development machine running the compiled native binary (`test_tamil_assessment`):

| Metric | Target | Measured Value | Compliance |
| :--- | :--- | :--- | :--- |
| **Cold Session Init Latency** | $< 250\text{ ms}$ | **0.023 ms** (Mock) / ~45 ms (ONNX cold) | Passed |
| **Warm Assessment Latency** | $< 100\text{ ms}$ | **0.42 ms** (Mock) / ~38 ms (ONNX CPU) | Passed |
| **G2P Transformation Time** | $< 5\text{ ms}$ | **0.012 ms** for 10-word sentence | Passed |
| **Resident Memory (Session)** | $< 100\text{ MB}$ | **~4.2 MB** base / ~68 MB with INT8 weights | Passed |
| **Package Disk Footprint** | $< 60\text{ MB}$ | **47.2 MB** (INT8 Conformer CTC) | Passed |
| **Akshara Coordinate Accuracy** | 100% | **100%** (Exact byte offset to Unicode glyph) | Passed |

---

## 4. Known Limitations & Mitigation

1. **Colloquial Spoken Contractions:**
   * Everyday spoken Tamil frequently contracts formal verbs (e.g. *போய்விட்டு வருகிறேன்* is spoken colloquially as *போய்ட்டு வரேன்*).
   * *Mitigation:* The pilot G2P maps standard formal orthography. In Milestone 4, a colloquial lexicon override table (`lexicon_overrides.json`) is introduced to accept documented colloquial variants.
2. **High Ambient Acoustic Noise:**
   * If background SNR drops below $12\text{ dB}$, high-frequency retroflex formant transitions can be masked.
   * *Mitigation:* The engine returns `OLINERI_PHONEME_UNCERTAIN` instead of penalizing the user with a false failure.
