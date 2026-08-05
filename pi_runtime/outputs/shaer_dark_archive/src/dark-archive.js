const screenTemplates = [
  ["home", "Home", "homeTemplate"],
  ["loading", "Loading", "loadingTemplate"],
  ["library-list", "Library", "libraryTemplate"],
  ["playlist-songs", "Tracks", "playlistSongsTemplate"],
  ["now-playing", "Now Playing", "playingTemplate"],
  ["memos", "Memos", "memosTemplate"],
  ["settings", "Settings", "settingsTemplate"],
  ["ascii-home", "About", "asciiTemplate"],
  ["charging", "Charging", "chargingTemplate"]
];

const routeHotspots = {
  home: [
    { target: "library-list", label: "[0] LOCAL FILES", left: 34, top: 118, width: 144, height: 13 },
    { target: "loading", label: "[1] SPOTIFY CONNECT", left: 34, top: 131, width: 144, height: 13 },
    { target: "memos", label: "[2] VOICE MEMOS", left: 34, top: 144, width: 144, height: 13 },
    { target: "settings", label: "[3] SETTINGS", left: 34, top: 157, width: 144, height: 13 }
  ],
  loading: [
    { target: "library-list", label: "CONTINUE", left: 25, top: 82, width: 190, height: 160 }
  ],
  "now-playing": [
    { action: "shuffle", label: "SHUFFLE", left: 20, top: 229, width: 35, height: 29 },
    { action: "previous", label: "PREVIOUS", left: 62, top: 229, width: 30, height: 29 },
    { action: "toggle-play", label: "PLAY OR PAUSE", left: 99, top: 226, width: 42, height: 35 },
    { action: "next", label: "NEXT", left: 148, top: 229, width: 30, height: 29 },
    { action: "repeat", label: "REPEAT", left: 185, top: 229, width: 35, height: 29 }
  ],
  memos: [
    { action: "recording-library", label: "ARCHIVE", left: 27, top: 238, width: 58, height: 24 },
    { action: "toggle-memo", label: "RECORD OR PAUSE", left: 99, top: 231, width: 42, height: 34 },
    { action: "save-memo", label: "SAVE", left: 153, top: 238, width: 54, height: 24 }
  ],
  settings: [
    { target: "ascii-home", setting: "ABOUT", label: "> ABOUT", left: 30, top: 62, width: 150, height: 13 },
    { setting: "APPEARANCE", label: "> APPEARANCE", left: 30, top: 75, width: 150, height: 13 },
    { setting: "PLAYBACK", label: "> PLAYBACK", left: 30, top: 88, width: 150, height: 13 },
    { setting: "AUDIO", label: "> AUDIO", left: 30, top: 101, width: 150, height: 13 },
    { setting: "CONNECTIVITY", label: "> CONNECTIVITY", left: 30, top: 114, width: 150, height: 13 },
    { target: "charging", setting: "POWER", label: "> POWER", left: 30, top: 127, width: 150, height: 13 },
    { setting: "DATE_TIME", label: "> DATE & TIME", left: 30, top: 140, width: 150, height: 13 },
    { setting: "SYNC", label: "> SYNC", left: 30, top: 153, width: 150, height: 13 },
    { setting: "ADVANCED", label: "> ADVANCED", left: 30, top: 166, width: 150, height: 13 }
  ],
  "ascii-home": [{ target: "home", label: "RETURN HOME", left: 0, top: 0, width: 240, height: 320 }],
  charging: [{ target: "home", label: "RETURN HOME", left: 0, top: 0, width: 240, height: 320 }]
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
  return window.SHAeRMusic
    ? window.SHAeRMusic.viewMessage(firmwareState.musicStatus)
    : "LOADING MUSIC";
}

function createHotspot(specification) {
  const button = document.createElement("button");
  button.type = "button";
  button.className = "archive-hotspot";
  button.dataset.nav = "";
  if (specification.target) button.dataset.target = specification.target;
  if (specification.action) button.dataset.action = specification.action;
  if (specification.setting) button.dataset.setting = specification.setting;
  if (specification.song != null) button.dataset.song = String(specification.song);
  if (specification.playlist != null) button.dataset.playlist = String(specification.playlist);
  button.dataset.label = specification.label;
  button.setAttribute("aria-label", specification.ariaLabel || specification.label);
  Object.assign(button.style, {
    left: `${specification.left}px`,
    top: `${specification.top}px`,
    width: `${specification.width}px`,
    height: `${specification.height}px`
  });
  return button;
}

function addHotspots(node, screenId, dynamic = []) {
  [...(routeHotspots[screenId] || []), ...dynamic].forEach((specification) => {
    node.appendChild(createHotspot(specification));
  });
}

function hydrateLoading(node) {
  const progress = node.querySelector("[data-loading-progress]");
  if (progress) progress.style.width = `${Math.max(0, Math.min(100, firmwareState.loadingProgress))}%`;
}

function hydrateChrome(node) {
  const clock = node.querySelector(".topbar span");
  const date = node.querySelector(".footer span");
  const battery = node.querySelector(".footer b");
  if (clock) clock.textContent = formatClock();
  if (date) date.textContent = formatDate();
  if (battery) battery.textContent = Number.isFinite(firmwareState.battery) ? `${firmwareState.battery}%` : "--%";
}

function hydrateLibrary(node) {
  const list = node.querySelector("[data-music-list]");
  const rows = [];
  const hotspots = [];
  let index = 0;

  if (firmwareState.songs.length) {
    rows.push(`[${index}] SAVED TRACKS (${firmwareState.songs.length})`);
    hotspots.push({ target: "playlist-songs", label: rows.at(-1), left: 24, top: 84 + (index * 13), width: 178, height: 13 });
    index += 1;
  }

  firmwareState.playlists.slice(0, 4).forEach((playlist, playlistIndex) => {
    const name = String(playlist.title || playlist.name || "UNTITLED PLAYLIST").toUpperCase();
    const count = Number(playlist.count ?? playlist.trackCount) || 0;
    rows.push(`[${index}] ${name}${count ? ` (${count})` : ""}`);
    hotspots.push({ playlist: playlistIndex, label: rows.at(-1), left: 24, top: 84 + (index * 13), width: 190, height: 13 });
    index += 1;
  });

  rows.push(`[${index}] RECORDINGS`);
  hotspots.push({ action: "recording-library", label: rows.at(-1), left: 24, top: 84 + (index * 13), width: 178, height: 13 });

  if (!firmwareState.songs.length && !firmwareState.playlists.length) {
    rows.unshift(musicMessage());
    hotspots.forEach((hotspot) => { hotspot.top += 13; });
  }

  list.innerHTML = rows.map((row) => `<p>${escapeHtml(row)}</p>`).join("");
  return hotspots;
}

function hydrateTracks(node) {
  const list = node.querySelector("[data-track-list]");
  const heading = node.querySelector("[data-music-heading]");
  const playlist = firmwareState.playlists[firmwareState.currentPlaylistIndex];
  if (heading) heading.textContent = playlist ? String(playlist.title || playlist.name).toUpperCase() : "SAVED TRACKS";

  if (!firmwareState.songs.length) {
    list.innerHTML = `<p>${escapeHtml(musicMessage())}</p>`;
    return [];
  }

  const rows = firmwareState.songs.slice(0, 6).map((track, index) => {
    const duration = track.duration ? ` ${track.duration}` : "";
    return `[${index}] ${String(track.title || "UNTITLED").toUpperCase()}${duration}`;
  });
  list.innerHTML = rows.map((row) => `<p>${escapeHtml(row)}</p>`).join("");
  return rows.map((label, song) => ({
    song,
    label,
    ariaLabel: `Play ${firmwareState.songs[song].title}`,
    left: 24,
    top: 82 + (song * 13),
    width: 190,
    height: 13
  }));
}

function hydrateNowPlaying(node) {
  const track = firmwareState.songs[firmwareState.currentTrackIndex] || null;
  const title = node.querySelector("[data-shaer-title]");
  const artist = node.querySelector("[data-shaer-artist]");
  const album = node.querySelector("[data-shaer-album]");
  const queue = node.querySelector("[data-shaer-queue]");

  if (title) title.textContent = track ? track.title : "NOTHING PLAYING";
  if (artist) artist.textContent = track ? (track.artistText || track.artist || "UNKNOWN ARTIST") : "CHOOSE MUSIC";
  if (album) album.textContent = track ? (track.album || "") : "";
  if (queue) {
    queue.innerHTML = firmwareState.queue.length
      ? firmwareState.queue.slice(0, 4).map((item, index) => `<li><span>${index + 1}.</span>${escapeHtml(item.title)}<b>${escapeHtml(item.duration || "")}</b></li>`).join("")
      : `<li>${escapeHtml(firmwareState.musicStatus === "ready" ? "QUEUE EMPTY" : musicMessage())}</li>`;
  }
}

window.SHAeRFirmware.mount({
  themeId: "shaer_dark_archive",
  state: firmwareState,
  screens: screenTemplates.map(([id, label]) => [id, label]),
  storageKey: "shaer-core-dark-archive",
  aliases: {
    home: "home",
    loading: "loading",
    library: "library-list",
    album: "playlist-songs",
    "now-playing": "now-playing",
    recordings: "memos",
    settings: "settings",
    charging: "charging",
    about: "ascii-home"
  },
  localToCanonical: {
    "library-list": "library",
    "playlist-songs": "album",
    "ascii-home": "about"
  },
  render(id) {
    const entry = screenTemplates.find(([screenId]) => screenId === id) || screenTemplates[0];
    const node = cloneTemplate(entry[2]);
    hydrateChrome(node);
    let dynamicHotspots = [];
    if (id === "loading") hydrateLoading(node);
    if (id === "library-list") dynamicHotspots = hydrateLibrary(node);
    if (id === "playlist-songs") dynamicHotspots = hydrateTracks(node);
    if (id === "now-playing") hydrateNowPlaying(node);
    addHotspots(node, id, dynamicHotspots);
    return node;
  }
});
