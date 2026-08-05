const appState = {
  active: "home",
  selectedIndex: 0,
  history: [],
  battery: null,
  loadingProgress: 18,
  playing: false,
  playProgress: 42,
  currentTrackIndex: 0,
  recording: false,
  memoElapsed: 28,
  musicStatus: "loading",
  songs: [],
  playlists: [],
  settings: ["About", "Appearance", "Playback", "Audio", "Connectivity", "Power", "Date & Time", "Sync", "Advanced"]
};

const screens = [
  ["home", "Home"],
  ["loading", "Loading"],
  ["library", "Library"],
  ["now", "Playing"],
  ["memos", "Memos"],
  ["settings", "Settings"],
  ["charge", "Charge"]
];

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, (char) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    "\"": "&quot;",
    "'": "&#39;"
  }[char]));
}

function timeText() {
  const date = new Date();
  const hour = date.getHours();
  const hh = String((hour % 12) || 12).padStart(2, "0");
  const mm = String(date.getMinutes()).padStart(2, "0");
  return `${hh}:${mm} ${hour >= 12 ? "PM" : "AM"}`;
}

function dateText(date = new Date()) {
  return date.toLocaleDateString("en-GB", { day: "2-digit", month: "short", year: "2-digit" }).toUpperCase();
}

function borderFor(id) {
  const map = {
    home: "screens/blue-elephants.png",
    loading: "screens/arch-loading.png",
    library: "screens/red-hands.png",
    now: "screens/pink-floral.png",
    memos: "screens/teal-floral.png",
    settings: "screens/black-leaves.png",
    charge: "screens/pink-panel.png"
  };
  return `assets/${map[id]}`;
}

function shell(id, label, body) {
  const hasLiveChrome = !["charge", "loading", "home", "library", "now", "memos", "settings"].includes(id);
  return `<article class="screen screen-${id}" data-screen="${id}">
    <img class="border-img" src="${borderFor(id)}" alt="">
    ${hasLiveChrome ? `<div class="topline"><span>${escapeHtml(label)}</span><b>${dateText()}</b></div>` : ""}
    ${body}
    ${hasLiveChrome ? `<div class="footer"><span>${Number.isFinite(appState.battery) ? `${appState.battery}%` : "--%"}</span><b>${timeText()}</b></div>` : ""}
  </article>`;
}

function navButton(className, attrs = "", content = "") {
  return `<button class="${className}" ${attrs} data-nav type="button">${content}</button>`;
}

function currentTrack() {
  return appState.songs[appState.currentTrackIndex] || null;
}

function musicMessage() {
  return window.SHAeRMusic ? window.SHAeRMusic.viewMessage(appState.musicStatus) : "LOADING MUSIC";
}

function renderHome() {
  const items = [
    ["Local Music", "library"],
    ["Spotify Connect", "loading"],
    ["Voice Memos", "memos"],
    ["Settings", "settings"]
  ].map(([label, target]) => navButton("home-row", `data-target="${target}" aria-label="${label}"`, label)).join("");
  return shell("home", "Menu Bar", `<section class="home-menu">${items}</section>`);
}

function renderLoading() {
  return shell("loading", "Loading", `
    <section class="mini-progress" aria-label="Loading">
      <span>LOADING</span>
      <i style="--p:${Math.max(0, Math.min(100, appState.loadingProgress))}%"></i>
    </section>
    ${navButton("wide-hit", "data-target=\"library\" aria-label=\"Continue\"", "")}
  `);
}

function renderLibrary() {
  const playlists = appState.playlists.map((playlist, index) => navButton("library-row", `data-playlist="${index}" aria-label="Open ${escapeHtml(playlist.title)}"`, `${escapeHtml(playlist.title)} (${playlist.count})`)).join("");
  const rows = appState.songs.length ? appState.songs.map((song, index) => navButton("library-row", `data-song="${index}" aria-label="Play ${escapeHtml(song.title)}"`, `${escapeHtml(song.title)}`)).join("") : `<div class="library-row music-state-row">${escapeHtml(musicMessage())}</div>`;
  const recordings = navButton("library-row", "data-action=\"recording-library\" aria-label=\"Open recording archive\"", "Recordings");
  return shell("library", "Library", `<section class="raga-list">${playlists}${rows}${recordings}</section>`);
}

function renderNow() {
  const track = currentTrack();
  const title = track ? track.title : "NOTHING PLAYING";
  const artist = track ? track.artist : "CHOOSE MUSIC";
  const album = track ? track.album : "";
  return shell("now", "Home", `
    <section class="raag-card">
      <div class="palace-art" data-shaer-cover></div>
      <p><span data-shaer-artist>${escapeHtml(artist)}</span>${album ? ` - <span data-shaer-album>${escapeHtml(album)}</span>` : `<span data-shaer-album></span>`}</p>
      <h1 data-shaer-title>${escapeHtml(title)}</h1>
      <div class="raga-progress"><i data-shaer-progress style="--p:${appState.playProgress}%"></i></div>
      <div class="raga-controls">
        ${navButton("raga-btn", "data-action=\"previous\" aria-label=\"Previous\"", "‹")}
        ${navButton("raga-btn play", "data-action=\"toggle-play\" aria-label=\"Play pause\"", appState.playing ? "Ⅱ" : "▶")}
        ${navButton("raga-btn", "data-action=\"next\" aria-label=\"Next\"", "›")}
      </div>
    </section>
  `);
}

function renderMemos() {
  const bars = [22, 36, 48, 25, 62, 44, 29, 51, 70, 33, 20, 46, 58].map((height, index) => `<i style="--h:${height}px;--i:${index}"></i>`).join("");
  return shell("memos", "Home", `
    <section class="memo-raga">
      <div class="wave-line">${bars}</div>
      <strong>${String(Math.floor(appState.memoElapsed / 60)).padStart(2, "0")}:${String(appState.memoElapsed % 60).padStart(2, "0")}</strong>
      <div class="memo-buttons">
        ${navButton("memo-round", "data-action=\"toggle-memo\" aria-label=\"Record\"", "REC")}
        ${navButton("memo-round", "data-action=\"toggle-play\" aria-label=\"Pause\"", appState.recording ? "STOP" : "PLAY")}
        ${navButton("memo-round", "data-action=\"save-memo\" aria-label=\"Finish and save\"", "DONE")}
      </div>
    </section>
  `);
}

function renderSettings() {
  const rows = appState.settings.map((setting) => navButton("setting-row", `data-setting="${escapeHtml(setting)}" aria-label="${escapeHtml(setting)}"`, escapeHtml(setting))).join("");
  return shell("settings", "Settings", `<section class="setting-list">${rows}</section>`);
}

function renderCharge() {
  return shell("charge", "Charge", `${navButton("wide-hit", "data-target=\"home\" aria-label=\"Return home\"", "")}`);
}

const renderers = { home: renderHome, loading: renderLoading, library: renderLibrary, now: renderNow, memos: renderMemos, settings: renderSettings, charge: renderCharge };


window.SHAeRFirmware.mount({
  themeId: "shaer_indian_print", state: appState, screens, storageKey: "shaer-core-indian-print",
  aliases: { home: "home", boot: "loading", loading: "loading", library: "library", album: "library", "now-playing": "now", recordings: "memos", settings: "settings", charging: "charge", about: "charge" },
  localToCanonical: { home: "home", loading: "loading", library: "library", now: "now-playing", memos: "recordings", settings: "settings", charge: "charging" },
  render: (id) => renderers[id]()
});
