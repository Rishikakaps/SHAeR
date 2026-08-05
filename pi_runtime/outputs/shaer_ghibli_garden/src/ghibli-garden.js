const appState = {
  active: "home",
  selectedIndex: 0,
  history: [],
  battery: null,
  loadingProgress: 44,
  playing: false,
  playProgress: 38,
  currentTrackIndex: 0,
  recording: false,
  memoElapsed: 24,
  musicStatus: "loading",
  songs: [],
  playlists: [],
  settings: ["About", "Appearance", "Playback", "Audio", "Connectivity", "Power", "Date & Time", "Sync", "Advanced"]
};

const screens = [
  ["home", "Menu"],
  ["loading", "Loading"],
  ["library", "Library"],
  ["now", "Playing"],
  ["memos", "Memos"],
  ["settings", "Settings"],
  ["pond", "Pond"]
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
  return date.toLocaleDateString("en-GB", { day: "2-digit", month: "long", year: "2-digit" });
}

function shell(id, title, body, variant = "") {
  return `<article class="screen ${variant}" data-screen="${id}">
    <div class="garden-bg"></div>
    <div class="topbar"><button class="back-chip" data-action="back" type="button" aria-label="Back">☰</button><span>${escapeHtml(title)}</span><b>${dateText()}</b></div>
    ${body}
    <div class="dock"><span>✦ SHAeR</span><b>${timeText()}</b></div>
  </article>`;
}

function navButton(className, attrs = "", content = "") {
  return `<button class="${className}" ${attrs} data-nav type="button">${content}</button>`;
}

function currentTrack() {
  return appState.songs[appState.currentTrackIndex] || null;
}

function musicMessage() {
  return window.SHAeRMusic ? window.SHAeRMusic.viewMessage(appState.musicStatus).toLowerCase() : "loading music";
}

function renderHome() {
  const items = [
    ["Local Music", "library", "♪"],
    ["Spotify Connect", "loading", "◌"],
    ["Memos", "memos", "▣"],
    ["Settings", "settings", "⚙"]
  ].map(([label, target, icon]) => navButton("glass-row", `data-target="${target}" aria-label="${label}"`, `<i>${icon}</i><span>${label}</span><b>›</b>`)).join("");
  return shell("home", "Menu", `
    <div class="vertical-script">千与千尋</div>
    <section class="liquid-card home-card">
      <p>06</p>
      <h1>Garden OS</h1>
      <div class="menu-list">${items}</div>
    </section>
  `, "scene-home");
}

function renderLoading() {
  return shell("loading", "Loading", `
    <section class="liquid-card loading-card">
      <div class="spinner"><i></i></div>
      <strong>mhm mhm</strong>
      <span>D drive loading</span>
      <div class="glass-progress"><i style="--p:${appState.loadingProgress}%"></i></div>
    </section>
    ${navButton("wide-hit", "data-target=\"library\" aria-label=\"Continue\"", "")}
  `, "scene-fuji");
}

function renderLibrary() {
  const playlistRows = appState.playlists.slice(0, 4).map((playlist, index) => navButton("library-row", `data-playlist="${index}" aria-label="Open ${escapeHtml(playlist.title)}"`, `
    <i></i><span>${escapeHtml(playlist.title)}<small>${playlist.count} tracks</small></span><b>›</b>
  `)).join("");
  const rows = appState.songs.length ? appState.songs.map((song, index) => navButton("library-row", `data-song="${index}" aria-label="Play ${escapeHtml(song.title)}"`, `
    <i></i><span>${escapeHtml(song.title)}<small>${escapeHtml(song.duration)}</small></span><b>›</b>
  `)).join("") : `<div class="library-row music-state-row"><i></i><span>${escapeHtml(musicMessage())}<small>${appState.musicStatus === "unauthenticated" ? "Connect Spotify or add local music" : "Library is empty"}</small></span></div>`;
  return shell("library", "Library", `
    <section class="liquid-card library-card">
      ${playlistRows}
      ${rows}
      ${navButton("library-row", "data-action=\"recording-library\" aria-label=\"Open recording archive\"", "<i></i><span>Recordings<small>Personal archive</small></span><b>›</b>")}
    </section>
  `, "scene-street");
}

function renderNow() {
  const track = currentTrack();
  const title = track ? track.title : "Nothing playing";
  const artist = track ? track.artist : "Choose music";
  return shell("now", "Playing", `
    <section class="liquid-card player-card">
      <div class="album-water" data-shaer-cover></div>
      <div class="track-copy"><strong data-shaer-title>${escapeHtml(title)}</strong><span data-shaer-artist>${escapeHtml(artist)}</span></div>
      <div class="glass-progress long"><i data-shaer-progress style="--p:${appState.playProgress}%"></i></div>
      <div class="player-buttons">
        ${navButton("round-control", "data-action=\"previous\" aria-label=\"Previous\"", "‹‹")}
        ${navButton("round-control play", "data-action=\"toggle-play\" aria-label=\"Play pause\"", appState.playing ? "Ⅱ" : "▶")}
        ${navButton("round-control", "data-action=\"next\" aria-label=\"Next\"", "››")}
      </div>
    </section>
  `, "scene-sakura");
}

function renderMemos() {
  const bars = [18, 34, 56, 31, 70, 44, 22, 64, 38, 53, 28, 46, 62, 35, 20].map((height, index) => `<i style="--h:${height}px;--i:${index}"></i>`).join("");
  return shell("memos", "Memos", `
    <section class="liquid-card memo-card">
      <button class="mic" data-action="toggle-memo" data-nav type="button" aria-label="${appState.recording ? "Stop recording" : "Record memo"}"></button>
      <h1>Voice Memo</h1>
      <span>Capture your thoughts</span>
      <div class="wave">${bars}</div>
      <strong>${String(Math.floor(appState.memoElapsed / 60)).padStart(2, "0")}:${String(appState.memoElapsed % 60).padStart(2, "0")}</strong>
      <div class="memo-actions">
        ${navButton("memo-btn record", "data-action=\"toggle-memo\" aria-label=\"Record\"", "Record")}
        ${navButton("memo-btn", "data-action=\"toggle-play\" aria-label=\"Pause\"", "Pause")}
        ${navButton("memo-btn", "data-action=\"save-memo\" aria-label=\"Finish and save\"", "Finish")}
      </div>
    </section>
  `, "scene-lake");
}

function renderSettings() {
  const rows = appState.settings.map((setting) => navButton("setting-row", `data-setting="${escapeHtml(setting)}" aria-label="${escapeHtml(setting)}"`, `<i></i><span>${escapeHtml(setting)}</span><b>›</b>`)).join("");
  return shell("settings", "Settings", `
    <section class="liquid-card settings-card">${rows}</section>
  `, "scene-forest");
}

function renderPond() {
  return shell("pond", "Pond", `
    ${navButton("wide-hit", "data-target=\"home\" aria-label=\"Return home\"", "")}
  `, "scene-pond");
}

const renderers = { home: renderHome, loading: renderLoading, library: renderLibrary, now: renderNow, memos: renderMemos, settings: renderSettings, pond: renderPond };


window.SHAeRFirmware.mount({
  themeId: "shaer_ghibli_garden", state: appState, screens, storageKey: "shaer-core-ghibli-garden",
  aliases: { home: "home", boot: "loading", loading: "loading", library: "library", album: "library", "now-playing": "now", recordings: "memos", settings: "settings", charging: "pond", about: "pond" },
  localToCanonical: { home: "home", loading: "loading", library: "library", now: "now-playing", memos: "recordings", settings: "settings", pond: "charging" },
  render: (id) => renderers[id]()
});
