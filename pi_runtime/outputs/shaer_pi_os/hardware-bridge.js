(function () {
  const keyForAction = {
    left: "ArrowLeft",
    right: "ArrowRight",
    select: "Enter",
    back: "Backspace",
    home: "Escape"
  };
  const themeByPath = {
    shaer_base_dark: "base-dark",
    shaer_base_light: "base-light",
    shaer_dark_archive: "archive",
    shaer_bombay_ticket: "bombay",
    shaer_japanese_punk: "punk",
    shaer_windows_xp: "xp",
    shaer_ghibli_garden: "garden",
    shaer_indian_print: "raga"
  };
  const pathTheme = Object.entries(themeByPath).find(([path]) => window.location.pathname.includes(path));
  const theme = pathTheme ? pathTheme[1] : "archive";
  const runtimeParams = new URLSearchParams(window.location.search);
  const deviceMode = runtimeParams.get("mode") === "device";
  const diagnosticSource = runtimeParams.get("diagnostic");
  const validationMode = runtimeParams.get("validation") === "1";
  const onboardingDownloadUrl = "https://github.com/Rishikakaps/SHAeR";

  document.body.dataset.shaerTheme = theme;
  if (validationMode) document.body.dataset.shaerValidation = "true";

  const runtime = {
    lastEventId: 0,
    physicalNonce: null,
    stopped: false,
    spotifyConfigured: false,
    spotifyAvailable: false,
    wasConnected: false,
    onboardingQrDismissed: false,
    onboardingUnpaired: false,
    volume: 50,
    inputMode: "navigation",
    playback: {
      title: "",
      artist: "",
      album: "",
      duration_ms: 0,
      progress_ms: 0,
      cover_art: null,
      status: "stopped",
      queue_position: null,
      source: "local",
      uri: null
    },
    queue: [],
    recording: {
      state: "idle",
      elapsed_ms: 0,
      storage_free: 0,
      level: null,
      last_error: null
    },
    recordingFilter: "recent",
    metrics: {
      startedAt: Date.now(),
      frames: 0,
      fps: 0,
      navigationLatencyMs: 0,
      spotifyLatencyMs: 0,
      artworkLatencyMs: 0,
      errors: 0
    }
  };

  let systemLayer;
  let overlayTimer;
  let pairingRequest = null;
  let recordingAudio = null;
  let unsubscribeMusic = null;
  let fpsWindowStarted = performance.now();
  let fpsWindowFrames = 0;

  function escapeHtml(value) {
    return String(value == null ? "" : value).replace(/[&<>"']/g, (char) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      "\"": "&quot;",
      "'": "&#39;"
    }[char]));
  }

  function initSystemLayer() {
    const host = document.querySelector(".device") || document.getElementById("liveScreen") || document.body;
    systemLayer = document.createElement("section");
    systemLayer.id = "shaer-system-layer";
    systemLayer.hidden = true;
    systemLayer.setAttribute("aria-live", "polite");
    host.appendChild(systemLayer);
    systemLayer.addEventListener("click", handleSystemAction);
    if (deviceMode && !window.sessionStorage.getItem("shaer-boot-shown")) {
      window.sessionStorage.setItem("shaer-boot-shown", "1");
      showBoot();
    }
  }

  function renderOverlay(content) {
    if (!systemLayer || !systemLayer.isConnected) initSystemLayer();
    window.clearTimeout(overlayTimer);
    systemLayer.innerHTML = content;
    systemLayer.hidden = false;
    const firstAction = systemLayer.querySelector("[data-system-action]");
    if (firstAction) {
      firstAction.classList.add("is-selected");
      firstAction.focus({ preventScroll: true });
    }
  }

  function closeOverlay() {
    if (!systemLayer) return;
    window.clearTimeout(overlayTimer);
    systemLayer.hidden = true;
    systemLayer.innerHTML = "";
  }

  function showBoot() {
    renderOverlay(`
      <div class="shaer-boot-overlay shaer-universal-screen" role="status">
        <div class="shaer-constellation" aria-hidden="true">
          <svg viewBox="0 0 86 92" focusable="false">
            <polyline pathLength="1" points="14,55 33,45 45,15 66,31 57,73 38,64" />
          </svg>
          <i></i><i></i><i></i><i></i><i></i><i></i>
        </div>
        <strong>SHAeR</strong>
        <span>powered by आदि-vasi</span>
        <div class="shaer-boot-line" aria-label="Loading"><i></i></div>
      </div>
    `);
    overlayTimer = window.setTimeout(() => {
      closeOverlay();
      checkOnboardingQr();
    }, 5400);
  }

  async function checkOnboardingQr() {
    if (!deviceMode || validationMode || runtime.stopped) return;
    try {
      const response = await fetch("/api/v1/pairing/state", { cache: "no-store" });
      const payload = response.ok ? await response.json() : {};
      const data = payload.data || {};
      runtime.onboardingUnpaired = !Boolean(data.paired) && Number(data.trusted_count || 0) === 0;
      if (runtime.onboardingUnpaired && !runtime.onboardingQrDismissed) showOnboardingQr();
    } catch {
      // Static previews and offline bench runs should continue without onboarding state.
    }
  }

  function showOnboardingQr() {
    runtime.onboardingQrDismissed = false;
    renderOverlay(`
      <section class="shaer-onboarding-qr" role="dialog" aria-modal="true" aria-label="Download SHAeR companion">
        <img src="/api/onboarding/download-qr.svg" alt="QR code for SHAeR GitHub download page">
        <p>${escapeHtml(onboardingDownloadUrl)}</p>
        <button type="button" data-system-action="onboarding-close">Next</button>
      </section>
    `);
  }

  function showUniversalConnection(kind, title, message, actions) {
    const buttons = actions.map(({ action, label }) => (
      `<button type="button" data-system-action="${escapeHtml(action)}">${escapeHtml(label)}</button>`
    )).join("");
    renderOverlay(`
      <section class="shaer-universal-screen shaer-connection-screen" role="dialog" aria-modal="true" data-popup-kind="${escapeHtml(kind)}">
        <span class="shaer-system-kicker">SHAeR OS</span>
        <h2>${escapeHtml(title)}</h2>
        <p>${escapeHtml(message)}</p>
        <div class="shaer-universal-list">${buttons}</div>
        <button class="shaer-back-row" type="button" data-system-action="close">Back</button>
      </section>
    `);
  }

  function showPopup(kind, title, message, actions) {
    const buttons = (actions || [{ action: "close", label: "OK" }]).map(({ action, label }) => (
      `<button type="button" data-system-action="${escapeHtml(action)}">${escapeHtml(label)}</button>`
    )).join("");
    renderOverlay(`
      <div class="shaer-system-scrim"></div>
      <section class="shaer-system-popup" role="dialog" aria-modal="true" data-popup-kind="${escapeHtml(kind)}">
        <span class="shaer-system-kicker">SHAeR SYSTEM</span>
        <h2>${escapeHtml(title)}</h2>
        <p>${escapeHtml(message)}</p>
        <div class="shaer-system-actions">${buttons}</div>
      </section>
    `);
  }

  function showLoading(title, message) {
    renderOverlay(`
      <div class="shaer-system-scrim"></div>
      <section class="shaer-system-popup" role="status" data-popup-kind="loading">
        <div class="shaer-system-spinner"></div>
        <h2>${escapeHtml(title)}</h2>
        <p>${escapeHtml(message)}</p>
      </section>
    `);
  }

  function showQueue() {
    const rows = runtime.queue.length ? runtime.queue.slice(0, 8).map((item, index) => `
      <div><b>${String(index + 1).padStart(2, "0")}</b><span>${escapeHtml(item.title || "Unknown track")}<br>${escapeHtml(item.artist || "Unknown artist")}</span></div>
    `).join("") : `<p>Queue is empty.</p>`;
    renderOverlay(`
      <div class="shaer-system-scrim"></div>
      <section class="shaer-system-popup" role="dialog" aria-modal="true" data-popup-kind="queue">
        <span class="shaer-system-kicker">UP NEXT</span>
        <h2>QUEUE</h2>
        <div class="shaer-system-queue">${rows}</div>
        <div class="shaer-system-actions"><button type="button" data-system-action="close">BACK</button></div>
      </section>
    `);
  }

  function recordingFilterQuery(filter) {
    const now = new Date();
    if (filter === "favorites") return "?favorite=1";
    if (filter === "month") return `?year=${now.getFullYear()}&month=${now.getMonth() + 1}`;
    if (filter === "year") return `?year=${now.getFullYear()}`;
    return "?limit=12";
  }

  async function showRecordingLibrary(filter = runtime.recordingFilter) {
    runtime.recordingFilter = filter;
    showLoading("PERSONAL ARCHIVE", "Reading recording metadata...");
    try {
      const response = await fetch(`/api/recording/library${recordingFilterQuery(filter)}`, { cache: "no-store" });
      const payload = await response.json();
      if (!response.ok) throw new Error(payload.error && payload.error.message ? payload.error.message : "Archive unavailable.");
      const items = Array.isArray(payload.recordings) ? payload.recordings : [];
      const tabs = ["recent", "favorites", "month", "year"].map((name) => (
        `<button type="button" data-system-action="recording-filter:${name}"${name === filter ? " class=\"is-current\"" : ""}>${name.toUpperCase()}</button>`
      )).join("");
      const rows = items.length ? items.slice(0, 8).map((item) => `
        <button class="shaer-recording-row" type="button" data-system-action="recording-play:${Number(item.id)}">
          <b>${escapeHtml(item.display_title || "Recording")}</b>
          <span>${escapeHtml(item.date || "")} · ${recordingTime(item.duration_ms)}${item.favorite ? " · FAVORITE" : ""}</span>
        </button>
      `).join("") : `<p class="shaer-recording-empty">No recordings in this view.</p>`;
      renderOverlay(`
        <div class="shaer-system-scrim"></div>
        <section class="shaer-system-popup shaer-recording-library" role="dialog" aria-modal="true" data-popup-kind="recordings">
          <span class="shaer-system-kicker">PERSONAL ARCHIVE</span>
          <h2>RECORDINGS</h2>
          <div class="shaer-recording-filters">${tabs}</div>
          <div class="shaer-recording-list">${rows}</div>
          <div class="shaer-system-actions"><button type="button" data-system-action="close">BACK</button></div>
        </section>
      `);
    } catch (error) {
      showPopup("recording-error", "ARCHIVE UNAVAILABLE", error.message || "The personal archive could not be opened.");
    }
  }

  function stopRecordingPlayback() {
    if (!recordingAudio) return;
    recordingAudio.pause();
    recordingAudio.removeAttribute("src");
    recordingAudio.load();
    recordingAudio = null;
  }

  async function playRecording(recordingId) {
    try {
      const response = await fetch(`/api/recording/library?limit=100`, { cache: "no-store" });
      const payload = await response.json();
      const item = (payload.recordings || []).find((candidate) => Number(candidate.id) === Number(recordingId));
      if (!response.ok || !item) throw new Error("Recording metadata is unavailable.");
      stopRecordingPlayback();
      recordingAudio = new Audio(`/api/recording/audio/${Number(recordingId)}`);
      recordingAudio.preload = "auto";
      runtime.playback = {
        ...runtime.playback,
        title: item.display_title || "Recording",
        artist: item.date || "Personal archive",
        album: "Voice Memos",
        duration_ms: Number(item.duration_ms) || 0,
        progress_ms: 0,
        cover_art: null,
        status: "loading",
        source: "recording",
        uri: `recording:${Number(recordingId)}`
      };
      applyPlayback(runtime.playback);
      recordingAudio.addEventListener("loadedmetadata", () => {
        runtime.playback.duration_ms = Math.round((recordingAudio.duration || 0) * 1000) || runtime.playback.duration_ms;
        applyPlayback(runtime.playback);
      });
      recordingAudio.addEventListener("timeupdate", () => {
        runtime.playback.progress_ms = Math.round(recordingAudio.currentTime * 1000);
        runtime.playback.status = recordingAudio.paused ? "paused" : "playing";
        applyPlayback(runtime.playback);
      });
      recordingAudio.addEventListener("ended", () => {
        runtime.playback.status = "stopped";
        runtime.playback.progress_ms = runtime.playback.duration_ms;
        applyPlayback(runtime.playback);
      });
      await recordingAudio.play();
      runtime.playback.status = "playing";
      closeOverlay();
      window.dispatchEvent(new CustomEvent("shaer:validation-navigate", { detail: { state: "now-playing" } }));
      window.setTimeout(() => applyPlayback(runtime.playback), 0);
    } catch (error) {
      showPopup("recording-error", "PLAYBACK ERROR", error.message || "This recording could not be played.");
    }
  }

  function controlRecordingPlayback(action) {
    if (!recordingAudio) return false;
    if (["toggle-play", "play", "pause"].includes(action)) {
      if (action === "pause" || (action === "toggle-play" && !recordingAudio.paused)) recordingAudio.pause();
      else recordingAudio.play().catch(() => showPopup("recording-error", "PLAYBACK ERROR", "This recording could not be resumed."));
      return true;
    }
    if (action === "previous") recordingAudio.currentTime = 0;
    if (action === "next") recordingAudio.currentTime = Math.min(recordingAudio.duration || 0, recordingAudio.currentTime + 15);
    return ["previous", "next"].includes(action);
  }

  function showVolume() {
    renderOverlay(`
      <div class="shaer-system-scrim"></div>
      <section class="shaer-system-popup" role="status" data-popup-kind="volume">
        <span class="shaer-system-kicker">AUDIO</span>
        <h2>VOLUME ${runtime.volume}%</h2>
        <div class="shaer-volume-track" style="--volume:${runtime.volume}%"><i></i></div>
      </section>
    `);
    overlayTimer = window.setTimeout(closeOverlay, 900);
  }

  function mapSpotifyError(status, message) {
    const lower = String(message || "").toLowerCase();
    if (status === 401 || lower.includes("log in") || lower.includes("authoriz")) {
      return ["LOGIN EXPIRED", "Reconnect Spotify from the SHAeR home menu."];
    }
    if (status === 403 || lower.includes("premium")) {
      return ["PREMIUM REQUIRED", "This Spotify playback action requires Spotify Premium."];
    }
    if (lower.includes("transfer")) return ["TRANSFER FAILED", "SHAeR could not take over playback. Try again."];
    if (lower.includes("network") || lower.includes("unreachable")) return ["NETWORK UNAVAILABLE", "SHAeR will reconnect automatically."];
    return ["SPOTIFY UNAVAILABLE", message || "SHAeR will retry in the background."];
  }

  function handleSystemAction(event) {
    const button = event.target.closest("[data-system-action]");
    if (!button) return;
    const action = button.dataset.systemAction;
    if (action === "close" || action === "cancel") closeOverlay();
    if (action === "onboarding-close") {
      runtime.onboardingQrDismissed = true;
      closeOverlay();
    }
    if (action === "retry-login") beginSpotifyLogin();
    if (action === "shutdown") requestShutdown();
    if (action === "pair-approve") answerPairing(true);
    if (action === "pair-deny") answerPairing(false);
    if (action === "recording-keep") closeOverlay();
    if (action === "recording-cancel") recordingControl("cancel");
    if (action.startsWith("recording-filter:")) showRecordingLibrary(action.slice(17));
    if (action.startsWith("recording-play:")) playRecording(Number(action.slice(15)));
    if (action.startsWith("settings:")) showSettingsDomain(action.slice(9));
  }

  async function answerPairing(approved) {
    if (!pairingRequest) return;
    const pairingId = pairingRequest.pairing_id;
    pairingRequest = null;
    try {
      await fetch("/api/v1/pairing/approve", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ pairing_id: pairingId, approved, physical_nonce: runtime.physicalNonce })
      });
    } finally {
      runtime.physicalNonce = null;
      closeOverlay();
    }
  }

  async function pollPairing() {
    if (!deviceMode || runtime.stopped) return;
    try {
      const response = await fetch("/api/v1/pairing/pending", { cache: "no-store" });
      const payload = response.ok ? await response.json() : {};
      const pending = payload.data && payload.data.requests ? payload.data.requests[0] : null;
      if (pending && (!pairingRequest || pairingRequest.pairing_id !== pending.pairing_id)) {
        pairingRequest = pending;
        showPopup(
          "pairing",
          `PAIR ${pending.code}`,
          `${pending.device_name} wants to manage this SHAeR. Confirm that the same code appears on the companion.`,
          [{ action: "pair-approve", label: "TRUST" }, { action: "pair-deny", label: "DENY" }]
        );
      }
    } catch {
      // Pairing discovery is unavailable in static preview mode.
    } finally {
      window.setTimeout(pollPairing, 1000);
    }
  }

  function isMemoScreen() {
    return Boolean(document.querySelector(
      "#liveScreen .screen-memos, #liveScreen [data-screen='memos'], #liveScreen [data-name='memos'], " +
      "#liveScreen .memo-card, #liveScreen .memo-raga, #liveScreen .memo-wave, #liveScreen .waveform"
    ));
  }

  function recordingTime(milliseconds) {
    const seconds = Math.max(0, Math.floor((Number(milliseconds) || 0) / 1000));
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor(seconds % 3600 / 60);
    const rest = seconds % 60;
    return hours ? `${hours}:${String(minutes).padStart(2, "0")}:${String(rest).padStart(2, "0")}` : `${String(minutes).padStart(2, "0")}:${String(rest).padStart(2, "0")}`;
  }

  function recordingStorageText(bytes) {
    const value = Number(bytes) || 0;
    return value >= 1024 ** 3 ? `${(value / 1024 ** 3).toFixed(1)} GB FREE` : `${Math.max(0, Math.round(value / 1024 ** 2))} MB FREE`;
  }

  function syncRecordingDom() {
    document.body.dataset.recordingState = runtime.recording.state;
    if (!isMemoScreen()) return;
    const screen = document.querySelector("#liveScreen .screen, #liveScreen article");
    if (!screen) return;
    let status = screen.querySelector(".shaer-recording-runtime");
    if (!status) {
      status = document.createElement("div");
      status.className = "shaer-recording-runtime";
      status.innerHTML = "<b data-recording-state></b><strong data-recording-time></strong><small data-recording-storage></small>";
      screen.appendChild(status);
    }
    status.querySelector("[data-recording-state]").textContent = runtime.recording.state.toUpperCase();
    status.querySelector("[data-recording-time]").textContent = recordingTime(runtime.recording.elapsed_ms);
    status.querySelector("[data-recording-storage]").textContent = recordingStorageText(runtime.recording.storage_free);
    document.querySelectorAll("#liveScreen .memo-wave > strong, #liveScreen .memo-card > strong, #liveScreen .memo-raga > strong").forEach((node) => {
      node.textContent = recordingTime(runtime.recording.elapsed_ms);
    });
    const copy = document.querySelector("#liveScreen .record-copy");
    if (copy) copy.textContent = runtime.recording.state === "idle" ? "PRESS ○ TO RECORD" : runtime.recording.state === "paused" ? "PAUSED · PRESS ○ TO RESUME" : "RECORDING · LONG PRESS TO FINISH";
    const level = Number.isFinite(runtime.recording.level) ? runtime.recording.level : (runtime.recording.state === "recording" ? 0.45 : 0.08);
    document.querySelectorAll("#liveScreen .memo-bars i, #liveScreen .waveform i, #liveScreen .memo-wave i").forEach((bar, index) => {
      const height = Math.max(0.12, Math.min(1, level * (0.7 + ((index * 7) % 5) / 5)));
      bar.style.transform = `scaleY(${height})`;
    });
  }

  async function recordingControl(requestedAction) {
    let action = requestedAction;
    if (action === "toggle" && runtime.recording.state === "finalizing") {
      showLoading("FINALIZING RECORDING", "Writing the WAV header and metadata safely...");
      return;
    }
    if (action === "toggle") action = runtime.recording.state === "idle" || runtime.recording.state === "error" ? "start" : runtime.recording.state === "recording" ? "pause" : "resume";
    if (action === "stop" && runtime.recording.state === "idle") {
      showRecordingLibrary();
      return;
    }
    if (action === "cancel" && runtime.recording.state === "idle") return;
    try {
      const response = await fetch("/api/recording/control", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action, theme: pathTheme ? pathTheme[0] : "shaer_dark_archive" })
      });
      const payload = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(payload.error && payload.error.message ? payload.error.message : "Recording command failed.");
      runtime.recording = { ...runtime.recording, ...(payload.data || {}) };
      closeOverlay();
      syncRecordingDom();
      if (action === "stop") showPopup("recording-saved", "RECORDING SAVED", "Your voice memo is now in the personal archive.");
    } catch (error) {
      showPopup("recording-error", "RECORDING ERROR", error.message || "The microphone could not complete this action.");
      runtime.metrics.errors += 1;
    }
  }

  async function pollRecording() {
    if (runtime.stopped) return;
    try {
      const response = await fetch("/api/recording/status", { cache: "no-store" });
      if (response.ok) runtime.recording = { ...runtime.recording, ...await response.json() };
      syncRecordingDom();
    } catch {
      // Recording remains unavailable in static preview mode.
    } finally {
      window.setTimeout(pollRecording, runtime.recording.state === "idle" ? 1200 : 250);
    }
  }

  function dispatchKey(key) {
    window.dispatchEvent(new KeyboardEvent("keydown", { key, bubbles: true, cancelable: true }));
  }

  function dispatchAction(action) {
    const started = performance.now();
    if (action.startsWith("theme:shaer_")) {
      const themeId = action.slice(6);
      if (/^shaer_[a-z0-9_]+$/.test(themeId)) window.location.assign(`/${themeId}/?mode=device`);
      return;
    }
    if (action === "toggle_input_mode") {
      runtime.inputMode = runtime.inputMode === "navigation" ? "volume" : "navigation";
      document.body.dataset.shaerInputMode = runtime.inputMode;
      window.dispatchEvent(new CustomEvent("shaer:input-mode", { detail: { mode: runtime.inputMode } }));
      showPopup("input-mode", runtime.inputMode === "volume" ? "VOLUME MODE" : "NAVIGATION MODE", runtime.inputMode === "volume" ? "Turn the encoder to adjust volume. Double-click OK to return." : "Turn the encoder to browse. Double-click OK to adjust volume.");
      return;
    }
    if (systemLayer && !systemLayer.hidden) {
      const buttons = Array.from(systemLayer.querySelectorAll("[data-system-action]"));
      const selected = systemLayer.querySelector("[data-system-action].is-selected") || buttons[0];
      if (action === "select" && selected) {
        selected.click();
        return;
      }
      if (action === "back") {
        const deny = systemLayer.querySelector('[data-system-action="pair-deny"]');
        if (deny) deny.click(); else closeOverlay();
        return;
      }
      if ((action === "left" || action === "right") && buttons.length > 1) {
        const index = Math.max(0, buttons.indexOf(selected));
        const next = buttons[(index + (action === "right" ? 1 : buttons.length - 1)) % buttons.length];
        buttons.forEach((button) => button.classList.remove("is-selected"));
        next.classList.add("is-selected");
        next.focus({ preventScroll: true });
        return;
      }
    }
    if (action === "back" && runtime.onboardingUnpaired && runtime.onboardingQrDismissed) {
      showOnboardingQr();
      return;
    }
    if (action === "long_select" && runtime.recording.state !== "idle") {
      recordingControl("stop");
      return;
    }
    if (action === "back" && runtime.recording.state !== "idle") {
      showPopup("recording-cancel", "DISCARD RECORDING?", "The current voice memo has not been saved.", [
        { action: "recording-keep", label: "KEEP" },
        { action: "recording-cancel", label: "DISCARD" }
      ]);
      return;
    }
    if (action === "long_select" || action === "menu") {
      showQueue();
      return;
    }
    if (action === "volume_up" || action === "volume_down") {
      changeVolume(action === "volume_up" ? 5 : -5);
      return;
    }
    if ((action === "left" || action === "right") && runtime.inputMode === "volume") {
      changeVolume(action === "right" ? 5 : -5);
      return;
    }
    const key = keyForAction[action];
    if (!key) return;
    window.dispatchEvent(new CustomEvent("shaer:hardware", { detail: { action, inputMode: runtime.inputMode } }));
    dispatchKey(key);
    window.requestAnimationFrame(() => {
      runtime.metrics.navigationLatencyMs = Math.round((performance.now() - started) * 10) / 10;
    });
  }

  async function pollHardware() {
    if (runtime.stopped) return;
    try {
      const response = await fetch(`/api/events?after=${runtime.lastEventId}`, { cache: "no-store" });
      if (response.ok) {
        const payload = await response.json();
        for (const event of payload.events || []) {
          runtime.lastEventId = Math.max(runtime.lastEventId, Number(event.id) || 0);
          if (event.physical_nonce) runtime.physicalNonce = String(event.physical_nonce);
          dispatchAction(event.action);
        }
      }
    } catch {
      // Static preview mode intentionally works without the Pi event server.
    } finally {
      window.setTimeout(pollHardware, 55);
    }
  }

  async function apiPost(path, payload, requiresSpotify = true) {
    if (requiresSpotify && !runtime.spotifyAvailable) return false;
    try {
      const response = await fetch(path, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload || {})
      });
      if (!response.ok) {
        const errorPayload = await response.json().catch(() => ({}));
        const [title, message] = mapSpotifyError(response.status, errorPayload.error);
        showPopup("error", title, message);
        runtime.metrics.errors += 1;
        return false;
      }
      return true;
    } catch {
      showPopup("error", "NO INTERNET", "SHAeR will reconnect when the network returns.");
      runtime.metrics.errors += 1;
      return false;
    }
  }

  async function beginSpotifyLogin() {
    showLoading("CONNECTING SPOTIFY", "Waiting for secure authorization...");
    try {
      const response = await fetch("/api/spotify/login?launch=0", { cache: "no-store" });
      const payload = await response.json();
      if (response.ok && payload.authorization_url) {
        window.location.assign(payload.authorization_url);
        return;
      }
      const [title, message] = mapSpotifyError(response.status, payload.error);
      showPopup("error", title, message, [
        { action: "cancel", label: "BACK" },
        { action: "retry-login", label: "RETRY" }
      ]);
    } catch {
      showPopup("error", "NO INTERNET", "Connect SHAeR to Wi-Fi and try again.");
    }
  }

  function formatTime(milliseconds) {
    const seconds = Math.max(0, Math.floor((Number(milliseconds) || 0) / 1000));
    return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, "0")}`;
  }

  function setText(selector, value) {
    document.querySelectorAll(selector).forEach((node) => {
      node.textContent = value || "";
      node.classList.toggle("shaer-long-text", String(value || "").length > 22);
    });
  }

  function applyArtwork(url) {
    const targets = document.querySelectorAll("[data-shaer-cover]");
    if (!url) {
      targets.forEach((node) => {
        node.classList.add("shaer-artwork-fallback");
        if (node.tagName === "IMG") node.removeAttribute("src");
        else node.style.removeProperty("background-image");
      });
      return;
    }
    const started = performance.now();
    const image = new Image();
    image.onload = () => {
      runtime.metrics.artworkLatencyMs = Math.round((performance.now() - started) * 10) / 10;
      targets.forEach((node) => {
        node.classList.remove("shaer-artwork-fallback");
        if (node.tagName === "IMG") node.src = url;
        else node.style.backgroundImage = `url(${JSON.stringify(url)})`;
      });
    };
    image.onerror = () => targets.forEach((node) => node.classList.add("shaer-artwork-fallback"));
    image.src = url;
  }

  function applyPlayback(state) {
    if (!state || !state.source) return;
    runtime.playback = { ...runtime.playback, ...state };
    if (Number.isFinite(runtime.playback.volume_percent)) runtime.volume = runtime.playback.volume_percent;
    setText("[data-shaer-title]", runtime.playback.title || "Unknown Track");
    setText("[data-shaer-artist]", runtime.playback.artist || "Unknown Artist");
    setText("[data-shaer-album]", runtime.playback.album || "Unknown Album");
    setText("[data-shaer-elapsed]", formatTime(runtime.playback.progress_ms));
    setText("[data-shaer-duration]", formatTime(runtime.playback.duration_ms));
    const progress = runtime.playback.duration_ms > 0
      ? Math.min(100, Math.max(0, runtime.playback.progress_ms / runtime.playback.duration_ms * 100))
      : 0;
    document.querySelectorAll("[data-shaer-progress]").forEach((node) => {
      node.style.setProperty("--p", `${progress}%`);
      node.style.setProperty("--play-progress", `${progress}%`);
      node.style.transform = `scaleX(${progress / 100})`;
    });
    applyArtwork(runtime.playback.cover_art);
    syncMarginaliaControl();
    window.dispatchEvent(new CustomEvent("shaer:playback", { detail: { ...runtime.playback } }));
  }

  function currentArchiveTrack() {
    const music = window.SHAeRMusic ? window.SHAeRMusic.getState() : {};
    const track = music.currentPlayback && music.currentPlayback.currentTrack;
    return track ? { ...track, ...runtime.playback } : { ...runtime.playback };
  }

  function syncMarginaliaControl() {
    const screen = document.querySelector("#liveScreen .screen");
    if (!screen || !runtime.playback.title || !["now-playing", "now", "playing"].some((name) => screen.className.includes(name) || screen.dataset.name === name)) return;
    let button = screen.querySelector("[data-shaer-marginalia]");
    if (!button) {
      button = document.createElement("button");
      button.type = "button";
      button.dataset.shaerMarginalia = "1";
      button.className = "shaer-marginalia-launch";
      button.setAttribute("aria-label", "Open Marginalia notebook");
      button.title = "Open Marginalia notebook";
      button.innerHTML = `<span aria-hidden="true"></span>`;
      screen.appendChild(button);
    }
  }

  function openMarginalia() {
    if (document.querySelector("[data-shaer-marginalia-overlay]")) return;
    const track = currentArchiveTrack();
    if (!track.title) return;
    const overlay = document.createElement("section");
    overlay.dataset.shaerMarginaliaOverlay = "1";
    overlay.className = "shaer-marginalia-page";
    overlay.innerHTML = `
      <header class="shaer-marginalia-page__bar">
        <button type="button" data-marginalia-tool="pen" aria-label="Pen"><span></span></button>
        <button type="button" data-marginalia-tool="circle" aria-label="Circle"><span></span></button>
        <button type="button" data-marginalia-tool="erase">ERASE</button>
        <strong>MARGINALIA</strong>
        <button type="button" data-marginalia-close aria-label="Close Marginalia">DONE</button>
      </header>
      <p class="shaer-marginalia-page__meta">${escapeHtml(track.title)} · ${escapeHtml(track.artist || "")}</p>
      <div class="shaer-marginalia-page__canvas">
        <canvas></canvas>
        <div class="shaer-marginalia-page__hint">welcome to marginalia</div>
      </div>
      <small>${formatTime(runtime.playback.progress_ms)} · saves when you leave</small>`;
    document.body.appendChild(overlay);
    const canvas = overlay.querySelector("canvas");
    const hint = overlay.querySelector(".shaer-marginalia-page__hint");
    const context = canvas.getContext("2d");
    const ratio = Math.max(1, window.devicePixelRatio || 1);
    canvas.width = Math.round(canvas.clientWidth * ratio);
    canvas.height = Math.round(canvas.clientHeight * ratio);
    context.scale(ratio, ratio);
    context.strokeStyle = "#17201e";
    context.lineWidth = 2.2;
    context.lineCap = "round";
    let drawing = false;
    let dirty = false;
    const point = (event) => { const rect = canvas.getBoundingClientRect(); return { x: event.clientX - rect.left, y: event.clientY - rect.top }; };
    const dismissHint = () => {
      if (hint) hint.hidden = true;
    };
    canvas.addEventListener("pointerdown", (event) => { dismissHint(); drawing = true; dirty = true; canvas.setPointerCapture(event.pointerId); const p = point(event); context.beginPath(); context.moveTo(p.x, p.y); });
    canvas.addEventListener("pointermove", (event) => { if (!drawing) return; const p = point(event); context.lineTo(p.x, p.y); context.stroke(); });
    canvas.addEventListener("pointerup", () => { drawing = false; });
    overlay.querySelectorAll("[data-marginalia-tool]").forEach((button) => button.addEventListener("click", dismissHint));
    const close = async () => {
      if (dirty) {
        try {
          const entryResponse = await fetch("/api/archive/entries", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(track) });
          const entry = await entryResponse.json();
          if (!entryResponse.ok) throw new Error(entry.error && entry.error.message ? entry.error.message : "Archive entry unavailable.");
          const imageBase64 = canvas.toDataURL("image/png").split(",", 2)[1];
          const pageResponse = await fetch(`/api/archive/${entry.data.id}/marginalia`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ image_base64: imageBase64, playback_position_ms: runtime.playback.progress_ms, theme_id: pathTheme ? pathTheme[0] : "shaer_dark_archive" }) });
          if (!pageResponse.ok) throw new Error("Marginalia could not be saved.");
        } catch (error) { showPopup("marginalia-error", "ARCHIVE ERROR", error.message || "Marginalia could not be saved."); }
      }
      overlay.remove();
      window.removeEventListener("keydown", handleMarginaliaKey);
    };
    const handleMarginaliaKey = (event) => {
      if (event.key === "Escape" || event.key === "Backspace") {
        event.preventDefault();
        close();
      }
    };
    window.addEventListener("keydown", handleMarginaliaKey);
    overlay.querySelector("[data-marginalia-close]").addEventListener("click", close);
  }

  function settingsRows(domain) {
    const capabilities = runtime.capabilities || {};
    const powerAvailable = capabilities.power === true || (capabilities.state && capabilities.state.power && capabilities.state.power.available === true);
    const bluetoothAvailable = capabilities.bluetooth === true || (capabilities.state && capabilities.state.bluetooth && capabilities.state.bluetooth.available === true);
    const rtcAvailable = capabilities.rtc === true || (capabilities.state && capabilities.state.rtc && capabilities.state.rtc.available === true);
    const syncStatus = runtime.wasConnected ? "Ready" : "Pair from companion";
    const rows = {
      ABOUT: [["Device", "SHAeR"], ["Firmware", "2026.07"], ["Renderer", theme], ["Storage", "Open details"]],
      APPEARANCE: [["Theme", theme], ["Brightness", "Device default"], ["Screen timeout", "Always on"], ["Reduced motion", "Off"]],
      PLAYBACK: [["Resume track", "On"], ["Gapless", "On"], ["Crossfade", "Off"], ["Queue memory", "On"]],
      AUDIO: [["Output", "System default"], ["Volume step", "5%"], ["Balance", "Center"], ["Mono audio", "Off"]],
      CONNECTIVITY: [["Wi-Fi", "Configured on device"], ["Bluetooth", bluetoothAvailable ? "Available" : "Unavailable"], ["Spotify", runtime.spotifyAvailable ? "Connected" : "Reconnect"], ["Sync", syncStatus]],
      POWER: [["Battery level", powerAvailable ? "Read from hardware" : "Unavailable"], ["Charging", powerAvailable ? "Read from hardware" : "Unavailable"], ["Low power", powerAvailable ? "Available" : "Unavailable"], ["Shut down", "Confirm before action", "shutdown"]],
      DATE_TIME: [["Date", rtcAvailable ? "RTC available" : "System clock"], ["Time", rtcAvailable ? "RTC available" : "System clock"], ["Time zone", Intl.DateTimeFormat().resolvedOptions().timeZone || "Local"], ["Auto time", rtcAvailable ? "Available" : "Uses OS"]],
      SYNC: [["Get Sync app", "QR pairing flow"], ["Pair device", "Requires physical confirm"], ["Sync now", runtime.wasConnected ? "Ready" : "Waiting"], ["Trusted devices", "Manage from companion"]],
      ADVANCED: [["Diagnostics", "Open status"], ["Storage repair", "Manual only"], ["Privacy", "Local archive"], ["Developer input", "Disabled by default"]]
    };
    return rows[domain] || rows.ABOUT;
  }

  function showSettingsDomain(setting) {
    const domain = String(setting || "").trim().toUpperCase().replace(/\s*&\s*/g, "_").replace(/\s+/g, "_");
    const label = domain === "DATE_TIME" ? "DATE & TIME" : domain.replace(/_/g, " ");
    const rows = settingsRows(domain);
    const visible = rows.slice(0, 4).map(([name, value, action], index) => `
      <button type="button" class="${index === 0 ? "is-selected" : ""}" data-system-action="${action === "shutdown" ? "shutdown" : "close"}">
        <b>${escapeHtml(name)}</b>
        <span>${escapeHtml(value)}</span>
      </button>
    `).join("");
    const dots = rows.map((_, index) => `<i${index < 4 ? " class=\"is-visible\"" : ""}></i>`).join("");
    renderOverlay(`
      <section class="shaer-settings-page" role="dialog" aria-modal="true" data-popup-kind="settings-${escapeHtml(domain.toLowerCase())}" data-settings-domain="${escapeHtml(domain)}">
        <span class="shaer-system-kicker">SETTINGS</span>
        <h2>${escapeHtml(label)}</h2>
        <div class="shaer-settings-page__rows">${visible}</div>
        <div class="shaer-settings-page__scroll" aria-label="Settings position">${dots}</div>
        <button class="shaer-back-row" type="button" data-system-action="close">Back</button>
      </section>
    `);
  }

  function normalizeQueue(payload) {
    const current = payload && payload.currently_playing ? [payload.currently_playing] : [];
    const queued = payload && Array.isArray(payload.queue) ? payload.queue : [];
    runtime.queue = [...current, ...queued].map((item) => ({
      title: item && item.name ? item.name : "Unknown Track",
      artist: item && Array.isArray(item.artists) ? item.artists.map((artist) => artist.name).filter(Boolean).join(", ") : "Unknown Artist",
      uri: item && item.uri ? item.uri : null
    }));
    window.dispatchEvent(new CustomEvent("shaer:queue", { detail: runtime.queue.slice() }));
  }

  function connectMusicStore() {
    if (diagnosticSource || !window.SHAeRMusic) return;
    unsubscribeMusic = window.SHAeRMusic.subscribe((music) => {
      runtime.spotifyConfigured = Boolean(music.spotify && music.spotify.configured);
      runtime.spotifyAvailable = Boolean(music.spotify && music.spotify.authenticated);
      runtime.queue = Array.isArray(music.queue) ? music.queue.map((track) => ({
        title: track.title,
        artist: track.artistText || track.artist,
        uri: track.uri
      })) : [];
      const playback = music.currentPlayback || {};
      const track = playback.currentTrack;
      if (track) {
        applyPlayback({
          title: track.title,
          artist: track.artistText || track.artist,
          album: track.album,
          duration_ms: playback.durationMs || track.durationMs,
          progress_ms: playback.progressMs,
          cover_art: track.artworkUrl,
          status: playback.isPlaying ? "playing" : "paused",
          volume_percent: playback.volumePercent,
          source: playback.source || track.source,
          uri: track.uri,
          shuffle: playback.shuffle,
          repeat_mode: playback.repeatMode,
          active_device_id: playback.activeDeviceId,
          active_device_name: playback.activeDeviceName
        });
      }
      if (music.status === "offline" && runtime.wasConnected) {
        showPopup("reconnect", "NO INTERNET", "Playback will recover when Wi-Fi returns.");
      } else if (music.status === "ready" && runtime.wasConnected && systemLayer && systemLayer.querySelector('[data-popup-kind="reconnect"]')) {
        closeOverlay();
      }
      runtime.wasConnected = music.status === "ready";
    });
    window.SHAeRMusic.start();
  }

  function changeVolume(delta) {
    runtime.volume = Math.min(100, Math.max(0, runtime.volume + delta));
    showVolume();
    apiPost("/api/spotify/control", { action: "volume", value: runtime.volume });
    window.dispatchEvent(new CustomEvent("shaer:volume", { detail: { percent: runtime.volume } }));
  }

  async function requestShutdown() {
    showLoading("SHUTTING DOWN", "Please wait before disconnecting power.");
    const ok = await apiPost("/api/system/shutdown", { physical_nonce: runtime.physicalNonce }, false);
    runtime.physicalNonce = null;
    if (!ok) return;
  }

  function frameTick(now) {
    runtime.metrics.frames += 1;
    fpsWindowFrames += 1;
    if (now - fpsWindowStarted >= 1000) {
      runtime.metrics.fps = Math.round(fpsWindowFrames * 1000 / (now - fpsWindowStarted));
      fpsWindowStarted = now;
      fpsWindowFrames = 0;
    }
    if (!runtime.stopped) window.requestAnimationFrame(frameTick);
  }

  function installDiagnosticFixture() {
    if (!['local', 'spotify'].includes(diagnosticSource)) return;
    const fixture = {
      title: "A Very Long SHAeR Test Title in 東京",
      artist: "Rishika & The Archive",
      album: "Native Sources",
      duration_ms: 243000,
      progress_ms: 87000,
      cover_art: null,
      status: "playing",
      queue_position: 1,
      volume_percent: 64,
      source: diagnosticSource,
      uri: diagnosticSource === "spotify" ? "spotify:track:diagnostic" : "local:track:diagnostic"
    };
    runtime.queue = [
      { title: fixture.title, artist: fixture.artist, uri: fixture.uri },
      { title: "Monsoon Window", artist: "SHAeR", uri: "diagnostic:2" },
      { title: "Night Chasing", artist: "MP-002", uri: "diagnostic:3" }
    ];
    applyPlayback(fixture);
    window.setInterval(() => applyPlayback(fixture), 500);
  }

  function reportMetrics() {
    const memory = performance.memory ? {
      usedJsHeapBytes: performance.memory.usedJSHeapSize,
      totalJsHeapBytes: performance.memory.totalJSHeapSize
    } : {};
    fetch("/api/system/metrics", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        theme,
        uptimeMs: Date.now() - runtime.metrics.startedAt,
        fps: runtime.metrics.fps,
        navigationLatencyMs: runtime.metrics.navigationLatencyMs,
        spotifyLatencyMs: runtime.metrics.spotifyLatencyMs,
        artworkLatencyMs: runtime.metrics.artworkLatencyMs,
        errors: runtime.metrics.errors,
        ...memory
      })
    }).catch(() => {});
  }

  document.addEventListener("click", (event) => {
    const target = event.target instanceof Element ? event.target : null;
    if (!target) return;
    if (target.closest("[data-shaer-marginalia]")) {
      event.preventDefault();
      event.stopImmediatePropagation();
      openMarginalia();
      return;
    }
    const memoControl = target.closest("#liveScreen [data-action]");
    const memoAction = memoControl && memoControl.dataset.action;
    if (isMemoScreen() && ["toggle-memo", "save-memo", "delete-memo", "recording-library"].includes(memoAction)) {
      event.preventDefault();
      event.stopImmediatePropagation();
      if (memoAction === "recording-library") showRecordingLibrary();
      else recordingControl(memoAction === "toggle-memo" ? "toggle" : memoAction === "save-memo" ? "stop" : "cancel");
      return;
    }
    if (isMemoScreen() && memoAction === "toggle-play" && runtime.recording.state !== "idle") {
      event.preventDefault();
      event.stopImmediatePropagation();
      recordingControl("toggle");
      return;
    }
    const spotifyLogin = target.closest('[aria-label*="Spotify connect" i]');
    if (spotifyLogin && !runtime.spotifyAvailable) beginSpotifyLogin();
    const control = target.closest("[data-action]");
    const action = control && control.dataset.action;
    if (action === "recording-library") {
      event.preventDefault();
      event.stopImmediatePropagation();
      showRecordingLibrary();
      return;
    }
    if (["toggle-play", "play", "pause", "next", "previous"].includes(action)) {
      const playback = window.SHAeRMusic ? window.SHAeRMusic.getState().currentPlayback : {};
      if (!controlRecordingPlayback(action)) {
        if (playback.source === "local" && window.SHAeRMusic) {
          if (action !== "next" && action !== "previous") window.SHAeRMusic.controlLocalPlayback(action);
        } else {
          apiPost("/api/spotify/control", { action });
        }
      }
    }
    if (action === "shuffle") {
      const playback = window.SHAeRMusic ? window.SHAeRMusic.getState().currentPlayback : {};
      apiPost("/api/spotify/control", { action: "shuffle", enabled: !Boolean(playback.shuffle) });
    }
    if (action === "repeat") {
      const playback = window.SHAeRMusic ? window.SHAeRMusic.getState().currentPlayback : {};
      const nextMode = playback.repeatMode === "off" ? "context" : playback.repeatMode === "context" ? "track" : "off";
      apiPost("/api/spotify/control", { action: "repeat", mode: nextMode });
    }
  }, true);

  window.addEventListener("shaer:track-select", (event) => {
    const track = event.detail || {};
    if (track.uri && String(track.uri).startsWith("spotify:")) {
      apiPost("/api/spotify/control", { action: "play-uri", uri: track.uri });
    } else if ((track.isLocal || track.source === "local") && window.SHAeRMusic) {
      window.SHAeRMusic.controlLocalPlayback("play-track", track);
    }
  });

  window.addEventListener("shaer:playlist-select", (event) => {
    if (window.SHAeRMusic) window.SHAeRMusic.openPlaylist(event.detail || {});
  });

  window.addEventListener("shaer:library-root", () => {
    if (window.SHAeRMusic) window.SHAeRMusic.closePlaylist();
  });

  window.addEventListener("keydown", (event) => {
    if (event.key === "q" || event.key === "Q") showQueue();
    if (event.key === "+" || event.key === "=") changeVolume(5);
    if (event.key === "-" || event.key === "_") changeVolume(-5);
    if (event.key === "b" || event.key === "B") {
      showUniversalConnection("bluetooth", "Bluetooth", "No audio device is connected.", [
        { action: "close", label: "Paired" },
        { action: "close", label: "Available" },
        { action: "close", label: "Pair via Sync" }
      ]);
    }
    if (event.key === "Escape" && systemLayer && !systemLayer.hidden) closeOverlay();
  });

  window.addEventListener("shaer:spotify-error", (event) => {
    const [title, message] = mapSpotifyError(0, event.detail && event.detail.message);
    showPopup("error", title, message);
  });

  window.shaerHardware = {
    get inputMode() {
      return runtime.inputMode;
    },
    stop() {
      runtime.stopped = true;
      if (unsubscribeMusic) unsubscribeMusic();
      if (window.SHAeRMusic) window.SHAeRMusic.stop();
    },
    test(action) {
      dispatchAction(action);
    }
  };

  window.shaerUI = {
    playback(state) {
      applyPlayback(state);
    },
    queue(items) {
      runtime.queue = Array.isArray(items) ? items.slice() : [];
    },
    popup(kind, title, message) {
      showPopup(kind, title, message);
    },
    settingsDomain(setting) {
      showSettingsDomain(setting);
    },
    closePopup: closeOverlay,
    showQueue,
    changeVolume
  };

  window.shaerDiagnostics = {
    snapshot() {
      return {
        theme,
        source: runtime.playback.source,
        queueLength: runtime.queue.length,
        spotifyConfigured: runtime.spotifyConfigured,
        spotifyAvailable: runtime.spotifyAvailable,
        metrics: { ...runtime.metrics }
      };
    }
  };

  window.shaerValidation = {
    states: [
      "boot", "home", "library", "album", "now-playing", "volume", "queue",
      "recording", "bluetooth", "spotify-login", "onboarding-qr", "loading", "error", "shutdown"
    ],
    render(state) {
      closeOverlay();
      if (["home", "library", "album", "now-playing", "loading"].includes(state)) {
        window.dispatchEvent(new CustomEvent("shaer:validation-navigate", { detail: { state } }));
        window.setTimeout(() => applyPlayback(runtime.playback), 0);
        return;
      }
      if (state === "recording") {
        runtime.recording = {
          ...runtime.recording,
          state: "recording",
          elapsed_ms: 83000,
          storage_free: 3.4 * 1024 ** 3,
          level: 0.58,
          theme: pathTheme ? pathTheme[0] : "shaer_dark_archive"
        };
        window.dispatchEvent(new CustomEvent("shaer:validation-navigate", { detail: { state } }));
        window.setTimeout(syncRecordingDom, 0);
        return;
      }
      if (state === "boot") showBoot();
      if (state === "onboarding-qr") showOnboardingQr();
      if (state === "volume") showVolume();
      if (state === "queue") showQueue();
      if (state === "bluetooth") showUniversalConnection("bluetooth", "Bluetooth", "Looking for your trusted audio device.", [
        { action: "close", label: "Connected" },
        { action: "close", label: "Paired" },
        { action: "close", label: "Available" }
      ]);
      if (state === "spotify-login") showUniversalConnection("spotify-login", "Spotify Connect", "Authorize SHAeR securely to continue.", [
        { action: "cancel", label: "BACK" },
        { action: "close", label: "CONNECT" }
      ]);
      if (state === "error") showPopup("error", "CONNECTION LOST", "SHAeR will retry automatically.");
      if (state === "shutdown") showPopup("shutdown", "POWER OFF SHAeR?", "Playback will stop and the filesystem will shut down safely.", [
        { action: "cancel", label: "CANCEL" },
        { action: "shutdown", label: "POWER OFF" }
      ]);
    }
  };

  initSystemLayer();
  installDiagnosticFixture();
  window.requestAnimationFrame(frameTick);
  window.setInterval(reportMetrics, 30000);
  if (!validationMode) {
    pollHardware();
    pollPairing();
    pollRecording();
  }
  connectMusicStore();
})();
