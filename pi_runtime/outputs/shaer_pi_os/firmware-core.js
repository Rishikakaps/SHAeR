(function () {
  "use strict";

  const VERSION = "0.16.0";
  const MAX_LOG_ENTRIES = 300;
  const SCREEN_CHANGE_LOADING_MS = 1250;
  const logs = [];
  let runtime = null;

  const transitions = Object.freeze({
    boot: new Set(["home", "loading", "error"]),
    home: new Set(["library", "loading", "recordings", "settings", "charging", "about", "error"]),
    loading: new Set(["home", "library", "error"]),
    library: new Set(["home", "album", "now-playing", "recordings", "error"]),
    album: new Set(["library", "now-playing", "error"]),
    "now-playing": new Set(["home", "library", "album", "queue", "error"]),
    queue: new Set(["now-playing", "library", "error"]),
    recordings: new Set(["home", "library", "now-playing", "error"]),
    settings: new Set(["home", "recordings", "charging", "about", "error"]),
    charging: new Set(["home", "settings", "error"]),
    about: new Set(["home", "settings"]),
    error: new Set(["home", "library", "settings", "now-playing"])
  });

  function log(category, event, fields = {}) {
    const entry = {
      time: new Date().toISOString(),
      category: String(category),
      event: String(event),
      ...fields
    };
    logs.push(entry);
    if (logs.length > MAX_LOG_ENTRIES) logs.splice(0, logs.length - MAX_LOG_ENTRIES);
    return entry;
  }

  function safeRead(key) {
    try {
      const value = JSON.parse(window.localStorage.getItem(key) || "null");
      return value && typeof value === "object" ? value : null;
    } catch (error) {
      log("storage", "state-read-failed", { level: "error", message: String(error) });
      return null;
    }
  }

  function safeWrite(key, value) {
    try {
      window.localStorage.setItem(key, JSON.stringify(value));
    } catch (error) {
      log("storage", "state-write-failed", { level: "error", message: String(error) });
    }
  }

  function canonicalFor(localId) {
    if (!runtime) return "home";
    return runtime.localToCanonical[localId] || (transitions[localId] ? localId : "home");
  }

  function localFor(canonicalId) {
    if (!runtime) return canonicalId;
    return runtime.aliases[canonicalId] || canonicalId;
  }

  function hasScreen(localId) {
    return Boolean(runtime && runtime.screenIds.has(localId));
  }

  function normalizeState(reason) {
    const state = runtime.state;
    const songs = Array.isArray(state.songs) ? state.songs : [];
    state.songs = songs;
    state.selectedIndex = Number.isInteger(state.selectedIndex) ? state.selectedIndex : 0;
    if (!state.navWindows || typeof state.navWindows !== "object" || Array.isArray(state.navWindows)) state.navWindows = {};
    state.currentTrackIndex = Number.isInteger(state.currentTrackIndex) ? state.currentTrackIndex : 0;
    if (songs.length) state.currentTrackIndex = Math.max(0, Math.min(state.currentTrackIndex, songs.length - 1));
    else state.currentTrackIndex = 0;
    state.playing = Boolean(state.playing && songs.length);
    state.recording = Boolean(state.recording);
    if (state.recording && state.playing) {
      state.playing = false;
      log("state", "playback-stopped-for-recording", { reason });
    }
    if (!hasScreen(state.active)) state.active = localFor("home");
    if (!Array.isArray(state.history)) state.history = [];
    state.history = state.history.filter((entry) => hasScreen(typeof entry === "string" ? entry : entry && entry.active)).slice(-24);
  }

  function persistedState() {
    const state = runtime.state;
    return {
      active: state.active,
      currentTrackIndex: state.currentTrackIndex,
      playing: state.playing,
      playProgress: Number(state.playProgress) || 0,
      volume: Number(state.volume) || 50
    };
  }

  function persist() {
    normalizeState("persist");
    safeWrite(runtime.storageKey, persistedState());
  }

  function restore() {
    if (runtime.params.has("reset")) window.localStorage.removeItem(runtime.storageKey);
    const saved = safeRead(runtime.storageKey);
    if (!saved) return;
    if (hasScreen(saved.active)) runtime.state.active = saved.active;
    if (Number.isInteger(saved.currentTrackIndex)) runtime.state.currentTrackIndex = saved.currentTrackIndex;
    if (typeof saved.playing === "boolean") runtime.state.playing = saved.playing;
    if (Number.isFinite(saved.playProgress)) runtime.state.playProgress = saved.playProgress;
    if (Number.isFinite(saved.volume)) runtime.state.volume = saved.volume;
    normalizeState("restore");
  }

  function canTransition(from, to) {
    if (from === to) return true;
    return Boolean(transitions[from] && transitions[from].has(to));
  }

  function shouldShowScreenChangeLoading(fromLocal, localTarget, options) {
    if (options.skipLoadingInterstitial) return false;
    if (fromLocal === localTarget) return false;
    if (options.replace && !options.showTransition) return false;
    const loadingLocal = localFor("loading");
    if (!hasScreen(loadingLocal)) return false;
    const from = canonicalFor(fromLocal);
    const to = canonicalFor(localTarget);
    return from !== "loading" && to !== "loading" && to !== "error";
  }

  function commitTransition(localTarget, options = {}, logContext = {}) {
    const fromLocal = logContext.fromLocal || runtime.state.active;
    const from = logContext.from || canonicalFor(fromLocal);
    const to = logContext.to || canonicalFor(localTarget);
    if (!options.replace && !options.back && !options.historyHandled && localTarget !== fromLocal) {
      runtime.state.history.push({
        active: fromLocal,
        selectedIndex: runtime.state.selectedIndex,
        windowStart: currentWindow().windowStart
      });
    }
    runtime.state.active = localTarget;
    runtime.state.selectedIndex = Number.isInteger(options.selectedIndex) ? options.selectedIndex : 0;
    if (Number.isInteger(options.windowStart)) currentWindow().windowStart = options.windowStart;
    normalizeState("transition");
    persist();
    renderLive();
    log("navigation", "transition", { from, to, theme: runtime.themeId });
    if (to === "loading") startLoading();
    return true;
  }

  function transition(target, options = {}) {
    if (!runtime) return false;
    const localTarget = hasScreen(target) ? target : localFor(target);
    if (!hasScreen(localTarget)) {
      log("navigation", "unknown-target", { level: "error", target: String(target) });
      return false;
    }
    const fromLocal = runtime.state.active;
    const from = canonicalFor(fromLocal);
    const to = canonicalFor(localTarget);
    if (!options.force && !options.back && !canTransition(from, to)) {
      log("navigation", "illegal-transition", { level: "error", from, to, theme: runtime.themeId });
      showError("That page is unavailable from here.");
      return false;
    }
    if (shouldShowScreenChangeLoading(fromLocal, localTarget, options)) {
      if (!options.replace && !options.back && localTarget !== fromLocal) {
        runtime.state.history.push({
          active: fromLocal,
          selectedIndex: runtime.state.selectedIndex,
          windowStart: currentWindow().windowStart
        });
      }
      runtime.state.active = localFor("loading");
      runtime.state.selectedIndex = 0;
      runtime.state.loadingProgress = 0;
      normalizeState("loading-interstitial");
      renderLive();
      log("navigation", "loading-interstitial", { from, to, theme: runtime.themeId });
      startLoading({
        localTarget,
        options: {
          ...options,
          replace: true,
          force: true,
          historyHandled: true,
          skipLoadingInterstitial: true
        },
        fromLocal,
        from,
        to
      });
      return true;
    }
    return commitTransition(localTarget, options, { fromLocal, from, to });
  }

  function back() {
    if (!runtime) return;
    const previous = runtime.state.history.pop();
    const previousId = typeof previous === "string" ? previous : previous && previous.active;
    if (previousId && hasScreen(previousId)) {
      transition(previousId, {
        replace: true,
        back: true,
        showTransition: true,
        selectedIndex: typeof previous === "object" && Number.isInteger(previous.selectedIndex) ? previous.selectedIndex : 0,
        windowStart: typeof previous === "object" && Number.isInteger(previous.windowStart) ? previous.windowStart : 0
      });
      return;
    }
    if (canonicalFor(runtime.state.active) !== "home") transition("home", { replace: true, back: true, force: true, showTransition: true });
  }

  function selectableItems() {
    return runtime ? Array.from(runtime.liveScreen.querySelectorAll("[data-nav]:not([hidden]):not([disabled])")) : [];
  }

  function pageKey() {
    return runtime ? runtime.state.active : "home";
  }

  function currentWindow() {
    if (!runtime.state.navWindows[pageKey()]) runtime.state.navWindows[pageKey()] = { windowStart: 0, visibleRows: 4, itemCount: 0 };
    return runtime.state.navWindows[pageKey()];
  }

  function inferredVisibleRows(items) {
    const explicit = items.find((item) => item.closest("[data-visible-rows]"));
    const value = explicit ? Number(explicit.closest("[data-visible-rows]").dataset.visibleRows) : NaN;
    if (Number.isInteger(value) && value >= 1) return value;
    return items.length > 4 ? 4 : Math.max(1, items.length);
  }

  function updateStaticRows(windowState) {
    const rows = Array.from(runtime.liveScreen.querySelectorAll(".menu-list[data-menu] > p, .settings-list > p, .terminal-list > p"));
    if (!rows.length || rows.length !== windowState.itemCount) return;
    rows.forEach((row, index) => {
      const visible = index >= windowState.windowStart && index < windowState.windowStart + windowState.visibleRows;
      row.hidden = !visible;
      if (visible) row.style.removeProperty("display");
      else row.style.display = "none";
      row.classList.toggle("is-selected", index === runtime.state.selectedIndex);
    });
  }

  function shouldWindowItem(item, host, windowState) {
    if (!host || windowState.itemCount <= windowState.visibleRows) return false;
    if (item.closest(".player-controls, .ticket-controls, .jp-controls, .raga-controls, .winamp-controls, .player-buttons, .memo-buttons, .memo-actions, .xp-recorder-controls, .taskbar")) return false;
    return Boolean(item.closest(
      ".settings-list, .setting-list, .settings-card, .settings-menu, .settings-panel, .terminal-list, .menu-list[data-menu], .library-card, .raga-list, .xp-menu, .jp-card-grid, .punk-list, .playlist-list, .top-songs"
    ));
  }

  function updateWindowedItems(items, windowState) {
    const absoluteItems = items.filter((item) => {
      const style = getComputedStyle(item);
      return style.position === "absolute" && (item.classList.contains("archive-hotspot") || item.classList.contains("base-hotspot"));
    });
    const firstTop = absoluteItems.length > 1 ? parseFloat(absoluteItems[0].dataset.originalTop || absoluteItems[0].style.top || "0") : 0;
    const secondTop = absoluteItems.length > 1 ? parseFloat(absoluteItems[1].dataset.originalTop || absoluteItems[1].style.top || "0") : firstTop;
    const rowStep = Math.max(1, Math.abs(secondTop - firstTop) || 1);
    items.forEach((item, index) => {
      const host = item.closest("[data-menu], .menu-list, .settings-list, .setting-list, .xp-menu, .library-card, .raga-list, .home-menu, .settings-card, .settings-panel, .memo-controls, .top-songs, .playlist-list, .jp-card-grid, .punk-list");
      const windowed = shouldWindowItem(item, host, windowState);
      const visible = !windowed || (index >= windowState.windowStart && index < windowState.windowStart + windowState.visibleRows);
      if (windowed && !visible) item.style.display = "none";
      else item.style.removeProperty("display");
      if (absoluteItems.includes(item)) {
        if (!item.dataset.originalTop) item.dataset.originalTop = item.style.top || `${item.offsetTop}px`;
        const originalTop = parseFloat(item.dataset.originalTop || "0");
        item.style.top = `${originalTop - (windowState.windowStart * rowStep)}px`;
      }
    });
  }

  function updateScrollHosts(items, windowState) {
    const hosts = new Set();
    items.forEach((item) => {
      const host = item.closest("[data-menu], .menu-list, .settings-list, .setting-list, .xp-menu, .library-card, .raga-list, .home-menu, .settings-card, .memo-controls, .top-songs, .playlist-list");
      if (host) hosts.add(host);
    });
    hosts.forEach((host) => {
      host.dataset.itemCount = String(windowState.itemCount);
      host.dataset.windowStart = String(windowState.windowStart);
      host.dataset.selectedIndex = String(runtime.state.selectedIndex);
      host.dataset.visibleRows = String(windowState.visibleRows);
      const selected = items[runtime.state.selectedIndex];
      if (selected && host.contains(selected)) {
        const maxTop = Math.max(0, host.scrollHeight - host.clientHeight);
        const targetTop = Math.min(maxTop, Math.max(0, selected.offsetTop - host.offsetTop));
        host.scrollTop = targetTop;
      }
    });
  }

  function updateScrollIndicators(windowState) {
    const thumbRatio = windowState.itemCount > 0 ? Math.min(1, windowState.visibleRows / windowState.itemCount) : 1;
    const travelRatio = windowState.itemCount > windowState.visibleRows
      ? windowState.windowStart / (windowState.itemCount - windowState.visibleRows)
      : 0;
    runtime.liveScreen.querySelectorAll(".position-indicator span, [data-scroll-indicator] span").forEach((thumb) => {
      thumb.style.height = `${Math.max(18, Math.round(thumbRatio * 100))}%`;
      thumb.style.marginTop = `${Math.round((1 - thumbRatio) * travelRatio * 100)}%`;
      thumb.style.transform = "none";
    });
  }

  function applyNavigationWindow(items) {
    if (!items.length) return;
    const windowState = currentWindow();
    const itemCount = items.length;
    const visibleRows = Math.min(itemCount, inferredVisibleRows(items));
    runtime.state.selectedIndex = Math.max(0, Math.min(runtime.state.selectedIndex, itemCount - 1));
    let windowStart = Number.isInteger(windowState.windowStart) ? windowState.windowStart : 0;
    if (runtime.state.selectedIndex < windowStart) windowStart = runtime.state.selectedIndex;
    if (runtime.state.selectedIndex >= windowStart + visibleRows) windowStart = runtime.state.selectedIndex - visibleRows + 1;
    windowStart = Math.max(0, Math.min(windowStart, Math.max(0, itemCount - visibleRows)));
    Object.assign(windowState, { itemCount, visibleRows, windowStart });
    runtime.liveScreen.dataset.selectedIndex = String(runtime.state.selectedIndex);
    runtime.liveScreen.dataset.windowStart = String(windowStart);
    runtime.liveScreen.dataset.visibleRows = String(visibleRows);
    runtime.liveScreen.dataset.itemCount = String(itemCount);
    updateWindowedItems(items, windowState);
    updateStaticRows(windowState);
    updateScrollHosts(items, windowState);
    updateScrollIndicators(windowState);
    window.dispatchEvent(new CustomEvent("shaer:navigation-window", {
      detail: {
        page: canonicalFor(runtime.state.active),
        localPage: runtime.state.active,
        selectedIndex: runtime.state.selectedIndex,
        ...windowState
      }
    }));
  }

  function focusSelected() {
    const items = selectableItems();
    if (!items.length) return;
    applyNavigationWindow(items);
    items.forEach((item, index) => {
      item.tabIndex = index === runtime.state.selectedIndex ? 0 : -1;
      item.classList.toggle("is-selected", index === runtime.state.selectedIndex);
      item.setAttribute("aria-selected", String(index === runtime.state.selectedIndex));
    });
    items[runtime.state.selectedIndex].focus({ preventScroll: true });
  }

  function moveSelection(delta) {
    const items = selectableItems();
    if (!items.length) return;
    runtime.state.selectedIndex = Math.max(0, Math.min(runtime.state.selectedIndex + delta, items.length - 1));
    focusSelected();
    log("input", "encoder", { delta, page: canonicalFor(runtime.state.active) });
  }

  function activateSelection() {
    const items = selectableItems();
    if (items[runtime.state.selectedIndex]) items[runtime.state.selectedIndex].click();
  }

  function currentTrack() {
    return runtime.state.songs[runtime.state.currentTrackIndex] || null;
  }

  function setTrack(index) {
    const next = Number(index);
    if (!Number.isInteger(next) || next < 0 || next >= runtime.state.songs.length) {
      showError("This track is no longer available.");
      return;
    }
    runtime.state.currentTrackIndex = next;
    runtime.state.playing = true;
    runtime.state.playProgress = 0;
    normalizeState("select-track");
    transition("now-playing", { force: true });
    log("playback", "track-selected", { index: next });
    window.dispatchEvent(new CustomEvent("shaer:track-select", { detail: { ...runtime.state.songs[next] } }));
  }

  function setPlaylist(index) {
    const next = Number(index);
    if (!Number.isInteger(next) || next < 0 || next >= runtime.state.playlists.length) {
      showError("This playlist is no longer available.");
      return;
    }
    runtime.state.currentPlaylistIndex = next;
    runtime.state.songs = [];
    runtime.state.musicStatus = "loading";
    transition("album", { force: true });
    window.dispatchEvent(new CustomEvent("shaer:playlist-select", { detail: { ...runtime.state.playlists[next] } }));
    log("playback", "playlist-opened", { index: next });
  }

  function playbackAction(action) {
    const songs = runtime.state.songs;
    if (!songs.length) {
      runtime.state.playing = false;
      showError("No playable track is available.");
      return;
    }
    if (action === "toggle-play") runtime.state.playing = !runtime.state.playing;
    if (action === "play") runtime.state.playing = true;
    if (action === "pause") runtime.state.playing = false;
    if (action === "next") runtime.state.currentTrackIndex = (runtime.state.currentTrackIndex + 1) % songs.length;
    if (action === "previous") runtime.state.currentTrackIndex = (runtime.state.currentTrackIndex - 1 + songs.length) % songs.length;
    if (["next", "previous"].includes(action)) runtime.state.playProgress = 0;
    normalizeState("playback-action");
    persist();
    renderLive();
    if (["next", "previous"].includes(action)) {
      const track = currentTrack();
      if (track && (track.isLocal || track.source === "local")) {
        window.dispatchEvent(new CustomEvent("shaer:track-select", { detail: { ...track } }));
      }
    }
    log("playback", action, { track: currentTrack() && currentTrack().title });
  }

  function showError(message) {
    if (window.shaerUI && typeof window.shaerUI.popup === "function") window.shaerUI.popup("error", "SHAeR", message);
    else log("ui", "error-popup", { level: "error", message });
  }

  function showToast(message) {
    const screen = runtime.liveScreen.querySelector(".screen, article");
    if (!screen) return;
    let toast = screen.querySelector(".toast");
    if (!toast) {
      toast = document.createElement("div");
      toast.className = "toast";
      screen.appendChild(toast);
    }
    toast.textContent = message;
    toast.classList.add("show");
    window.clearTimeout(runtime.toastTimer);
    runtime.toastTimer = window.setTimeout(() => toast.classList.remove("show"), 900);
  }

  function settingAction(name) {
    const setting = String(name || "").trim().toUpperCase();
    const domain = setting === "DATE & TIME" ? "DATE_TIME" : setting.replace(/\s+/g, "_");
    const implemented = new Set(["ABOUT", "APPEARANCE", "PLAYBACK", "AUDIO", "CONNECTIVITY", "POWER", "DATE_TIME", "SYNC", "ADVANCED"]);
    if (setting === "RECORDER") return transition("recordings", { force: true });
    if (implemented.has(domain)) {
      if (window.shaerUI && typeof window.shaerUI.settingsDomain === "function") {
        window.shaerUI.settingsDomain(domain);
      } else if (window.shaerUI && typeof window.shaerUI.popup === "function") {
        window.shaerUI.popup("settings", domain.replace(/_/g, " "), "Settings are available on the device runtime.");
      } else showToast(`${domain.replace(/_/g, " ")} SETTINGS`);
      return;
    }
    showToast(`${setting} UNAVAILABLE`);
  }

  function handleAction(action) {
    if (action === "back") return back();
    if (["toggle-play", "play", "pause", "next", "previous"].includes(action)) return playbackAction(action);
    if (["shuffle", "repeat"].includes(action)) {
      runtime.state[action] = !runtime.state[action];
      persist();
      renderLive();
      return;
    }
    if (action === "toggle-memo") {
      runtime.state.recording = !runtime.state.recording;
      normalizeState("recording-toggle");
      persist();
      renderLive();
      return;
    }
    if (["save-memo", "delete-memo"].includes(action)) {
      runtime.state.recording = false;
      runtime.state.memoElapsed = 0;
      persist();
      renderLive();
      return;
    }
    showToast(String(action).replace(/-/g, " ").toUpperCase());
  }

  function click(event) {
    const target = event.target.closest("button, [data-target], [data-song], [data-action], [data-setting], [data-playlist]");
    if (!target || !runtime.liveScreen.contains(target)) return;
    event.preventDefault();
    event.stopImmediatePropagation();
    if (target.dataset.setting) return settingAction(target.dataset.setting);
    if (target.dataset.target) {
      const localTarget = hasScreen(target.dataset.target) ? target.dataset.target : localFor(target.dataset.target);
      if (canonicalFor(localTarget) === "album") window.dispatchEvent(new CustomEvent("shaer:library-root"));
      return transition(target.dataset.target);
    }
    if (target.dataset.song != null) return setTrack(target.dataset.song);
    if (target.dataset.playlist != null) return setPlaylist(target.dataset.playlist);
    if (target.dataset.action) return handleAction(target.dataset.action);
  }

  function applySettingVisibility() {
    const implemented = new Set(["ABOUT", "APPEARANCE", "PLAYBACK", "AUDIO", "CONNECTIVITY", "POWER", "DATE_TIME", "DATE & TIME", "SYNC", "ADVANCED"]);
    runtime.liveScreen.querySelectorAll("[data-setting]").forEach((node) => {
      const key = String(node.dataset.setting || "").trim().toUpperCase();
      const supported = implemented.has(key) || implemented.has(key.replace(/\s+/g, "_"));
      node.hidden = !supported;
      if (!supported) node.style.display = "none";
      else node.style.removeProperty("display");
    });
    runtime.liveScreen.querySelectorAll(".settings-list p").forEach((node) => {
      const name = String(node.textContent || "").replace(/^>\s*/, "").trim().toUpperCase();
      const supported = implemented.has(name) || implemented.has(name.replace(/\s+/g, "_"));
      if (!supported) {
        node.hidden = true;
        node.style.display = "none";
      } else {
        node.hidden = false;
        node.style.removeProperty("display");
      }
    });
  }

  function renderResult(localId) {
    const result = runtime.render(localId, runtime.state);
    if (result instanceof Node) return result;
    const wrapper = document.createElement("div");
    wrapper.innerHTML = String(result || "");
    return wrapper.firstElementChild || document.createElement("article");
  }

  function renderLive() {
    normalizeState("render");
    document.body.dataset.shaerPage = canonicalFor(runtime.state.active);
    document.body.dataset.shaerLocalPage = runtime.state.active;
    const node = renderResult(runtime.state.active);
    runtime.liveScreen.replaceChildren(node);
    applySettingVisibility();
    focusSelected();
    if (runtime.picker) runtime.picker.querySelectorAll("button").forEach((button) => button.classList.toggle("active", button.dataset.screen === runtime.state.active));
  }

  function renderPreviewChrome() {
    if (runtime.deviceMode) return;
    if (runtime.picker) {
      runtime.picker.replaceChildren();
      runtime.screens.forEach(([id, label]) => {
        const button = document.createElement("button");
        button.type = "button";
        button.dataset.screen = id;
        button.textContent = label;
        button.addEventListener("click", () => transition(id, { force: true }));
        runtime.picker.appendChild(button);
      });
    }
    if (runtime.strip) {
      runtime.strip.replaceChildren();
      runtime.screens.forEach(([id]) => {
        const node = renderResult(id);
        node.style.transform = "scale(.58)";
        node.style.marginRight = "-100px";
        runtime.strip.appendChild(node);
      });
    }
  }

  function startLoading(pendingTransition = null) {
    window.clearTimeout(runtime.loadingTimer);
    if (!Number.isFinite(runtime.state.loadingProgress)) runtime.state.loadingProgress = 0;
    const startedAt = Date.now();
    const tick = () => {
      if (canonicalFor(runtime.state.active) !== "loading") return;
      const elapsed = Date.now() - startedAt;
      runtime.state.loadingProgress = Math.min(100, Math.round((elapsed / SCREEN_CHANGE_LOADING_MS) * 100));
      renderLive();
      if (runtime.state.loadingProgress >= 100) {
        if (pendingTransition) {
          commitTransition(pendingTransition.localTarget, pendingTransition.options, {
            fromLocal: pendingTransition.fromLocal,
            from: pendingTransition.from,
            to: pendingTransition.to
          });
        } else {
          transition("library", { replace: true, force: true, skipLoadingInterstitial: true });
        }
      }
      else runtime.loadingTimer = window.setTimeout(tick, 120);
    };
    runtime.state.loadingProgress = 0;
    renderLive();
    runtime.loadingTimer = window.setTimeout(tick, 80);
  }

  function keydown(event) {
    if (!runtime) return;
    if (["ArrowRight", "ArrowDown"].includes(event.key)) {
      event.preventDefault();
      event.stopImmediatePropagation();
      moveSelection(1);
    } else if (["ArrowLeft", "ArrowUp"].includes(event.key)) {
      event.preventDefault();
      event.stopImmediatePropagation();
      moveSelection(-1);
    } else if (["Enter", " "].includes(event.key)) {
      event.preventDefault();
      event.stopImmediatePropagation();
      activateSelection();
    } else if (["Escape", "Backspace"].includes(event.key)) {
      event.preventDefault();
      event.stopImmediatePropagation();
      back();
    }
  }

  function validationNavigate(event) {
    const state = event.detail && event.detail.state;
    if (!["home", "library", "album", "now-playing", "recording", "loading"].includes(state)) return;
    event.stopImmediatePropagation();
    transition(state === "recording" ? "recordings" : state, { replace: true, force: true });
  }

  function playbackUpdate(event) {
    if (!runtime || !event.detail) return;
    const detail = event.detail;
    runtime.state.provider = String(detail.source || "local");
    runtime.state.playing = String(detail.status || "").toLowerCase() === "playing";
    if (runtime.state.playing && !detail.title && !detail.uri) {
      runtime.state.playing = false;
      log("state", "rejected-impossible-playback", { level: "error", provider: runtime.state.provider });
    }
    normalizeState("provider-update");
  }

  function musicStateUpdate(event) {
    if (!runtime || !event.detail) return;
    const music = event.detail;
    const playbackTrack = music.currentPlayback && music.currentPlayback.currentTrack;
    const preferred = music.selectedPlaylist
      ? music.playlistTracks
      : (music.spotify && music.spotify.authenticated ? music.savedTracks : music.localTracks);
    const candidates = [
      ...(music.selectedPlaylist ? [] : [playbackTrack]),
      ...(Array.isArray(preferred) ? preferred : [])
    ].filter(Boolean);
    const seen = new Set();
    runtime.state.songs = candidates.filter((track) => {
      const key = String(track.uri || track.id || `${track.title}|${track.artistText || track.artist}`);
      if (seen.has(key)) return false;
      seen.add(key);
      return true;
    });
    runtime.state.playlists = Array.isArray(music.playlists) ? music.playlists.slice() : [];
    runtime.state.selectedPlaylist = music.selectedPlaylist || null;
    runtime.state.queue = Array.isArray(music.queue) ? music.queue.slice() : [];
    runtime.state.musicStatus = String(music.status || "loading");
    runtime.state.musicError = music.error || null;
    runtime.state.spotify = music.spotify || { configured: false, authenticated: false };
    if (playbackTrack && runtime.state.songs.length) runtime.state.currentTrackIndex = 0;
    normalizeState("music-state-update");
    renderLive();
  }

  async function refreshCapabilities() {
    try {
      const response = await window.fetch("/api/system/capabilities", {
        cache: "no-store",
        headers: { Accept: "application/json" }
      });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      const result = await response.json();
      const capabilities = result && result.capabilities;
      if (capabilities && typeof capabilities === "object") {
        runtime.capabilities.power = capabilities.power === true;
        runtime.capabilities.recording = capabilities.recording !== false;
        document.body.dataset.shaerPower = runtime.capabilities.power ? "enabled" : "disabled";
        renderLive();
      }
    } catch (error) {
      log("capability", "discovery-unavailable", { message: String(error) });
    }
  }

  function mount(adapter) {
    if (runtime) throw new Error("SHAeR firmware core can mount only one theme renderer.");
    const params = new URLSearchParams(window.location.search);
    const aliases = { ...adapter.aliases };
    const localToCanonical = { ...(adapter.localToCanonical || {}) };
    Object.entries(aliases).forEach(([canonical, local]) => {
      if (!localToCanonical[local]) localToCanonical[local] = canonical;
    });
    runtime = {
      themeId: adapter.themeId,
      state: adapter.state,
      screens: adapter.screens,
      screenIds: new Set(adapter.screens.map(([id]) => id)),
      render: adapter.render,
      aliases,
      localToCanonical,
      storageKey: adapter.storageKey || `shaer-core-${adapter.themeId}`,
      liveScreen: adapter.liveScreen || document.getElementById("liveScreen"),
      picker: adapter.picker || document.getElementById("screenPicker"),
      strip: adapter.strip || document.getElementById("screenStrip"),
      params,
      deviceMode: params.get("mode") === "device" || window.location.hash === "#device",
      loadingTimer: null,
      toastTimer: null,
      capabilities: { power: false, recording: true }
    };
    document.body.dataset.mode = runtime.deviceMode ? "device" : "preview";
    document.body.dataset.firmwareCore = VERSION;
    restore();
    normalizeState("mount");
    runtime.liveScreen.addEventListener("click", click, true);
    window.addEventListener("keydown", keydown, true);
    window.addEventListener("shaer:validation-navigate", validationNavigate, true);
    window.addEventListener("shaer:playback", playbackUpdate);
    window.addEventListener("shaer:music-state", musicStateUpdate);
    renderPreviewChrome();
    renderLive();
    refreshCapabilities();
    log("firmware", "mounted", { theme: runtime.themeId, version: VERSION });
    return api;
  }

  function snapshot() {
    if (!runtime) return null;
    normalizeState("snapshot");
    return {
      version: VERSION,
      theme: runtime.themeId,
      page: canonicalFor(runtime.state.active),
      localPage: runtime.state.active,
      selectedIndex: runtime.state.selectedIndex,
      history: runtime.state.history.map((entry) => canonicalFor(typeof entry === "string" ? entry : entry.active)),
      windowStart: currentWindow().windowStart,
      visibleRows: currentWindow().visibleRows,
      loadingProgress: Number(runtime.state.loadingProgress) || 0,
      playback: {
        provider: runtime.state.provider || "local",
        playing: runtime.state.playing,
        track: currentTrack()
      },
      recording: Boolean(runtime.state.recording),
      navigation: {
        album: localFor("album") !== localFor("library")
      },
      capabilities: { ...runtime.capabilities }
    };
  }

  const api = Object.freeze({ mount, transition, back, dispatch: handleAction, snapshot, logs: () => logs.slice(), version: VERSION });
  window.SHAeRFirmware = api;
  window.SHAeRLog = Object.freeze({ write: log, entries: () => logs.slice() });
})();
