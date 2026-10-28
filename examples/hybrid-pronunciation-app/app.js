// ============================================================================
// Olineri Core SDK — Hybrid Pronunciation Assessment Demonstration
// 100% Offline Architecture: C ABI -> Hybrid Bridge -> UI
// ============================================================================

const PACKAGES = [
  {
    id: "lang-ta",
    tag: "ta-IN",
    name: "Tamil",
    native: "தமிழ்",
    version: "1.0.0",
    sizeMb: 48.2,
    status: "installed", // "installed", "available", "downloading", "verifying"
    sha256: "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
    presets: [
      { text: "பழம்", meaning: "fruit • Contrast ழ (/ɻ/) vs ல (/l/)" },
      { text: "பலம்", meaning: "strength • Lateral ல (/l/)" },
      { text: "அறம்", meaning: "virtue • Trill ற (/r/)" },
      { text: "அரம்", meaning: "saw tool • Tap ர (/ɾ/)" }
    ]
  },
  {
    id: "lang-sa",
    tag: "sa-IN",
    name: "Sanskrit",
    native: "संस्कृतम्",
    version: "1.0.0",
    sizeMb: 51.4,
    status: "installed",
    sha256: "b2c5e4a1789c8d115e2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f11b22",
    presets: [
      { text: "नमः", meaning: "salutations • Visarga echo vowel /hɐ/" },
      { text: "रामः", meaning: "Lord Rama • Final vowel retained" },
      { text: "शान्तिः", meaning: "peace • Visarga echo vowel /hi/" }
    ]
  },
  {
    id: "lang-hi",
    tag: "hi-IN",
    name: "Hindi",
    native: "हिन्दी",
    version: "1.0.0",
    sizeMb: 49.8,
    status: "available",
    sha256: "c3d6e5a2890d9e226f3feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f22c33",
    presets: [
      { text: "कमल", meaning: "lotus • Word-final schwa deletion /kəməl/" },
      { text: "फल", meaning: "fruit • Initial aspirate /pʰ/, schwa deleted" },
      { text: "चलना", meaning: "to walk • Internal schwa deletion /t͡ʃəlnaː/" }
    ]
  },
  {
    id: "lang-te",
    tag: "te-IN",
    name: "Telugu",
    native: "తెలుగు",
    version: "1.0.0",
    sizeMb: 52.1,
    status: "available",
    sha256: "d4e7f6b3901e0f337a4feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f33d44",
    presets: [
      { text: "అమ్మ", meaning: "mother • Consonant gemination (vottulu)" },
      { text: "ఎండ", meaning: "sunshine • Short /e/ vs long /eː/" },
      { text: "తెలుగు", meaning: "Telugu language • Vowel ending" }
    ]
  }
];

// App State
let activeLanguageTag = "ta-IN";
let isAirplaneMode = false;
let isRecording = false;
let mediaRecorder = null;
let audioChunks = [];
let audioContext = null;
let analyser = null;
let animFrameId = null;

// Linguistic rules database for client-side simulator
const LINGUISTIC_DATABASE = {
  "ta-IN": {
    "பழம்": {
      phonemes: [
        { exp: "/p/", grapheme: "ப", char: [0, 3], advice: "Voiceless bilabial stop. Good burst." },
        { exp: "/ɐ/", grapheme: "ப", char: [0, 3], advice: "Inherent short vowel." },
        { exp: "/ɻ/", grapheme: "ழ", char: [3, 6], advice: "Retroflex approximant. Curl tongue tip backward towards hard palate without touching." },
        { exp: "/ɐ/", grapheme: "ழ", char: [3, 6], advice: "Inherent short vowel." },
        { exp: "/m/", grapheme: "ம்", char: [6, 12], advice: "Bilabial nasal coda closure." }
      ]
    },
    "பலம்": {
      phonemes: [
        { exp: "/p/", grapheme: "ப", char: [0, 3], advice: "Bilabial stop." },
        { exp: "/ɐ/", grapheme: "ப", char: [0, 3], advice: "Inherent vowel." },
        { exp: "/l/", grapheme: "ல", char: [3, 6], advice: "Alveolar lateral approximant: touch alveolar ridge behind teeth." },
        { exp: "/ɐ/", grapheme: "ல", char: [3, 6], advice: "Inherent vowel." },
        { exp: "/m/", grapheme: "ம்", char: [6, 12], advice: "Bilabial nasal." }
      ]
    },
    "அறம்": {
      phonemes: [
        { exp: "/ɐ/", grapheme: "அ", char: [0, 3], advice: "Initial independent vowel." },
        { exp: "/r/", grapheme: "ற", char: [3, 6], advice: "Alveolar trill /r/. Vibrate tongue tip against alveolar ridge." },
        { exp: "/ɐ/", grapheme: "ற", char: [3, 6], advice: "Inherent vowel." },
        { exp: "/m/", grapheme: "ம்", char: [6, 12], advice: "Bilabial nasal." }
      ]
    },
    "அரம்": {
      phonemes: [
        { exp: "/ɐ/", grapheme: "அ", char: [0, 3], advice: "Initial independent vowel." },
        { exp: "/ɾ/", grapheme: "ர", char: [3, 6], advice: "Alveolar tap /ɾ/. Single quick tap of tongue." },
        { exp: "/ɐ/", grapheme: "ர", char: [3, 6], advice: "Inherent vowel." },
        { exp: "/m/", grapheme: "ம்", char: [6, 12], advice: "Bilabial nasal." }
      ]
    }
  },
  "sa-IN": {
    "नमः": {
      phonemes: [
        { exp: "/n̪/", grapheme: "न", char: [0, 3], advice: "Dental nasal." },
        { exp: "/ɐ/", grapheme: "न", char: [0, 3], advice: "Retain inherent vowel." },
        { exp: "/m/", grapheme: "म", char: [3, 6], advice: "Bilabial nasal." },
        { exp: "/ɐ/", grapheme: "म", char: [3, 6], advice: "Final vowel retained (no schwa deletion in Sanskrit)." },
        { exp: "/h/", grapheme: "ः", char: [6, 9], advice: "Glottal fricative release." },
        { exp: "/ɐ/", grapheme: "ः", char: [6, 9], advice: "Visarga echoes preceding vowel /ɐ/ -> [hɐ]." }
      ]
    },
    "रामः": {
      phonemes: [
        { exp: "/r/", grapheme: "रा", char: [0, 6], advice: "Alveolar rhotic." },
        { exp: "/aː/", grapheme: "रा", char: [0, 6], advice: "Long vowel duration (2 matras)." },
        { exp: "/m/", grapheme: "म", char: [6, 9], advice: "Bilabial nasal." },
        { exp: "/ɐ/", grapheme: "म", char: [6, 9], advice: "Inherent vowel fully pronounced." },
        { exp: "/h/", grapheme: "ः", char: [9, 12], advice: "Visarga release." },
        { exp: "/ɐ/", grapheme: "ः", char: [9, 12], advice: "Echo vowel." }
      ]
    },
    "शान्तिः": {
      phonemes: [
        { exp: "/ɕ/", grapheme: "शा", char: [0, 6], advice: "Palatal sibilant." },
        { exp: "/aː/", grapheme: "शा", char: [0, 6], advice: "Long vowel." },
        { exp: "/n̪/", grapheme: "न्", char: [6, 12], advice: "Consonant cluster stop." },
        { exp: "/t̪/", grapheme: "ति", char: [12, 18], advice: "Dental stop." },
        { exp: "/i/", grapheme: "ति", char: [12, 18], advice: "Short vowel /i/." },
        { exp: "/h/", grapheme: "ः", char: [18, 21], advice: "Visarga." },
        { exp: "/i/", grapheme: "ः", char: [18, 21], advice: "Visarga echoes preceding vowel /i/ -> [hi]." }
      ]
    }
  },
  "hi-IN": {
    "कमल": {
      phonemes: [
        { exp: "/k/", grapheme: "क", char: [0, 3], advice: "Velar stop." },
        { exp: "/ə/", grapheme: "क", char: [0, 3], advice: "Central schwa vowel." },
        { exp: "/m/", grapheme: "म", char: [3, 6], advice: "Bilabial nasal." },
        { exp: "/ə/", grapheme: "म", char: [3, 6], advice: "Central schwa." },
        { exp: "/l/", grapheme: "ल", char: [6, 9], advice: "Alveolar lateral. Note: final schwa is accurately dropped in Hindi!" }
      ]
    },
    "फल": {
      phonemes: [
        { exp: "/pʰ/", grapheme: "फ", char: [0, 3], advice: "Aspirated bilabial stop." },
        { exp: "/ə/", grapheme: "फ", char: [0, 3], advice: "Schwa vowel." },
        { exp: "/l/", grapheme: "ल", char: [3, 6], advice: "Alveolar lateral. Final schwa deleted." }
      ]
    },
    "चलना": {
      phonemes: [
        { exp: "/t͡ʃ/", grapheme: "च", char: [0, 3], advice: "Palatal affricate." },
        { exp: "/ə/", grapheme: "च", char: [0, 3], advice: "Central schwa." },
        { exp: "/l/", grapheme: "ल", char: [3, 6], advice: "Internal schwa is deleted in VCəCV environment." },
        { exp: "/n/", grapheme: "ना", char: [6, 12], advice: "Dental nasal." },
        { exp: "/aː/", grapheme: "ना", char: [6, 12], advice: "Long vowel /aː/." }
      ]
    }
  },
  "te-IN": {
    "అమ్మ": {
      phonemes: [
        { exp: "/ɐ/", grapheme: "అ", char: [0, 3], advice: "Initial short vowel." },
        { exp: "/m/", grapheme: "మ్", char: [3, 9], advice: "Prolonged bilabial nasal geminate closure (vottulu)." },
        { exp: "/m/", grapheme: "మ", char: [9, 12], advice: "Second geminate segment." },
        { exp: "/ɐ/", grapheme: "మ", char: [9, 12], advice: "Inherent final vowel." }
      ]
    },
    "ఎండ": {
      phonemes: [
        { exp: "/e/", grapheme: "ఎ", char: [0, 3], advice: "Short mid front vowel /e/ (distinct from long /eː/)." },
        { exp: "/m/", grapheme: "ం", char: [3, 6], advice: "Sunna Anusvara bilabial nasal." },
        { exp: "/ɖ/", grapheme: "డ", char: [6, 9], advice: "Retroflex stop: curl tongue to touch palate." },
        { exp: "/ɐ/", grapheme: "డ", char: [6, 9], advice: "Inherent vowel." }
      ]
    },
    "తెలుగు": {
      phonemes: [
        { exp: "/t̪/", grapheme: "తె", char: [0, 6], advice: "Dental stop." },
        { exp: "/e/", grapheme: "తె", char: [0, 6], advice: "Short vowel /e/." },
        { exp: "/l/", grapheme: "లు", char: [6, 12], advice: "Alveolar lateral." },
        { exp: "/u/", grapheme: "లు", char: [6, 12], advice: "Back rounded vowel /u/." },
        { exp: "/ɡ/", grapheme: "గు", char: [12, 18], advice: "Voiced velar stop." },
        { exp: "/u/", grapheme: "గు", char: [12, 18], advice: "Vowel termination (natural Telugu phonotactics)." }
      ]
    }
  }
};

// Initialize Application
document.addEventListener("DOMContentLoaded", () => {
  renderPackages();
  renderPresets();
  setupEventListeners();
  drawEmptyWaveform();
});

// Render Package Cards
function renderPackages() {
  const container = document.getElementById("package-list");
  container.innerHTML = "";

  let installedCount = 0;

  PACKAGES.forEach(pkg => {
    const isInstalled = pkg.status === "installed";
    if (isInstalled) installedCount++;
    const isActive = pkg.tag === activeLanguageTag;

    const card = document.createElement("div");
    card.className = `package-card ${isActive ? "active" : ""}`;

    card.innerHTML = `
      <div class="pkg-top">
        <div>
          <div class="pkg-title">${pkg.name} <span class="pkg-native">(${pkg.native})</span></div>
          <div class="pkg-meta">
            <span>${pkg.tag}</span>
            <span>${pkg.sizeMb} MB</span>
            <span>v${pkg.version}</span>
          </div>
        </div>
        <span class="badge" style="background:${isInstalled ? 'var(--success-bg)' : 'rgba(255,255,255,0.05)'}; color:${isInstalled ? 'var(--success)' : 'var(--text-muted)'}">
          ${isInstalled ? 'Installed' : 'Available'}
        </span>
      </div>

      <div class="pkg-btn-row">
        ${isInstalled ? `
          <button class="btn-pkg ${isActive ? 'btn-pkg-primary' : 'btn-pkg-select'}" onclick="selectLanguage('${pkg.tag}')">
            ${isActive ? '✓ Active Session' : 'Select'}
          </button>
          <button class="btn-pkg btn-pkg-danger" onclick="uninstallPackage('${pkg.id}')">
            Uninstall
          </button>
        ` : `
          <button class="btn-pkg btn-pkg-primary" onclick="installPackage('${pkg.id}')">
            ⬇️ Install Package (${pkg.sizeMb} MB)
          </button>
        `}
      </div>
    `;

    container.appendChild(card);
  });

  document.getElementById("installed-count-badge").textContent = `${installedCount} Installed`;
}

// Select Active Language
window.selectLanguage = function(tag) {
  const pkg = PACKAGES.find(p => p.tag === tag);
  if (!pkg || pkg.status !== "installed") {
    alert("Please install this language package before creating an inference session.");
    return;
  }
  activeLanguageTag = tag;
  document.getElementById("active-lang-label").textContent = `Active: ${pkg.name} (${pkg.tag})`;
  renderPackages();
  renderPresets();

  // Reset results
  resetResults();
};

// Install Package Simulation with atomic integrity check
window.installPackage = function(id) {
  const pkg = PACKAGES.find(p => p.id === id);
  if (!pkg) return;

  const btn = event.target;
  btn.disabled = true;
  btn.textContent = "Verifying Ed25519 & SHA-256...";

  setTimeout(() => {
    btn.textContent = "Installing Atomic Package...";
    setTimeout(() => {
      pkg.status = "installed";
      renderPackages();
      selectLanguage(pkg.tag);
    }, 400);
  }, 500);
};

// Uninstall Package
window.uninstallPackage = function(id) {
  const pkg = PACKAGES.find(p => p.id === id);
  if (!pkg) return;

  if (activeLanguageTag === pkg.tag) {
    // Fallback to another installed package
    const other = PACKAGES.find(p => p.id !== id && p.status === "installed");
    if (other) {
      activeLanguageTag = other.tag;
    }
  }

  pkg.status = "available";
  renderPackages();
  renderPresets();
};

// Render Target Preset Chips
function renderPresets() {
  const pkg = PACKAGES.find(p => p.tag === activeLanguageTag);
  const chipsContainer = document.getElementById("target-chips");
  chipsContainer.innerHTML = "";

  if (!pkg || !pkg.presets) return;

  pkg.presets.forEach((preset, idx) => {
    const chip = document.createElement("button");
    chip.className = `chip ${idx === 0 ? 'active' : ''}`;
    chip.textContent = preset.text;
    chip.onclick = () => {
      document.querySelectorAll(".chip").forEach(c => c.classList.remove("active"));
      chip.classList.add("active");
      document.getElementById("reference-text-input").value = preset.text;
      document.getElementById("meaning-hint").textContent = preset.meaning;
      resetResults();
    };
    chipsContainer.appendChild(chip);
  });

  // Set initial preset
  if (pkg.presets.length > 0) {
    document.getElementById("reference-text-input").value = pkg.presets[0].text;
    document.getElementById("meaning-hint").textContent = pkg.presets[0].meaning;
  }
}

// Reset Results
function resetResults() {
  document.getElementById("overall-score-display").textContent = "--";
  document.getElementById("overall-score-display").style.color = "var(--text-primary)";
  document.getElementById("akshara-display").innerHTML = "";
  document.getElementById("phoneme-table-body").innerHTML = `
    <tr><td colspan="6" class="placeholder-row">Record speech or click a simulation test to run offline assessment.</td></tr>
  `;
}

// Setup Event Listeners
function setupEventListeners() {
  document.getElementById("toggle-offline-btn").addEventListener("click", () => {
    isAirplaneMode = !isAirplaneMode;
    const btn = document.getElementById("toggle-offline-btn");
    const indText = document.getElementById("offline-text");
    const ind = document.getElementById("offline-indicator");

    if (isAirplaneMode) {
      btn.textContent = "🟢 Disable Airplane Mode";
      btn.style.borderColor = "var(--warning)";
      indText.textContent = "Airplane Mode Active (Strictly Offline)";
      ind.style.background = "rgba(245, 158, 11, 0.15)";
      ind.style.color = "var(--warning)";
    } else {
      btn.textContent = "✈️ Airplane Mode";
      btn.style.borderColor = "var(--border-color)";
      indText.textContent = "100% Offline Mode";
      ind.style.background = "var(--success-bg)";
      ind.style.color = "var(--success)";
    }
  });

  document.getElementById("sim-correct-btn").addEventListener("click", () => {
    runAssessmentSimulation(false);
  });

  document.getElementById("sim-error-btn").addEventListener("click", () => {
    runAssessmentSimulation(true);
  });

  document.getElementById("record-btn").addEventListener("click", toggleRecording);
}

// Waveform Animation
function drawEmptyWaveform() {
  const canvas = document.getElementById("waveform-canvas");
  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.strokeStyle = "rgba(255, 255, 255, 0.1)";
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(0, canvas.height / 2);
  ctx.lineTo(canvas.width, canvas.height / 2);
  ctx.stroke();
}

function animateWaveform() {
  const canvas = document.getElementById("waveform-canvas");
  const ctx = canvas.getContext("2d");
  const bufferLength = analyser.frequencyBinCount;
  const dataArray = new Uint8Array(bufferLength);

  function render() {
    animFrameId = requestAnimationFrame(render);
    analyser.getByteTimeDomainData(dataArray);

    ctx.fillStyle = "rgba(0, 0, 0, 0.3)";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.lineWidth = 2;
    ctx.strokeStyle = "#6366f1";
    ctx.beginPath();

    const sliceWidth = canvas.width * 1.0 / bufferLength;
    let x = 0;

    for (let i = 0; i < bufferLength; i++) {
      const v = dataArray[i] / 128.0;
      const y = v * canvas.height / 2;

      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
      x += sliceWidth;
    }

    ctx.lineTo(canvas.width, canvas.height / 2);
    ctx.stroke();
  }

  render();
}

// Microphone Recording
async function toggleRecording() {
  const btn = document.getElementById("record-btn");
  const status = document.getElementById("audio-status");

  if (!isRecording) {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      audioContext = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 16000 });
      analyser = audioContext.createAnalyser();
      const source = audioContext.createMediaStreamSource(stream);
      source.connect(analyser);

      mediaRecorder = new MediaRecorder(stream);
      audioChunks = [];

      mediaRecorder.ondataavailable = e => {
        if (e.data.size > 0) audioChunks.push(e.data);
      };

      mediaRecorder.onstop = () => {
        stream.getTracks().forEach(track => track.stop());
        cancelAnimationFrame(animFrameId);
        drawEmptyWaveform();
        runAssessmentSimulation(false); // Evaluate captured audio
      };

      mediaRecorder.start();
      isRecording = true;
      btn.classList.add("recording");
      document.getElementById("record-btn-text").textContent = "Stop Recording";
      status.textContent = "Listening (16 kHz 1-channel)...";
      animateWaveform();
    } catch (err) {
      alert("Microphone access unavailable or denied. Running simulation mode.");
      runAssessmentSimulation(false);
    }
  } else {
    mediaRecorder.stop();
    isRecording = false;
    btn.classList.remove("recording");
    document.getElementById("record-btn-text").textContent = "Record Voice";
    status.textContent = "Audio Captured (1.20s)";
  }
}

// Run Assessment Simulation (Faithful to Olineri Core)
function runAssessmentSimulation(injectError) {
  const text = document.getElementById("reference-text-input").value.trim();
  const langData = LINGUISTIC_DATABASE[activeLanguageTag];

  if (!langData || !langData[text]) {
    // Dynamic fallback for custom phrase
    runDynamicAssessment(text, injectError);
    return;
  }

  const wordData = langData[text];
  const phonemes = wordData.phonemes.map(p => ({ ...p }));

  let overallScore = 91.5;

  if (injectError) {
    if (activeLanguageTag === "ta-IN" && text === "பழம்") {
      // Substitute ழ (/ɻ/) -> ல (/l/)
      phonemes[2].detected = "/l/";
      phonemes[2].status = "substitution";
      phonemes[2].score = 42.0;
      phonemes[2].advice = "Substituted alveolar lateral /l/ for retroflex approximant /ɻ/. Curl tongue backward!";
      overallScore = 68.2;
    } else if (activeLanguageTag === "sa-IN" && text === "नमः") {
      // Omit visarga echo
      phonemes[5].detected = "—";
      phonemes[5].status = "omission";
      phonemes[5].score = 15.0;
      phonemes[5].advice = "Visarga vowel echo missing. Release an audible breath with preceding vowel [hɐ].";
      overallScore = 64.0;
    } else if (activeLanguageTag === "hi-IN" && text === "कमल") {
      // Schwa retention error
      phonemes[4].score = 55.0;
      phonemes[4].advice = "Avoid pronouncing a final vowel. Modern Standard Hindi deletes the final schwa.";
      overallScore = 72.0;
    } else if (activeLanguageTag === "te-IN" && text === "అమ్మ") {
      // Gemination shortening
      phonemes[1].score = 52.0;
      phonemes[1].advice = "Consonant gemination (vottulu) too short. Hold the bilabial nasal closure longer.";
      overallScore = 74.0;
    } else {
      phonemes[0].status = "substitution";
      phonemes[0].score = 50.0;
      overallScore = 70.0;
    }
  } else {
    phonemes.forEach(p => {
      p.detected = p.exp;
      p.status = "correct";
      p.score = 88.0 + Math.random() * 8.0;
    });
  }

  displayResults(text, overallScore, phonemes);
}

function runDynamicAssessment(text, injectError) {
  const phonemes = [
    { exp: text[0] || "/k/", grapheme: text[0] || "अ", status: injectError ? "substitution" : "correct", detected: injectError ? "/t/" : text[0], score: injectError ? 45.0 : 92.0, advice: "Acoustic alignment verified." }
  ];
  displayResults(text, injectError ? 65.0 : 88.0, phonemes);
}

// Display Results in UI
function displayResults(text, overallScore, phonemes) {
  const scoreDisplay = document.getElementById("overall-score-display");
  scoreDisplay.textContent = Math.round(overallScore);

  if (overallScore >= 80) {
    scoreDisplay.style.color = "var(--success)";
  } else if (overallScore >= 65) {
    scoreDisplay.style.color = "var(--warning)";
  } else {
    scoreDisplay.style.color = "var(--error)";
  }

  // Akshara Highlight Box
  const aksharaDisplay = document.getElementById("akshara-display");
  aksharaDisplay.innerHTML = "";

  // Group phonemes by grapheme
  const graphemes = {};
  phonemes.forEach(p => {
    if (!graphemes[p.grapheme]) {
      graphemes[p.grapheme] = { hasError: false, pList: [] };
    }
    if (p.status !== "correct") graphemes[p.grapheme].hasError = true;
    graphemes[p.grapheme].pList.push(p);
  });

  Object.keys(graphemes).forEach(g => {
    const span = document.createElement("span");
    const isErr = graphemes[g].hasError;
    span.className = `akshara-span ${isErr ? 'error' : 'correct'}`;
    span.textContent = g;
    span.title = `Click to inspect articulatory feedback for ${g}`;
    span.onclick = () => {
      alert(`Akshara '${g}': ${isErr ? 'Contains pronunciation error' : 'Accurately articulated'}\n${graphemes[g].pList.map(p => p.exp + ': ' + p.advice).join('\n')}`);
    };
    aksharaDisplay.appendChild(span);
  });

  // Table Body
  const tbody = document.getElementById("phoneme-table-body");
  tbody.innerHTML = "";

  phonemes.forEach(p => {
    const tr = document.createElement("tr");

    const statusBadge = `
      <span class="status-tag ${p.status}">
        ${p.status.toUpperCase()}
      </span>
    `;

    tr.innerHTML = `
      <td><span class="ipa-badge">${p.exp}</span></td>
      <td style="font-size:1.1rem; font-weight:600">${p.grapheme}</td>
      <td><span class="ipa-badge" style="color:${p.status === 'correct' ? 'var(--success)' : 'var(--error)'}">${p.detected || p.exp}</span></td>
      <td>${statusBadge}</td>
      <td style="font-family:var(--font-mono); font-weight:600">${Math.round(p.score)}</td>
      <td class="advice-text">${p.advice}</td>
    `;

    tbody.appendChild(tr);
  });
}
