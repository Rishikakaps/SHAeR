const appState = {
  active: "home",
  battery: null,
  loadingProgress: 68,
  playing: false,
  playProgress: 42,
  currentTrackIndex: 0,
  selectedIndex: 0,
  history: [],
  recording: false,
  memoElapsed: 0,
  savedMemos: [],
  musicStatus: "loading",
  songs: [],
  playlists: [],
  settings: {
    top: ["APPEARANCE", "PLAYBACK"],
    middle: ["AUDIO", "CONNECTIVITY", "POWER"],
    bottom: ["DATE & TIME", "SYNC", "ADVANCED"]
  }
};

const screens = [
  ["home", "Home"],
  ["loading", "Loading"],
  ["library", "Library"],
  ["liked-list", "Liked List"],
  ["now-playing", "Now Playing"],
  ["memos", "Memos"],
  ["settings", "Settings"],
  ["chai-ticket", "Chai Ticket"]
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

function formatStamp(date = new Date()) {
  const months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"];
  const day = String(date.getDate()).padStart(2, "0");
  const month = months[date.getMonth()];
  const year = String(date.getFullYear()).slice(-2);
  const hours = String(date.getHours()).padStart(2, "0");
  const minutes = String(date.getMinutes()).padStart(2, "0");
  return `${day}${month}${year}/${hours}:${minutes}`;
}

function footer() {
  const battery = Number.isFinite(appState.battery) ? `${appState.battery} %` : "-- %";
  return `<div class="footer"><span>${formatStamp()}</span><b>${battery}</b></div>`;
}

function tab(label, wide = false) {
  return `<div class="tab${wide ? " wide" : ""}">${label}</div>`;
}

function button(className, attrs = "") {
  return `<button class="hit ${className}" ${attrs} type="button"></button>`;
}

function screenShell(id, label, body, wide = false) {
  return `<article class="screen ${id === "chai-ticket" ? "charge-screen" : ""}" data-screen="${id}">
    ${tab(label, wide)}
    ${body}
    ${id === "chai-ticket" ? "" : footer()}
  </article>`;
}

function currentTrack() {
  return appState.songs[appState.currentTrackIndex] || null;
}

function musicMessage() {
  return window.SHAeRMusic ? window.SHAeRMusic.viewMessage(appState.musicStatus) : "LOADING MUSIC";
}

function renderHome() {
  const body = `
    <div class="menu-card">
      <div class="menu-title">MENU BAR<span>SELECT YOUR CHOICE</span></div>
      <i class="menu-line one"></i><i class="menu-line two"></i><i class="menu-line three"></i><i class="menu-line four"></i><i class="menu-vline"></i>
      <div class="connect">SPOTIFY<span>CONNECT</span></div>
      <div class="local">LOCAL<br>FILES</div>
      <div class="settings-link">SETTINGS</div>
      <div class="cross"></div>
      <div class="mic-icon"></div>
      <div class="memo-label">MEMO</div>
      <div class="brand"><strong>SHAER</strong><small>POWERED BY आदि-VASIOS</small></div>
    </div>
    ${button("home-spotify", "data-target=\"loading\" data-nav aria-label=\"Spotify connect\"")}
    ${button("home-local", "data-target=\"library\" data-nav aria-label=\"Local files\"")}
    ${button("home-settings", "data-target=\"settings\" data-nav aria-label=\"Settings\"")}
    ${button("home-memos", "data-target=\"memos\" data-nav aria-label=\"Voice memo recorder\"")}
  `;
  return screenShell("home", "Home", body, false);
}

function renderLoading() {
  const body = `
    <div class="dd-frame"><img class="dd-image" src="assets/doordarshan.png" alt="Doordarshan loading mark"></div>
    <div class="loading-mark">SHAER</div>
    <div class="rail-left"></div>
    <div class="loading-box">
      <i class="corner tl"></i><i class="corner tr"></i><i class="corner bl"></i><i class="corner br"></i>
      <div>MHM MHM<br>D DRIVE UPDATING</div>
    </div>
    <div class="loading-bottom"><strong>LOADING</strong><span class="progress-line" style="transform:scaleX(${appState.loadingProgress / 100})"></span></div>
    ${button("full-hit", "data-target=\"library\" data-nav aria-label=\"Continue\"")}
  `;
  return screenShell("loading", "Loading", body, true);
}

function renderLibrary() {
  const rows = appState.songs.length ? appState.songs.slice(0, 3).map((song, index) => `
    <button class="hit song-row r${index + 1}" data-song="${index}" data-nav type="button" aria-label="Play ${escapeHtml(song.title)}">
      <span>${escapeHtml(song.title)}</span><b>${escapeHtml(song.duration)}</b>
    </button>
  `).join("") : `<div class="hit song-row r1 music-state-row">${escapeHtml(musicMessage())}</div>`;
  const previewRows = appState.songs.slice(0, 4).map((song, index) => `
    <button class="playlist-preview-row" data-song="${index}" data-nav type="button" aria-label="Play ${escapeHtml(song.title)} from playlist preview">
      <span>${escapeHtml(song.title)}</span><b>${escapeHtml(song.duration)}</b>
    </button>
  `).join("");
  const body = `
    <div class="grid-card">
      <button class="hit liked-head" data-target="liked-list" data-nav type="button" aria-label="Saved tracks">
        <span class="heart">♡</span><span class="liked-title">SAVED TRACKS<small>${escapeHtml(appState.songs[0] ? appState.songs[0].title : musicMessage())}</small></span>
      </button>
      ${rows}
      ${appState.playlists[0] ? `<button class="hit playlist-block" data-playlist="0" data-nav type="button" aria-label="Open ${escapeHtml(appState.playlists[0].title)}"><span>${escapeHtml(appState.playlists[0].title)}</span><span class="playlist-count">( ${appState.playlists[0].count} )</span></button>` : ""}
      <div class="playlist-preview">${previewRows}</div>
      <button class="recordings-link" data-action="recording-library" data-nav type="button" aria-label="Open recording archive">RECORDINGS</button>
      ${appState.playlists[1] ? `<button class="hit playlist-two" data-playlist="1" data-nav type="button" aria-label="Open ${escapeHtml(appState.playlists[1].title)}">${escapeHtml(appState.playlists[1].title)}</button>` : ""}
    </div>
  `;
  return screenShell("library", "Library", body, true);
}

function renderLikedList() {
  const topSongs = appState.songs.length ? appState.songs.slice(0, 3).map((song, index) => `
    <button class="list-row" data-song="${index}" data-nav type="button" aria-label="Play ${escapeHtml(song.title)}">
      <span>${escapeHtml(song.title)} - ${escapeHtml(song.artist)}</span><b>${escapeHtml(song.duration)}</b>
    </button>
  `).join("") : `<div class="list-row music-state-row"><span>${escapeHtml(musicMessage())}</span></div>`;
  const playlistSongs = appState.songs.map((song, index) => `
    <button class="list-row" data-song="${index}" data-nav type="button" aria-label="Play ${escapeHtml(song.title)}">
      <span>${escapeHtml(song.title)} - ${escapeHtml(song.artist)}</span>
    </button>
  `).join("");
  const times = appState.songs.map((song) => `<span>${escapeHtml(song.duration)}</span>`).join("");
  const body = `
    <div class="list-ticket">
      <div class="list-head">SAVED TRACKS</div>
      <div class="music-note">♫</div>
      <div class="top-songs">${topSongs}</div>
      <div class="side-times">${times}</div>
      <div class="playlist-list"><h3>${escapeHtml(appState.selectedPlaylist ? appState.selectedPlaylist.title : "YOUR MUSIC")}</h3>${playlistSongs}</div>
      <div class="scale-mark scale-one">1X</div><div class="scale-mark scale-two">2X</div><div class="scale-mark scale-three">2X</div>
      <div class="route-name" aria-hidden="true">SHAER</div>
    </div>
  `;
  return screenShell("liked-list", "Library", body, true);
}

function renderNowPlaying() {
  const track = currentTrack();
  const title = track ? track.title : "NOTHING PLAYING";
  const artist = track ? track.artist : "CHOOSE MUSIC";
  const album = track ? track.album : "";
  const body = `
    <div class="ticket-stub">
      <img class="ticket-border-img" src="assets/now-playing-ticket-border-alpha.png" alt="">
      <div class="song-title"><span data-shaer-title>${escapeHtml(title)}</span><br><span data-shaer-artist>${escapeHtml(artist)}</span><br><small data-shaer-album>${escapeHtml(album)}</small></div>
      <div class="ticket-progress" data-shaer-progress style="--play-progress:${Math.max(0, Math.min(100, appState.playProgress))}%"></div>
      <div class="ticket-controls">
        <button data-action="shuffle" data-nav type="button" aria-label="Shuffle">⌘</button>
        <button data-action="previous" data-nav type="button" aria-label="Previous">◀</button>
        <button data-action="toggle-play" data-nav type="button" aria-label="${appState.playing ? "Pause" : "Play"}">${appState.playing ? "▣" : "▷"}</button>
        <button data-action="next" data-nav type="button" aria-label="Next">▶</button>
        <button data-action="repeat" data-nav type="button" aria-label="Repeat">↻</button>
      </div>
    </div>
  `;
  return screenShell("now-playing", "Playing", body, true);
}

function renderMemos() {
  const bars = [16, 32, 52, 29, 62, 40, 18, 55, 73, 48, 25, 64, 37, 21, 47, 69, 34, 18, 39, 56]
    .map((height, index) => `<i style="--h:${height}px;--i:${index}"></i>`).join("");
  const body = `
    <div class="wave-panel">
      <div class="record-title">VOICE MEMO</div>
      <div class="waveform" aria-label="Recording waveform">${bars}</div>
    </div>
    <button class="record-dot${appState.recording ? " recording" : ""}" data-action="toggle-memo" data-nav type="button" aria-label="${appState.recording ? "Stop recording" : "Record memo"}"></button>
    <div class="memo-controls">
      <button data-action="delete-memo" data-nav type="button">DEL</button>
      <button data-action="save-memo" data-nav type="button">SAVE</button>
      <button data-action="save-memo" data-nav type="button">DONE</button>
    </div>
  `;
  return screenShell("memos", "Memos", body, true);
}

function renderSettings() {
  const top = appState.settings.top.map((item) => `<button class="setting-choice" data-setting="${item}" data-nav type="button">${escapeHtml(item)}</button>`).join("");
  const middle = appState.settings.middle.map((item) => `<button class="setting-choice" data-setting="${item}" data-nav type="button">${escapeHtml(item)}</button>`).join("");
  const bottom = appState.settings.bottom.map((item) => `<button class="setting-choice" data-setting="${item}" data-nav type="button">${escapeHtml(item)}</button>`).join("");
  const body = `
    <div class="settings-card">
      <button class="about" data-setting="ABOUT" data-nav type="button">ABOUT</button>
      <div class="settings-group top">${top}</div>
      <div class="settings-group mid">${middle}</div>
      <div class="settings-group bottom">${bottom}</div>
      <div class="scale-mark scale-one">1X</div><div class="scale-mark scale-two">2X</div><div class="scale-mark scale-three">2X</div>
      <div class="route-name" aria-hidden="true">SHAER</div>
    </div>
  `;
  return screenShell("settings", "Setting", body, true);
}

function renderChai() {
  const body = `
    <div class="chai-pencil">
      <div class="steam"><i></i><i></i><i></i></div>
      <img src="assets/chai-pencil-tight.png" alt="Chai cup pencil illustration">
    </div>
    ${button("full-hit", "data-target=\"home\" data-nav aria-label=\"Return home\"")}
  `;
  return `<article class="screen charge-screen" data-screen="chai-ticket">${body}</article>`;
}

const renderers = {
  "home": renderHome,
  "loading": renderLoading,
  "library": renderLibrary,
  "liked-list": renderLikedList,
  "now-playing": renderNowPlaying,
  "memos": renderMemos,
  "settings": renderSettings,
  "chai-ticket": renderChai
};

function renderScreen(id) {
  return renderers[id]();
}


window.SHAeRFirmware.mount({
  themeId: "shaer_bombay_ticket", state: appState, screens, storageKey: "shaer-core-bombay-ticket",
  aliases: { home: "home", boot: "loading", loading: "loading", library: "library", album: "liked-list", "now-playing": "now-playing", recordings: "memos", settings: "settings", charging: "chai-ticket", about: "chai-ticket" },
  localToCanonical: { home: "home", loading: "loading", library: "library", "liked-list": "album", "now-playing": "now-playing", memos: "recordings", settings: "settings", "chai-ticket": "charging" },
  render: (id) => renderScreen(id)
});
