const appState = {
  active: "home",
  selectedIndex: 0,
  history: [],
  battery: null,
  loadingProgress: 42,
  memoElapsed: 0,
  playing: false,
  playProgress: 35,
  currentTrackIndex: 0,
  musicStatus: "loading",
  songs: [],
  playlists: [],
  settings: ["About", "Appearance", "Playback", "Audio", "Connectivity", "Power", "Date & Time", "Sync", "Advanced"]
};

const screens = [
  ["home", "Home"],
  ["loading", "Loading"],
  ["library", "My Music"],
  ["now", "Winamp"],
  ["memos", "Voice Recorder"],
  ["vlc", "VLC"],
  ["settings", "Settings"],
  ["boot", "Boot"]
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
  const hh = String(date.getHours()).padStart(2, "0");
  const mm = String(date.getMinutes()).padStart(2, "0");
  return `${hh}:${mm} pm`;
}

function dateText(date = new Date()) {
  return date.toLocaleDateString("en-GB", { day: "2-digit", month: "long", year: "2-digit" }).toLowerCase();
}

function shell(id, title, body) {
  const boot = id === "boot";
  return `<article class="screen ${boot ? "boot-screen" : ""}" data-screen="${id}">
    ${boot ? "" : `<div class="xp-desktop-date">${dateText()}</div>`}
    ${body}
    ${boot ? "" : `<div class="taskbar"><span class="taskbar-brand">SHAeR</span><span>${timeText()}</span></div>`}
  </article>`;
}

function titlebar(title) {
  return `<div class="titlebar"><span>${title}</span><i></i><b></b><em></em></div>`;
}

function navButton(className, attrs = "", content = "") {
  return `<button class="${className}" ${attrs} data-nav type="button">${content}</button>`;
}

function currentTrack() {
  return appState.songs[appState.currentTrackIndex] || null;
}

function musicMessage() {
  return window.SHAeRMusic ? window.SHAeRMusic.viewMessage(appState.musicStatus) : "Loading music";
}

function menuItem(iconClass, label, attrs) {
  return navButton("xp-menu-item", attrs, `<i class="${iconClass}"></i><span>${escapeHtml(label)}</span><b>▶</b>`);
}

function renderHome() {
  return shell("home", "Home", `
    <section class="start-menu">
      <h1>Menu Bar</h1>
      ${menuItem("ico-music", "Local music", "data-target=\"library\" aria-label=\"Local music\"")}
      ${menuItem("ico-spotify", "Spotify connect", "data-target=\"loading\" aria-label=\"Spotify connect\"")}
      ${menuItem("ico-memo", "Memos", "data-target=\"memos\" aria-label=\"Memos\"")}
      ${menuItem("ico-settings", "Settings", "data-target=\"settings\" aria-label=\"Settings\"")}
    </section>
  `);
}

function renderLoading() {
  return shell("loading", "Loading", `
    <section class="dialog loading-dialog">
      ${titlebar("Loading")}
      <div class="dialog-body">
        <p>Loading...</p>
        <div class="xp-progress"><i style="--p:${appState.loadingProgress}%"></i></div>
        <div class="dialog-buttons">
          <button type="button" disabled>Done</button>
          ${navButton("xp-button", "data-target=\"library\" aria-label=\"Cancel loading\"", "Cancel")}
        </div>
      </div>
    </section>
  `);
}

function renderLibrary() {
  const tracks = appState.songs.slice(0, 4).map((track, index) => menuItem("ico-music", track.title, `data-song="${index}" aria-label="Play ${escapeHtml(track.title)}"`));
  const playlists = appState.playlists.slice(0, 3).map((playlist, index) => menuItem("ico-folder", playlist.title, `data-playlist="${index}" aria-label="Open ${escapeHtml(playlist.title)}"`));
  const status = appState.songs.length ? [] : [menuItem("ico-music", musicMessage(), "disabled aria-disabled=\"true\"")];
  const rows = [...status, ...tracks, ...playlists, menuItem("ico-memo", "Recordings", `data-action="recording-library" aria-label="Open recording archive"`)].join("");
  return shell("library", "My Music", `
    <section class="xp-menu music-menu">
      <h1>My Music</h1>
      ${rows}
    </section>
  `);
}

function renderNow() {
  const track = currentTrack();
  const title = track ? track.title : "Nothing playing";
  const artist = track ? track.artist : "Choose music";
  const album = track ? track.album : "Artwork unavailable";
  return shell("now", "Winamp", `
    <section class="winamp">
      <div class="album-art" data-shaer-cover><span data-shaer-album>${escapeHtml(album)}</span></div>
      <div class="knob k1"></div><div class="knob k2"></div>
      <div class="sliders"><i></i><i></i><i></i></div>
      <div class="meta">
        <label>Artist:</label><b data-shaer-artist>${escapeHtml(artist)}</b>
        <label>Title:</label><b data-shaer-title>${escapeHtml(title)}</b>
      </div>
      <div class="xp-song-progress"><i data-shaer-progress style="--p:${appState.playProgress}%"></i></div>
      <div class="winamp-controls">
        ${navButton("media-btn", "data-action=\"previous\" aria-label=\"Previous\"", "◀")}
        ${navButton("media-btn", "data-action=\"toggle-play\" aria-label=\"Play pause\"", appState.playing ? "▮▮" : "▶")}
        ${navButton("media-btn", "data-action=\"next\" aria-label=\"Next\"", "▶")}
        ${navButton("media-btn", "data-target=\"vlc\" aria-label=\"Open VLC\"", "□")}
      </div>
    </section>
  `);
}

function renderVlc() {
  return shell("vlc", "VLC", `
    <section class="vlc-window">
      ${titlebar("VLC media player")}
      <div class="vlc-menu">File View Play Audio Video Help</div>
      <div class="vlc-canvas"><div class="cone"></div></div>
      <div class="vlc-footer">
        ${navButton("vlc-round", "data-action=\"previous\" aria-label=\"Previous\"", "◀")}
        ${navButton("vlc-round", "data-action=\"toggle-play\" aria-label=\"Play pause\"", appState.playing ? "▮▮" : "▶")}
        ${navButton("vlc-round", "data-action=\"next\" aria-label=\"Next\"", "▶")}
      </div>
    </section>
  `);
}

function renderMemos() {
  const bars = [11, 20, 29, 17, 34, 25, 14, 31, 21, 27, 16, 23]
    .map((height) => `<i style="--h:${height}px"></i>`).join("");
  return shell("memos", "Voice Recorder", `
    <section class="xp-recorder">
      ${titlebar("Sound Recorder")}
      <div class="xp-recorder-body">
        <div class="xp-waveform">${bars}</div>
        <strong>00:00</strong>
        <div class="xp-recorder-controls">
          ${navButton("xp-recorder-button record", "data-action=\"toggle-memo\" aria-label=\"Record or pause\"", "REC")}
          ${navButton("xp-recorder-button", "data-action=\"save-memo\" aria-label=\"Save recording\"", "SAVE")}
          ${navButton("xp-recorder-button", "data-action=\"delete-memo\" aria-label=\"Discard recording\"", "DEL")}
          ${navButton("xp-recorder-button", "data-action=\"recording-library\" aria-label=\"Open recording archive\"", "LIB")}
        </div>
      </div>
    </section>
  `);
}

function renderSettings() {
  const rows = appState.settings.map((row, index) => menuItem(index === 0 ? "ico-about" : index === 1 ? "ico-audio" : index === 2 ? "ico-play" : "ico-settings", row, `data-setting="${escapeHtml(row)}" aria-label="${escapeHtml(row)}"`)).join("");
  return shell("settings", "Settings", `
    <section class="xp-menu settings-menu">
      <h1>Settings</h1>
      ${rows}
    </section>
  `);
}

function renderBoot() {
  return shell("boot", "Boot", `
    <section class="boot-card">
      <div class="windows-logo"><i></i><i></i><i></i><i></i></div>
      <h1>SHAeR</h1>
      <p>powered by aadi-vasi</p>
      <div class="boot-progress"><i></i></div>
      ${navButton("boot-hit", "data-target=\"home\" aria-label=\"Return home\"", "")}
    </section>
  `);
}

const renderers = {
  home: renderHome,
  loading: renderLoading,
  library: renderLibrary,
  now: renderNow,
  memos: renderMemos,
  vlc: renderVlc,
  settings: renderSettings,
  boot: renderBoot
};


window.SHAeRFirmware.mount({
  themeId: "shaer_windows_xp", state: appState, screens, storageKey: "shaer-core-windows-xp",
  aliases: { home: "home", boot: "boot", loading: "loading", library: "library", album: "library", "now-playing": "now", recordings: "memos", settings: "settings", charging: "boot", about: "boot" },
  localToCanonical: { home: "home", boot: "boot", loading: "loading", library: "library", now: "now-playing", vlc: "now-playing", memos: "recordings", settings: "settings" },
  render: (id) => renderers[id]()
});
