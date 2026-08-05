const appState = {
  active: "home",
  selectedIndex: 0,
  history: [],
  battery: null,
  loadingProgress: 68,
  playing: true,
  playProgress: 42,
  currentTrackIndex: 0,
  recording: false,
  memoElapsed: 0,
  musicStatus: "loading",
  songs: [],
  playlists: [],
  settings: ["ABOUT", "APPEARANCE", "PLAYBACK", "AUDIO", "CONNECTIVITY", "POWER", "DATE & TIME", "SYNC", "ADVANCED"]
};

const screens = [
  ["home", "Home"],
  ["library", "Library"],
  ["loading", "Loading"],
  ["liked", "Liked List"],
  ["now", "Now Playing"],
  ["memos", "Voice Memos"],
  ["settings", "Settings"],
  ["poster", "Poster"]
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

function nowTime() {
  const date = new Date();
  const hour = date.getHours();
  const hh = String((hour % 12) || 12).padStart(2, "0");
  const mm = String(date.getMinutes()).padStart(2, "0");
  return `${hh}:${mm} ${hour >= 12 ? "PM" : "AM"}`;
}

function dateText(date = new Date()) {
  return date.toLocaleDateString("en-GB", { day: "2-digit", month: "short", year: "numeric" }).toUpperCase();
}

function footer() {
  const battery = Number.isFinite(appState.battery) ? `${appState.battery}%` : "--%";
  return `<div class="jp-footer"><span>${dateText()}</span><b>${battery}</b></div>`;
}

function shell(id, label, body) {
  return `<article class="screen" data-screen="${id}">
    <div class="jp-top"><b>MP-002</b><span>${label}</span><em>${nowTime()}</em></div>
    <div class="jp-rail left"></div><div class="jp-rail right"></div>
    ${body}
    ${footer()}
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

function renderLibrary() {
  const playlistItems = appState.playlists.slice(0, 3).map((playlist, index) => [
    playlist.title,
    `${playlist.count} TRACKS`,
    "playlists",
    `data-playlist="${index}" aria-label="Open ${escapeHtml(playlist.title)}"`
  ]);
  const items = [
    [appState.songs.length ? "SAVED TRACKS" : musicMessage(), `${appState.songs.length} AVAILABLE`, "home"],
    ...playlistItems,
    ["RECORDINGS", "PERSONAL ARCHIVE", "genres", "data-action=\"recording-library\""]
  ].map(([title, sub, kind, override], index) => `
    ${navButton(`jp-card card-${index + 1}`, override || `data-target="liked"`, `
      <i class="folder-icon ${kind}"></i><strong>${escapeHtml(title)}</strong><small>${escapeHtml(sub)}</small>
    `)}
  `).join("");
  return shell("library", "LIBRARY", `
    <section class="library-head">
      <h1>LIBRARY</h1><p>MUSIC INDEX</p><mark>${appState.songs.length}</mark>
    </section>
    <div class="status-strip">LOCAL FILES <span>CONNECTED</span></div>
    <div class="jp-card-grid">${items}</div>
    <div class="micro-panel"><span>REC</span><span>GAIN</span><span>GENRE</span></div>
    ${navButton("corner-action", "data-target=\"loading\" aria-label=\"Load drive\"", "D DRIVE")}
  `);
}

function renderHome() {
  const items = [
    ["LOCAL MUSIC", "data-target=\"library\"", "01"],
    ["SPOTIFY CONNECT", "data-target=\"loading\"", "02"],
    ["SETTINGS", "data-target=\"settings\"", "03"],
    ["VOICE MEMOS", "data-target=\"memos\"", "04"]
  ].map(([label, attrs, number]) => `
    ${navButton("home-nav-row", `${attrs} aria-label="${escapeHtml(label)}"`, `<span>${number}</span><strong>${escapeHtml(label)}</strong><em>▶</em>`)}
  `).join("");
  return shell("home", "MENU", `
    <section class="jp-home-card">
      <p>MP-002 / START</p>
      <h1>MENU BAR</h1>
      <div class="home-warning">LOCAL MODE / NO SIGNAL</div>
      <div class="home-nav-list">${items}</div>
      <div class="home-sticker">SHAeR<br><span>OS</span></div>
    </section>
  `);
}

function renderLoading() {
  return shell("loading", "LOADING", `
    <section class="loading-title"><h1>LOADING...</h1><p>SHAER DISK UNIT</p></section>
    <div class="load-poster">
      <div class="pink-sheets"></div>
      <div class="noise-cloud"></div>
      <div class="cassette-mini">LOADING</div>
    </div>
    <div class="load-dial"><span>C</span><span>A</span><span>G</span></div>
    <div class="drive-progress"><b>D DRIVE LOADING</b><i style="--p:${appState.loadingProgress}%"></i><em>${appState.loadingProgress}%</em></div>
    ${navButton("wide-hit", "data-target=\"liked\" aria-label=\"Continue to list\"", "")}
  `);
}

function renderLiked() {
  const rows = appState.songs.length ? appState.songs.map((song, index) => `
    ${navButton(`punk-row ${index === appState.currentTrackIndex ? "active" : ""}`, `data-song="${index}" aria-label="Play ${escapeHtml(song.title)}"`, `
      <span>${String(index + 1).padStart(2, "0")}</span><strong>${escapeHtml(song.title)}</strong><em>${escapeHtml(song.duration)}</em>
    `)}
  `).join("") : `<div class="punk-row music-state-row"><span>--</span><strong>${escapeHtml(musicMessage())}</strong><em>--:--</em></div>`;
  return shell("liked", "LIBRARY", `
    <section class="list-panel">
      <p>J-ラジオ</p>
      <h1>LIBRARY</h1>
      <div class="pink-select">01&nbsp;&nbsp; ${escapeHtml(appState.selectedPlaylist ? appState.selectedPlaylist.title : "SAVED TRACKS")} <span>♡</span></div>
      <div class="punk-list">${rows}</div>
      <div class="pager">1 : 1 <i></i></div>
    </section>
  `);
}

function renderNow() {
  const track = currentTrack();
  const title = track ? track.title : "NOTHING PLAYING";
  const artist = track ? track.artist : "CHOOSE MUSIC";
  const duration = track ? track.duration : "0:00";
  const album = track ? track.album : "SHAeR";
  return shell("now", "NOW PLAYING", `
    <section class="track-title">
      <h1>${track ? `TRACK ${String(appState.currentTrackIndex + 1).padStart(2, "0")}` : "NOW PLAYING"}</h1><p>${escapeHtml(album || "SHAeR")}</p>
    </section>
    <div class="manga-cover" data-shaer-cover><div class="hair"></div><b>キャル</b></div>
    <div class="track-meters"><i></i><i></i><i></i><i></i><i></i></div>
    <div class="track-meta"><strong data-shaer-title>${escapeHtml(title)}</strong><span data-shaer-artist>${escapeHtml(artist)}</span><em data-shaer-duration>${escapeHtml(duration)}</em></div>
    <div class="jp-progress"><i data-shaer-progress style="--p:${appState.playProgress}%"></i></div>
    <div class="jp-controls">
      ${navButton("ctrl", "data-action=\"previous\" aria-label=\"Previous\"", "◀")}
      ${navButton("ctrl big", "data-action=\"toggle-play\" aria-label=\"Play pause\"", appState.playing ? "▮▮" : "▶")}
      ${navButton("ctrl", "data-action=\"next\" aria-label=\"Next\"", "▶")}
    </div>
    ${navButton("warning-tag", "data-target=\"memos\" aria-label=\"Open voice memos\"", "VOICE MEMO")}
  `);
}

function renderMemos() {
  const bars = [18, 24, 36, 28, 44, 31, 40, 21, 35, 49, 26, 38]
    .map((height, index) => `<i style="--h:${height}px;--i:${index}"></i>`).join("");
  return shell("memos", "VOICE MEMOS", `
    <section class="memo-label-block"><h1>VOICE MEMOS</h1><p>ボイスメモ</p></section>
    <div class="cassette">
      <div class="tape-title">VOICE MEMO</div><div class="reel a"></div><div class="reel b"></div><div class="tape-window"></div>
    </div>
    <div class="memo-wave">${bars}</div>
    <p class="record-copy">PRESS ○ TO RECORD</p>
    ${navButton(`record-button ${appState.recording ? "recording" : ""}`, "data-action=\"toggle-memo\" aria-label=\"Record memo\"", "<span></span>")}
    <div class="memo-buttons">
      ${navButton("tiny-button", "data-action=\"delete-memo\" aria-label=\"Delete memo\"", "DEL")}
      ${navButton("tiny-button", "data-action=\"save-memo\" aria-label=\"Save memo\"", "SAVE")}
      ${navButton("tiny-button", "data-action=\"save-memo\" aria-label=\"Finish and save\"", "DONE")}
    </div>
  `);
}

function renderSettings() {
  const rows = appState.settings.map((setting, index) => `
    ${navButton("setting-row", `data-setting="${escapeHtml(setting)}" aria-label="${escapeHtml(setting)}"`, `
      <span>◎</span><strong>${escapeHtml(setting)}</strong>
    `)}
  `).join("");
  return shell("settings", "SETTINGS", `
    <section class="settings-panel"><p>SETTINGS</p><div>${rows}</div></section>
  `);
}

function renderPoster() {
  return shell("poster", "POSTER", `
    <section class="night-poster">
      <h1>YORU NO TSUISEKI</h1><p>Night Chasing</p>
      <div class="car-scene"><i></i><b></b><span></span></div>
      <div class="poster-tags"><span>FM-002</span><span>NOISE</span><span>SHAER</span></div>
    </section>
    ${navButton("poster-hit", "data-target=\"library\" aria-label=\"Return to library\"", "")}
  `);
}

const renderers = {
  home: renderHome,
  library: renderLibrary,
  loading: renderLoading,
  liked: renderLiked,
  now: renderNow,
  memos: renderMemos,
  settings: renderSettings,
  poster: renderPoster
};


window.SHAeRFirmware.mount({
  themeId: "shaer_japanese_punk", state: appState, screens, storageKey: "shaer-core-japanese-punk",
  aliases: { home: "home", boot: "loading", loading: "loading", library: "library", album: "liked", "now-playing": "now", recordings: "memos", settings: "settings", charging: "poster", about: "poster" },
  localToCanonical: { home: "home", loading: "loading", library: "library", liked: "album", now: "now-playing", memos: "recordings", settings: "settings", poster: "charging" },
  render: (id) => renderers[id]()
});
