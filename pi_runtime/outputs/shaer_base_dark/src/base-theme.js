const themeId = document.body.dataset.baseTheme === "light" ? "shaer_base_light" : "shaer_base_dark";

const screenTemplates = [
  ["home", "Home", "homeTemplate"],
  ["loading", "Loading", "loadingTemplate"],
  ["library", "Library", "libraryTemplate"],
  ["tracks", "Tracks", "tracksTemplate"],
  ["now-playing", "Now Playing", "playingTemplate"],
  ["memos", "Memos", "memosTemplate"],
  ["settings", "Settings", "settingsTemplate"],
  ["about", "About", "aboutTemplate"],
  ["charging", "Charging", "chargingTemplate"]
];

const routeHotspots = {
  home: [
    { target: "library", label: "Local", left: 26, top: 88, width: 176, height: 36 },
    { target: "loading", label: "Spotify", left: 26, top: 132, width: 176, height: 36 },
    { target: "memos", label: "Memos", left: 26, top: 176, width: 176, height: 36 },
    { target: "settings", label: "Settings", left: 26, top: 220, width: 176, height: 36 }
  ],
  loading: [{ target: "library", label: "Continue", left: 0, top: 0, width: 240, height: 320 }],
  "now-playing": [
    { action: "previous", label: "Previous", left: 20, top: 256, width: 56, height: 28 },
    { action: "toggle-play", label: "Play or pause", left: 92, top: 256, width: 56, height: 28 },
    { action: "next", label: "Next", left: 164, top: 256, width: 56, height: 28 }
  ],
  memos: [
    { action: "recording-library", label: "Archive", left: 26, top: 176, width: 176, height: 30 },
    { action: "toggle-memo", label: "Record or pause", left: 26, top: 214, width: 176, height: 30 },
    { action: "save-memo", label: "Done", left: 26, top: 252, width: 176, height: 30 }
  ],
  settings: [
    { setting: "ABOUT", label: "About", left: 26, top: 88, width: 176, height: 36 },
    { setting: "APPEARANCE", label: "Appearance", left: 26, top: 132, width: 176, height: 36 },
    { setting: "PLAYBACK", label: "Playback", left: 26, top: 176, width: 176, height: 36 },
    { setting: "AUDIO", label: "Audio", left: 26, top: 220, width: 176, height: 36 },
    { setting: "CONNECTIVITY", label: "Connectivity", left: 26, top: 264, width: 176, height: 36 },
    { setting: "POWER", label: "Power", left: 26, top: 308, width: 176, height: 36 },
    { setting: "DATE_TIME", label: "Date and time", left: 26, top: 352, width: 176, height: 36 },
    { setting: "SYNC", label: "Sync", left: 26, top: 396, width: 176, height: 36 },
    { setting: "ADVANCED", label: "Advanced", left: 26, top: 440, width: 176, height: 36 }
  ],
  about: [{ target: "settings", label: "Back to settings", left: 0, top: 0, width: 240, height: 320 }],
  charging: [{ target: "home", label: "Return home", left: 0, top: 0, width: 240, height: 320 }]
};

const firmwareState = {
  active: "home",
  selectedIndex: 0,
  history: [],
  playing: false,
  playProgress: 0,
  currentTrackIndex: 0,
  currentPlaylistIndex: 0,
  recording: false,
  memoElapsed: 0,
  loadingProgress: 0,
  musicStatus: "loading",
  musicError: null,
  songs: [],
  playlists: [],
  queue: []
};

function formatClock(date = new Date()) {
  const hours = date.getHours();
  return `${String((hours % 12) || 12).padStart(2, "0")}:${String(date.getMinutes()).padStart(2, "0")} ${hours >= 12 ? "PM" : "AM"}`;
}

function formatDate(date = new Date()) {
  return date.toLocaleDateString("en-GB", { day: "2-digit", month: "short", year: "numeric" });
}

function escapeHtml(value) {
  return String(value == null ? "" : value).replace(/[&<>"']/g, (character) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    "\"": "&quot;",
    "'": "&#39;"
  }[character]));
}

function cloneTemplate(templateId) {
  const template = document.getElementById(templateId);
  return template.content.firstElementChild.cloneNode(true);
}

function musicMessage() {
  return window.SHAeRMusic ? window.SHAeRMusic.viewMessage(firmwareState.musicStatus) : "Loading music";
}

function addHotspots(node, screenId, dynamic = []) {
  [...(routeHotspots[screenId] || []), ...dynamic].forEach((specification) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "base-hotspot";
    button.dataset.nav = "";
    if (specification.target) button.dataset.target = specification.target;
    if (specification.action) button.dataset.action = specification.action;
    if (specification.setting) button.dataset.setting = specification.setting;
    if (specification.song != null) button.dataset.song = String(specification.song);
    if (specification.playlist != null) button.dataset.playlist = String(specification.playlist);
    button.dataset.label = specification.label;
    button.setAttribute("aria-label", specification.label);
    Object.assign(button.style, {
      left: `${specification.left}px`,
      top: `${specification.top}px`,
      width: `${specification.width}px`,
      height: `${specification.height}px`
    });
    node.appendChild(button);
  });
}

function hydrateChrome(node) {
  node.querySelectorAll("[data-clock]").forEach((item) => { item.textContent = formatClock(); });
  node.querySelectorAll("[data-date]").forEach((item) => { item.textContent = formatDate(); });
  node.querySelectorAll("[data-battery]").forEach((item) => {
    item.textContent = Number.isFinite(firmwareState.battery) ? `${firmwareState.battery}%` : "--%";
  });
}

function setIndicator(node, visible, total) {
  const indicator = node.querySelector("[data-scroll-indicator] span");
  if (!indicator) return;
  indicator.style.height = visible >= total ? "100%" : `${Math.max(24, Math.round((visible / total) * 168))}px`;
}

function hydrateLoading(node) {
  const progress = node.querySelector("[data-loading-progress]");
  if (progress) progress.style.width = `${Math.max(0, Math.min(100, firmwareState.loadingProgress))}%`;
}

function hydrateLibrary(node) {
  const list = node.querySelector("[data-music-list]");
  const rows = [];
  const hotspots = [];
  if (firmwareState.songs.length) rows.push({ label: `Saved (${firmwareState.songs.length})`, target: "tracks" });
  firmwareState.playlists.slice(0, 2).forEach((playlist, playlistIndex) => {
    rows.push({ label: String(playlist.title || playlist.name || "Playlist"), playlist: playlistIndex });
  });
  rows.push({ label: "Recordings", action: "recording-library" });
  if (!firmwareState.songs.length && !firmwareState.playlists.length) rows.unshift({ label: musicMessage() });
  list.innerHTML = rows.slice(0, 4).map((row) => `<p>${escapeHtml(row.label)}</p>`).join("");
  rows.slice(0, 4).forEach((row, index) => {
    hotspots.push({
      ...row,
      top: 88 + (index * 44),
      left: 26,
      width: 176,
      height: 36
    });
  });
  setIndicator(node, Math.min(4, rows.length), rows.length);
  return hotspots;
}

function hydrateTracks(node) {
  const list = node.querySelector("[data-track-list]");
  const playlist = firmwareState.playlists[firmwareState.currentPlaylistIndex];
  const heading = playlist ? String(playlist.title || playlist.name || "Tracks") : "Tracks";
  node.querySelectorAll("[data-music-heading], [data-short-heading]").forEach((item) => { item.textContent = heading; });
  if (!firmwareState.songs.length) {
    list.innerHTML = `<p>${escapeHtml(musicMessage())}</p>`;
    setIndicator(node, 1, 1);
    return [];
  }
  const rows = firmwareState.songs.slice(0, 4);
  list.innerHTML = rows.map((track) => `<p>${escapeHtml(track.title || "Untitled")}</p>`).join("");
  setIndicator(node, rows.length, firmwareState.songs.length);
  return rows.map((track, song) => ({
    song,
    label: `Play ${track.title || "track"}`,
    left: 26,
    top: 88 + (song * 44),
    width: 176,
    height: 36
  }));
}

function hydrateNowPlaying(node) {
  const track = firmwareState.songs[firmwareState.currentTrackIndex] || null;
  const progress = node.querySelector("[data-shaer-progress]");
  if (node.querySelector("[data-shaer-title]")) node.querySelector("[data-shaer-title]").textContent = track ? track.title : "Nothing playing";
  if (node.querySelector("[data-shaer-artist]")) node.querySelector("[data-shaer-artist]").textContent = track ? (track.artistText || track.artist || "Unknown artist") : "Choose music";
  if (node.querySelector("[data-shaer-album]")) node.querySelector("[data-shaer-album]").textContent = track ? (track.album || "") : "";
  if (progress) progress.style.width = `${Math.max(0, Math.min(100, firmwareState.playProgress || 0))}%`;
}

function hydrateCharging(node) {
  const level = Number.isFinite(firmwareState.battery) ? firmwareState.battery : null;
  const label = node.querySelector("[data-charge-label]");
  const bar = node.querySelector("[data-charge-progress]");
  if (label) label.textContent = level == null ? "--%" : `${level}%`;
  if (bar) bar.style.width = `${level == null ? 0 : Math.max(0, Math.min(100, level))}%`;
}

window.SHAeRFirmware.mount({
  themeId,
  state: firmwareState,
  screens: screenTemplates.map(([id, label]) => [id, label]),
  storageKey: `shaer-core-${themeId}`,
  aliases: {
    home: "home",
    loading: "loading",
    library: "library",
    album: "tracks",
    "now-playing": "now-playing",
    recordings: "memos",
    settings: "settings",
    charging: "charging",
    about: "about"
  },
  localToCanonical: {
    tracks: "album"
  },
  render(id) {
    const entry = screenTemplates.find(([screenId]) => screenId === id) || screenTemplates[0];
    const node = cloneTemplate(entry[2]);
    hydrateChrome(node);
    let dynamicHotspots = [];
    if (id === "loading") hydrateLoading(node);
    if (id === "library") dynamicHotspots = hydrateLibrary(node);
    if (id === "tracks") dynamicHotspots = hydrateTracks(node);
    if (id === "now-playing") hydrateNowPlaying(node);
    if (id === "charging") hydrateCharging(node);
    addHotspots(node, id, dynamicHotspots);
    return node;
  }
});
