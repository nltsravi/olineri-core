# Pronunciation Assessment & Acoustic Scoring Specification

This document details the mathematical, algorithmic, and linguistic principles governing pronunciation assessment in the SDK.

---

## 1. Why ASR Transcript Equality Fails

Many naive speech applications implement "pronunciation scoring" by running speech-to-text (ASR) and computing Levenshtein edit distance between the decoded transcript and the reference text. **This approach is scientifically invalid for pronunciation training**:

1. **Language Model Bias:** ASR decoders (especially with n-gram or neural LMs) are explicitly trained to find the most probable grammatically coherent sentence. If a learner mispronounces a sound (e.g., saying *பலம்* [palam - strength] instead of *பழம்* [paɻam - fruit]), an ASR decoder may either autocorrect it to the expected word or swap it to a different valid dictionary word.
2. **Coarse Resolution:** Transcript matching provides binary word-level match/mismatch without temporal localization or phoneme-level diagnostic information.
3. **No Acoustic Gradient:** A learner whose pronunciation is 80% intelligible but phonetically imperfect receives a binary pass or fail, without actionable guidance on tongue placement, voicing, or aspiration.

The SDK rejects transcript-matching as an assessment technique. Instead, it measures the **direct acoustic evidence** against canonical phonetic representations using **Forced Alignment** and **Goodness of Pronunciation (GOP)**.

---

## 2. Mathematical Formulation: CTC Forced Alignment & GOP

### 2.1. The Acoustic Matrix
Let an input audio signal $X$ of duration $D$ seconds sampled at 16 kHz be transformed into a sequence of $T$ acoustic feature frames (e.g., 80-dimensional log Mel filterbank coefficients spaced at 10 ms with 25 ms windows).

Passing these features through an acoustic neural network (e.g., Conformer-CTC) produces an emission matrix:
$$\mathbf{L} \in \mathbb{R}^{T \times |\mathcal{V}|}$$
where $\mathcal{V}$ is the vocabulary of tokens (characters/subwords/phonemes) plus the special CTC blank token $\epsilon$.

Applying softmax across vocabulary dimension yields the posterior probabilities:
$$P(k \mid t) = \frac{\exp(L_{t, k})}{\sum_{v \in \mathcal{V}} \exp(L_{t, v})}$$

### 2.2. Constrained CTC Viterbi Forced Alignment
Given the reference text normalized and transformed via G2P into a target sequence of $N$ phonetic units $\mathbf{S} = (s_1, s_2, \dots, s_N)$, we construct the modified sequence $\mathbf{S}'$ of length $2N + 1$ by interleaving blank tokens:
$$\mathbf{S}' = (\epsilon, s_1, \epsilon, s_2, \epsilon, \dots, \epsilon, s_N, \epsilon)$$

Let $\alpha(t, j)$ be the maximum path probability of generating prefix $\mathbf{S}'_{1:j}$ by frame $t$:

$$\alpha(t, j) = P(\mathbf{S}'_j \mid t) \cdot \max \begin{cases}
\alpha(t-1, j) & \text{(stay)} \\
\alpha(t-1, j-1) & \text{(transition from previous)} \\
\alpha(t-1, j-2) & \text{if } \mathbf{S}'_j \neq \epsilon \text{ and } \mathbf{S}'_j \neq \mathbf{S}'_{j-2} \text{ (skip blank)}
\end{cases}$$

At frame $T$, the optimal path is recovered by backtracking from $\arg\max_{j \in \{2N, 2N+1\}} \alpha(T, j)$.

Backtracking identifies the exact start and end frame indices $[t_{\text{start}}(i), t_{\text{end}}(i)]$ for each target phoneme $s_i$.

### 2.3. Goodness of Pronunciation (GOP) Metric
For each target phoneme $s_i$ spanning frames $t \in [t_{\text{start}}(i), t_{\text{end}}(i)]$ with duration $\Delta t_i = t_{\text{end}}(i) - t_{\text{start}}(i) + 1$:

$$\text{GOP}(s_i) = \frac{1}{\Delta t_i} \sum_{t=t_{\text{start}}(i)}^{t_{\text{end}}(i)} \left[ \log P(s_i \mid t) - \max_{q \in \mathcal{V} \setminus \{\epsilon\}} \log P(q \mid t) \right]$$

#### Normalized Likelihood Ratio:
The term $\log P(s_i \mid t) - \max_{q} \log P(q \mid t)$ represents the Log-Posterior Ratio (LPR).
* If the speaker accurately articulated sound $s_i$, then $s_i = \arg\max_q P(q \mid t)$, and $\text{GOP}(s_i) \approx 0$ (or slightly negative due to softmax spread).
* If the speaker substituted another phoneme $q^*$, then $\log P(s_i \mid t) \ll \log P(q^* \mid t)$, producing a large negative score (e.g., $-2.5$ to $-8.0$).

### 2.4. Calibrated Score Mapping
Raw GOP values $[-\infty, 0]$ are mapped into a standardized $[0, 100]$ pedagogical score using a calibrated sigmoid function with language-tuned sensitivity parameters $(\beta, \theta)$:

$$\text{Score}(s_i) = \frac{100}{1 + \exp\left(-\beta \cdot (\text{GOP}(s_i) - \theta)\right)}$$

---

## 3. Error Classification Taxonomy

Every phoneme segment is evaluated and assigned an error classification:

| Classification | Condition | Diagnostic Information |
| :--- | :--- | :--- |
| **CORRECT** | $\text{Score}(s_i) \ge \tau_{\text{pass}}$ (typically $\ge 70$) | Articulation matches canonical target within acoustic margin. |
| **SUBSTITUTION** | $\text{Score}(s_i) < \tau_{\text{pass}}$ AND $\exists q^* \neq s_i$ with high posterior mass | User substituted target $s_i$ with acoustic competitor $q^*$ (e.g. Tamil ழ $\to$ ள or ல). |
| **OMISSION (DELETION)** | $\Delta t_i < t_{\text{min}}$ OR mean acoustic energy in $[t_{\text{start}}, t_{\text{end}}]$ below silence threshold | Target phoneme was skipped or swallowed. |
| **INSERTION** | High posterior mass for non-blank tokens during inter-word silence or consonant pauses | User added an extra intrusive sound (e.g. epenthetic vowel). |
| **DISTORTION / UNCERTAIN** | High acoustic variance or SNR $< 10\text{ dB}$ | Audio quality or background noise makes assessment inconclusive. Reported as `UNCERTAIN` rather than false negative. |

---

## 4. Grapheme-to-Phoneme (G2P) & Brahmic Akshara Alignment

In Brahmic scripts, characters do not represent isolated letters; they represent **Aksharas** (orthographic syllabic units).

### 4.1. Unicode Grapheme Cluster Structure
An Akshara consists of:
$$\text{Akshara} = C_1 + (\text{Virama} + C_2)^* + (V_{\text{matra}})? + (M)?$$
Where:
* $C$: Base Consonant (e.g. Tamil க, Devanagari क)
* $\text{Virama}$: Vowel-cancelling sign (Tamil Pulli ் `U+0BCD`, Devanagari Halant ् `U+094D`, Telugu Virama ్ `U+0C4D`)
* $V_{\text{matra}}$: Dependent vowel sign (e.g. Tamil ா, ி, ீ, ு; Devanagari ा, ि, ी, ु)
* $M$: Modifiers (Anusvara ं, Visarga ः, Chandrabindu ँ)

### 4.2. Bi-Directional Character-to-Phoneme Mapping
To guarantee that errors in phonemes highlight the exact corresponding characters in hybrid mobile UIs, the G2P engine constructs explicit bidirectional coordinate spans:

```cpp
struct PhonemeSpan {
    std::string phoneme_ipa;      // e.g. "/ɻ/"
    std::string phoneme_label;    // Model vocabulary token
    uint32_t char_start_utf8;     // Byte offset in UTF-8
    uint32_t char_end_utf8;       // Byte offset in UTF-8
    uint32_t cluster_index;       // Grapheme cluster index for UI highlighting
    uint32_t cluster_count;       // Length in grapheme clusters
};
```

---

## 5. Language-Specific Phonetic Contrasts & Pedagogical Rules

### 5.1. Tamil Liquid Triplet
* **Target: ழ (`U+0BB4`, /ɻ/)** — Retroflex approximant.
  * Correct articulation: Tongue tip curled backward towards the hard palate without touching the roof of the mouth; air flows smoothly along the sides.
  * Common error: Substituting alveolar lateral ல (/l/) or retroflex lateral ள (/ɭ/).
  * Feedback: "Curl the tip of your tongue back towards the roof of your mouth, but do not touch the palate."
* **Target: ற (`U+0BB1`, /r/ or /tt/)** vs **ர (`U+0BB0`, /ɾ/)**:
  * ர is an alveolar flap/tap; ற is an alveolar trill or geminate voiceless stop.

### 5.2. Sanskrit Consonant Aspiration & Visarga
* **Aspiration Contrast:** क (/k/) vs ख (/kʰ/), ग (/ɡ/) vs घ (/ɡʱ/).
  * Feedback: "Release a distinct burst of air (aspiration) from the vocal tract."
* **Visarga (ः):**
  * When following 'a': echoed as [ɐhɐ] (e.g. नमः [nɐmɐhɐ]).
  * When following 'i': echoed as [ihi] (e.g. शान्तिः [ɕaːnt̪ihi]).

### 5.3. Hindi Schwa Deletion
* **Rule:** If word length $> 2$ and ends in an un-geminated consonant without matra, final inherent /ə/ is omitted (e.g. कमल $\to$ /kəməl/, not /kəmələ/).
* **Internal Deletion:** In morphemes with structure $V C_1 ə C_2 V$, the schwa is deleted: चलना $\to$ /t͡ʃəlnaː/.

### 5.4. Telugu Vowel Length & Retroflexes
* **Short vs Long Vowels:**
  * ఎ (/e/) vs ఏ (/eː/)
  * ఒ (/o/) vs ఓ (/oː/)
* **Retroflexes:**
  * ట (/ʈ/), ఠ (/ʈʰ/), డ (/ɖ/), ఢ (/ɖʱ/), ణ (/ɳ/), ళ (/ɭ/).

---

## 6. Uncertainty & Safety Guardrails

To prevent pedagogical discouragement caused by noisy environments or hardware defects:
1. **Audio Level Check:** If RMS energy $< -45\text{ dBFS}$, assessment returns `ERROR_AUDIO_TOO_QUIET`.
2. **Clipping Detection:** If $> 1\%$ of audio samples are at full-scale value ($\pm 1.0$), returns `ERROR_AUDIO_CLIPPED`.
3. **SNR Estimation:** If signal-to-noise ratio $< 12\text{ dB}$, individual phoneme scores below threshold are flagged as `UNCERTAIN` rather than `INCORRECT`.
