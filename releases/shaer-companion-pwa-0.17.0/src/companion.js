import { ShaerApiClient, normalizeBaseUrl } from "./core/api-client.js";
import { CredentialVault } from "./core/credential-vault.js";
import { DeviceDiscovery } from "./core/discovery.js";
import { MusicStore } from "./core/music-store.js";

(function () {
  "use strict";

  const vault = new CredentialVault();
  const discovery = new DeviceDiscovery();
  const state = {
    token: "",
    baseUrl: "",
    deviceId: "",
    paired: false,
    pairingId: "",
    dashboardTimer: 0,
    tracks: [],
    playlists: [],
    recordings: [],
    settings: {},
    playbackSource: "unknown",
    volumePercent: null,
    spotifyView: "saved",
    spotifySearchTimer: 0,
    updateFile: null
  };
  const client = new ShaerApiClient({ tokenProvider: async () => state.token });
  const musicStore = new MusicStore(client);

  const $ = (selector) => document.querySelector(selector);
  const $$ = (selector) => Array.from(document.querySelectorAll(selector));
  const escapeHtml = (value) => String(value == null ? "" : value).replace(/[&<>"']/g, (character) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;", "'": "&#39;"
  }[character]));

  async function api(path, options = {}) {
    return client.request(path, options);
  }

  function setAppState(kind, title, detail) {
    const node = $("#appState");
    node.dataset.state = kind;
    node.querySelector("strong").textContent = title;
    node.querySelector("span").textContent = detail;
  }

  function toast(message, error = false) {
    const node = $("#toast");
    node.textContent = message;
    node.className = error ? "show error" : "show";
    window.clearTimeout(toast.timer);
    toast.timer = window.setTimeout(() => { node.className = ""; }, 3200);
  }

  function setConnection(online, label) {
    const node = $(".connection");
    node.classList.toggle("online", online);
    $("#connectionLabel").textContent = label;
  }

  function showView(view) {
    $$(".view").forEach((node) => node.classList.toggle("active", node.dataset.page === view));
    $$("#navigation [data-view]").forEach((node) => {
      const selected = node.dataset.view === view;
      node.classList.toggle("active", selected);
      if (selected) node.setAttribute("aria-current", "page");
      else node.removeAttribute("aria-current");
    });
    if (!state.paired) return;
    if (view === "music") loadMusic();
    if (view === "recordings") loadRecordings();
    if (view === "themes") loadThemes();
    if (view === "settings") loadSettings();
    if (view === "diagnostics") loadDiagnostics();
    if (view === "updates") loadUpdateStatus();
    if (view === "access") loadLinkedDevices();
  }

  async function discover() {
    setConnection(false, "Finding device");
    setAppState("connecting", "Connecting", "Looking for SHAeR on your local network.");
    try {
      const devices = await discovery.scan(state.baseUrl);
      if (!devices.length) throw new Error("No SHAeR device responded.");
      const device = devices.find((item) => item.baseUrl === state.baseUrl) || devices[0];
      state.baseUrl = device.baseUrl;
      client.setBaseUrl(device.baseUrl);
      setConnection(true, state.paired ? "Connected" : "SHAeR found");
      $("#miniDevice").textContent = device.device_name;
      $("#miniFirmware").textContent = device.firmware_version;
      setAppState("ready", "Connected", device.device_name || "SHAeR");
      if (!state.token) $("#pairingDialog").showModal();
      else await connect();
    } catch (error) {
      setConnection(false, "Device unavailable");
      setAppState("offline", "SHAeR unavailable", "Check that the device is powered on and connected to this local network.");
      await showDeviceDiscovery();
    }
  }

  async function showDeviceDiscovery() {
    const dialog = $("#deviceDialog");
    const root = $("#discoveredDevices");
    root.className = "discovered-devices empty-state";
    root.textContent = "Searching the local network...";
    if (!dialog.open) dialog.showModal();
    const devices = await discovery.scan(state.baseUrl);
    if (!devices.length) {
      root.textContent = "No device found automatically. Enter SHAeR's local address below.";
      return;
    }
    root.className = "discovered-devices";
    root.innerHTML = devices.map((device) => `<button class="discovered-device secondary" type="button" data-device-url="${escapeHtml(device.baseUrl)}"><span>${escapeHtml(device.device_name || "SHAeR")}</span><small>${escapeHtml(device.baseUrl)}</small></button>`).join("");
  }

  async function connectAddress(value) {
    try {
      state.baseUrl = normalizeBaseUrl(value);
      client.setBaseUrl(state.baseUrl);
      const device = await client.discovery();
      $("#deviceDialog").close();
      $("#miniDevice").textContent = device.device_name || "SHAeR";
      $("#miniFirmware").textContent = device.firmware_version || "Unknown firmware";
      if (state.token) await connect();
      else $("#pairingDialog").showModal();
    } catch (error) {
      toast(error.message || "That SHAeR address is unavailable.", true);
    }
  }

  async function connect() {
    try {
      await loadDashboard();
      state.paired = true;
      setConnection(true, "Connected");
      setAppState("ready", "Connected", "Live device data is available.");
      window.clearInterval(state.dashboardTimer);
      state.dashboardTimer = window.setInterval(loadDashboard, 2500);
    } catch (error) {
      state.paired = false;
      if (error.status === 401) {
        state.token = "";
        state.deviceId = "";
        await vault.clear();
        setAppState("unauthenticated", "Pairing required", "This companion credential is missing, expired, or revoked.");
        $("#pairingDialog").showModal();
      } else {
        setConnection(false, "Connection interrupted");
        setAppState("offline", "Paired but disconnected", "SHAeR will reconnect automatically when it returns to the network.");
      }
    }
  }

  async function startPairing() {
    const name = $("#companionName").value.trim();
    if (!name) return toast("Name this companion first.", true);
    try {
      const pairing = await api("/api/v1/pairing/start", { method: "POST", body: { device_name: name } });
      state.pairingId = pairing.pairing_id;
      $("#pairingCode").textContent = pairing.code;
      $("#pairingMessage").textContent = "Check this code on SHAeR, then press its OK button to trust this companion.";
      pollPairing();
    } catch (error) {
      toast(error.message, true);
    }
  }

  async function pollPairing() {
    if (!state.pairingId) return;
    try {
      const result = await api(`/api/v1/pairing/status?pairing_id=${encodeURIComponent(state.pairingId)}`);
      if (result.state === "paired" && result.token) {
        state.token = result.token;
        state.deviceId = result.device?.id || "";
        state.pairingId = "";
        await vault.save({ baseUrl: state.baseUrl, token: state.token, deviceId: state.deviceId, deviceName: $("#miniDevice").textContent });
        $("#pairingDialog").close();
        toast("Companion trusted by SHAeR.");
        await connect();
        return;
      }
      if (result.state === "denied") {
        state.pairingId = "";
        $("#pairingMessage").textContent = "SHAeR denied this pairing request.";
        return;
      }
      window.setTimeout(pollPairing, 900);
    } catch (error) {
      state.pairingId = "";
      toast(error.message, true);
    }
  }

  async function loadDashboard() {
    let dashboard;
    try {
      dashboard = await api("/api/v1/dashboard");
    } catch (error) {
      if (error.kind === "cancelled") return;
      throw error;
    }
    const playing = dashboard.now_playing || {};
    state.playbackSource = playing.source || "unknown";
    state.volumePercent = playing.volume_percent == null ? null : Number(playing.volume_percent);
    $("#trackTitle").textContent = playing.title || "Nothing playing";
    $("#trackArtist").textContent = [playing.artist, playing.album].filter(Boolean).join(" · ") || (dashboard.spotify_authenticated ? "Playback is idle" : "Spotify is not authenticated");
    $("#playbackSource").textContent = String(playing.source || "idle").toUpperCase();
    $("#spotifyStatus").textContent = dashboard.spotify_authenticated ? "Spotify connected" : "Spotify not connected";
    $("#batteryMetric").textContent = dashboard.battery_percent == null ? "Unavailable" : `${dashboard.battery_percent}%`;
    $("#chargingMetric").textContent = dashboard.charging == null ? "Hardware pending" : dashboard.charging ? "Charging" : "On battery";
    const storage = dashboard.storage || {};
    $("#storageMetric").textContent = storage.total ? `${Math.round(storage.used / storage.total * 100)}% used` : "--";
    $("#storageDetail").textContent = storage.free ? `${formatBytes(storage.free)} available` : "Waiting for device";
    $("#themeMetric").textContent = friendly(dashboard.current_theme || "--");
    $("#temperatureMetric").textContent = dashboard.cpu_temperature_c == null ? "Unavailable" : `${dashboard.cpu_temperature_c}°C`;
    $("#uptimeMetric").textContent = `Uptime ${formatDuration(dashboard.uptime_s || 0)}`;
    if (playing.cover_art) {
      $("#albumArtwork").innerHTML = `<img src="${escapeHtml(playing.cover_art)}" alt="Album artwork">`;
    } else {
      $("#albumArtwork").innerHTML = "<span>SHAeR</span>";
    }
    const playButton = $('[data-control="play_pause"]');
    playButton.innerHTML = playing.status === "playing" ? "&#10074;&#10074;" : "&#9654;";
    setConnection(true, "Connected");
    setAppState("ready", "Connected", "Live device data is available.");
  }

  async function control(action) {
    try {
      if (state.playbackSource === "spotify") {
        const spotifyAction = { play_pause: "toggle-play", next: "next", previous: "previous" }[action];
        if (spotifyAction) await client.spotifyControl(spotifyAction);
        else if (action === "volume_up" || action === "volume_down") {
          if (state.volumePercent == null) throw new Error("The active Spotify output does not expose volume control.");
          const value = Math.max(0, Math.min(100, state.volumePercent + (action === "volume_up" ? 5 : -5)));
          await client.spotifyControl("volume", { value });
          state.volumePercent = value;
        } else await api("/api/v1/playback/control", { method: "POST", body: { action } });
      } else {
        await api("/api/v1/playback/control", { method: "POST", body: { action } });
      }
      toast(`${friendly(action)} sent to SHAeR.`);
      window.setTimeout(loadDashboard, 250);
    } catch (error) { toast(error.message, true); }
  }

  async function loadMusic() {
    try {
      const [music, playlistData] = await Promise.all([api(`/api/v1/music/tracks?q=${encodeURIComponent($("#musicSearch").value)}`), api("/api/v1/music/playlists")]);
      state.tracks = music.tracks || [];
      state.playlists = playlistData.playlists || [];
      renderTracks();
      renderPlaylists();
    } catch (error) { toast(error.message, true); }
  }

  function renderTracks() {
    const node = $("#trackList");
    if (!state.tracks.length) {
      node.className = "data-list empty-state";
      node.textContent = "No matching tracks in the device library.";
      return;
    }
    node.className = "data-list";
    node.innerHTML = state.tracks.map((track) => `<div class="track-row"><div><strong>${escapeHtml(track.title || filename(track.filepath))}</strong><small>${escapeHtml(track.artist || "Unknown artist")}</small></div><span>${escapeHtml(track.album || "--")}</span><span>${formatTrackTime(track.duration_s)}</span><button class="row-action" type="button" data-delete-track="${track.id}" title="Delete track" aria-label="Delete ${escapeHtml(track.title || "track")}">×</button></div>`).join("");
  }

  function renderPlaylists() {
    const node = $("#playlistList");
    if (!state.playlists.length) {
      node.className = "data-list empty-state";
      node.textContent = "No playlists yet.";
      return;
    }
    node.className = "data-list";
    node.innerHTML = state.playlists.map((playlist) => `<button class="playlist-item" type="button" data-playlist="${playlist.id}"><span>${escapeHtml(playlist.name)}</span><small>${playlist.track_count} tracks</small></button>`).join("");
  }

  async function showMusicSource(source) {
    $$('[data-music-source]').forEach((button) => {
      const selected = button.dataset.musicSource === source;
      button.classList.toggle("active", selected);
      button.setAttribute("aria-selected", String(selected));
    });
    $("#localMusicPane").classList.toggle("active", source === "local");
    $("#spotifyMusicPane").classList.toggle("active", source === "spotify");
    if (source === "local") await loadMusic();
    else await loadSpotifyView(state.spotifyView);
  }

  async function loadSpotifyView(view, { append = false } = {}) {
    state.spotifyView = view;
    $$('[data-spotify-view]').forEach((button) => {
      const selected = button.dataset.spotifyView === view;
      button.classList.toggle("active", selected);
      button.setAttribute("aria-selected", String(selected));
    });
    const collection = $("#spotifyCollection");
    collection.className = "spotify-collection empty-state";
    collection.textContent = "Loading real Spotify data...";
    $("#spotifyLoadMore").hidden = true;
    try {
      const status = await musicStore.refreshStatus();
      if (!status?.authenticated) {
        $("#spotifyLibraryState").textContent = status?.configured ? "Spotify authentication required" : "Spotify is not configured";
        $("#spotifyLibraryDetail").textContent = status?.configured ? "Log in from SHAeR, then refresh this page." : "Set SPOTIFY_CLIENT_ID on SHAeR before signing in.";
        collection.textContent = "No Spotify account is authenticated. SHAeR does not substitute demo music.";
        return;
      }
      $("#spotifyLibraryState").textContent = "Spotify connected";
      $("#spotifyLibraryDetail").textContent = "Showing live account and playback data.";
      if (view === "saved") await musicStore.loadSaved(append ? musicStore.state.saved.items.length : 0, append);
      if (view === "playlists") await musicStore.loadPlaylists(append ? musicStore.state.playlists.items.length : 0, append);
      if (view === "recent") await musicStore.loadRecent();
      if (view === "queue") await musicStore.loadQueue();
      if (view === "search") await musicStore.search($("#spotifySearch").value);
      renderSpotifyCollection();
    } catch (error) {
      collection.textContent = error.message;
      setAppState(error.kind === "network" ? "offline" : "error", "Spotify unavailable", error.message);
    }
  }

  function collectionState(view) {
    if (view === "queue") return musicStore.state.queue;
    if (view === "search") return musicStore.state.search;
    return musicStore.state[view];
  }

  function renderSpotifyCollection() {
    const root = $("#spotifyCollection");
    const view = state.spotifyView;
    const source = collectionState(view);
    if (["loading", "idle"].includes(source.status)) {
      root.className = "spotify-collection empty-state";
      root.textContent = view === "search" ? "Type a Spotify search above." : "Loading real Spotify data...";
      return;
    }
    if (["error", "offline", "unauthenticated"].includes(source.status)) {
      root.className = "spotify-collection empty-state";
      root.textContent = source.error?.message || (source.status === "offline" ? "SHAeR is offline." : "Spotify authentication is required.");
      return;
    }

    let items = source.items || [];
    let kind = view === "playlists" ? "playlist" : "track";
    if (view === "queue") items = [source.currentTrack, ...(source.items || [])].filter(Boolean);
    if (view === "search") items = source.results?.tracks || [];
    if (!items.length) {
      root.className = "spotify-collection empty-state";
      root.textContent = view === "search" ? "No Spotify results match this search." : `No ${friendly(view).toLowerCase()} are available.`;
      return;
    }
    root.className = "spotify-collection";
    root.innerHTML = items.map((item, index) => kind === "playlist" ? spotifyPlaylistRow(item) : spotifyTrackRow(item, view === "queue" && index === 0 ? "Now playing" : "" )).join("");
    if (["saved", "playlists"].includes(view)) {
      const data = musicStore.state[view];
      $("#spotifyLoadMore").hidden = !data.total || data.items.length >= data.total;
    }
  }

  function spotifyTrackRow(track, label = "") {
    const artwork = track.artworkUrl ? `<img loading="lazy" src="${escapeHtml(track.artworkUrl)}" alt="">` : '<span class="spotify-artwork-placeholder"></span>';
    return `<article class="spotify-row">${artwork}<div><strong>${escapeHtml(track.title)}</strong><small>${escapeHtml(track.artistText)}</small></div><span>${escapeHtml(track.album || "Album unavailable")}</span><span>${label || formatMilliseconds(track.durationMs)}</span><button class="row-action" type="button" data-spotify-uri="${escapeHtml(track.uri || "")}" ${track.uri ? "" : "disabled"} title="Play on Spotify" aria-label="Play ${escapeHtml(track.title)}">&#9654;</button></article>`;
  }

  function spotifyPlaylistRow(playlist) {
    const artwork = playlist.artworkUrl ? `<img loading="lazy" src="${escapeHtml(playlist.artworkUrl)}" alt="">` : '<span class="spotify-artwork-placeholder"></span>';
    return `<article class="spotify-row">${artwork}<div><strong>${escapeHtml(playlist.name)}</strong><small>${escapeHtml(playlist.ownerName || "Owner unavailable")}</small></div><span>${playlist.trackCount} tracks</span><span>${playlist.isPublic == null ? "" : playlist.isPublic ? "Public" : "Private"}</span><button class="row-action" type="button" data-spotify-context="${escapeHtml(playlist.uri || "")}" ${playlist.uri ? "" : "disabled"} title="Play playlist" aria-label="Play ${escapeHtml(playlist.name)}">&#9654;</button></article>`;
  }

  async function playSpotify(button) {
    try {
      if (button.dataset.spotifyContext) await client.spotifyControl("play-context", { context_uri: button.dataset.spotifyContext });
      else await client.spotifyControl("play-uri", { uri: button.dataset.spotifyUri });
      toast("Playback request sent to Spotify.");
      window.setTimeout(loadDashboard, 350);
    } catch (error) { toast(error.message, true); }
  }

  async function uploadFiles(files, endpoint) {
    for (const file of files) {
      toast(`Uploading ${file.name}…`);
      const content = await fileBase64(file);
      await api(endpoint, { method: "POST", body: { filename: file.name, content_base64: content } });
    }
  }

  async function deleteTrack(id) {
    if (!window.confirm("Delete this track from SHAeR and its library?")) return;
    try {
      await api(`/api/v1/music/tracks/${id}`, { method: "DELETE" });
      toast("Track deleted.");
      loadMusic();
    } catch (error) { toast(error.message, true); }
  }

  async function createPlaylist() {
    const name = window.prompt("Playlist name");
    if (!name) return;
    try {
      await api("/api/v1/music/playlists", { method: "POST", body: { name, track_ids: [] } });
      toast("Playlist created.");
      loadMusic();
    } catch (error) { toast(error.message, true); }
  }

  async function loadThemes() {
    try {
      const data = await api("/api/v1/themes");
      const colors = [
        ["#06100d", "#00d89b"], ["#ddd7c8", "#b84c5c"], ["#0d0d10", "#ed0060"],
        ["#5b93d8", "#f1f1e8"], ["#5c7d78", "#f1d7bd"], ["#600000", "#ead9b8"]
      ];
      const node = $("#themeGrid");
      node.className = "theme-grid";
      node.innerHTML = (data.installed || []).map((theme, index) => `<article class="theme-card ${theme.id === data.active ? "active" : ""}"><div class="theme-preview" style="--preview:${colors[index % colors.length][0]};--accent:${colors[index % colors.length][1]}"></div><footer><div><strong>${escapeHtml(theme.name)}</strong><small>${theme.id === data.active ? "Active" : "Installed"}</small></div><button type="button" data-theme="${escapeHtml(theme.id)}">${theme.id === data.active ? "Active" : "Use"}</button></footer></article>`).join("");
    } catch (error) { toast(error.message, true); }
  }

  async function loadRecordings() {
    const query = new URLSearchParams();
    const search = $("#recordingSearch").value.trim();
    const month = $("#recordingMonth").value;
    if (search) query.set("q", search);
    if ($("#recordingFilter").value === "favorite") query.set("favorite", "true");
    if (month) {
      const [yearValue, monthValue] = month.split("-");
      query.set("year", yearValue);
      query.set("month", monthValue);
    }
    try {
      const data = await api(`/api/v1/recordings?${query}`);
      state.recordings = data.recordings || [];
      $("#recordingStorage").textContent = `${state.recordings.length} shown · ${formatBytes((data.storage || {}).recordings || 0)}`;
      renderRecordings();
    } catch (error) { toast(error.message, true); }
  }

  function renderRecordings() {
    const node = $("#recordingList");
    if (!state.recordings.length) {
      node.className = "recording-list empty-state";
      node.textContent = "No recordings match this archive view.";
      return;
    }
    node.className = "recording-list";
    node.innerHTML = state.recordings.map((item) => `<article class="recording-item ${item.status === "complete" || item.status === "recovered" ? "" : "recoverable"}" data-recording-id="${item.id}"><div class="recording-icon">${item.favorite ? "★" : "●"}</div><div class="recording-copy"><strong>${escapeHtml(item.display_title)}</strong><small>${escapeHtml(item.date)} · ${formatMilliseconds(item.duration_ms)} · ${formatBytes(item.file_size)} · ${escapeHtml(friendly(item.sync_status))}${item.status !== "complete" ? ` · ${escapeHtml(friendly(item.status))}` : ""}</small></div><div class="recording-actions"><button type="button" data-recording-action="favorite" title="${item.favorite ? "Remove favorite" : "Favorite"}" aria-label="${item.favorite ? "Remove favorite" : "Favorite"}">${item.favorite ? "★" : "☆"}</button><button type="button" data-recording-action="rename" title="Rename" aria-label="Rename">&#9998;</button><button type="button" data-recording-action="download" title="Download" aria-label="Download">&#8595;</button><button type="button" data-recording-action="duplicate" title="Duplicate" aria-label="Duplicate">&#9635;</button><button type="button" data-recording-action="move" title="Move to archive folder" aria-label="Move to archive folder">&#9633;</button><button type="button" data-recording-action="delete" title="Delete" aria-label="Delete">×</button></div></article>`).join("");
  }

  async function recordingAction(button) {
    const row = button.closest("[data-recording-id]");
    const id = Number(row.dataset.recordingId);
    const item = state.recordings.find((entry) => entry.id === id);
    const action = button.dataset.recordingAction;
    try {
      if (action === "download") {
        const response = await fetch(client.url(`/api/v1/recordings/${id}/download`), { headers: { Authorization: `Bearer ${state.token}` } });
        if (!response.ok) throw new Error("Recording download failed.");
        const url = URL.createObjectURL(await response.blob());
        const anchor = document.createElement("a");
        anchor.href = url;
        anchor.download = `${safeFilename(item.display_title)}.wav`;
        anchor.click();
        URL.revokeObjectURL(url);
        return;
      }
      if (action === "delete") {
        if (!window.confirm(`Delete “${item.display_title}” permanently?`)) return;
        await api(`/api/v1/recordings/${id}`, { method: "DELETE" });
      } else if (action === "favorite") {
        await api(`/api/v1/recordings/${id}`, { method: "POST", body: { favorite: !item.favorite } });
      } else if (action === "rename") {
        const title = window.prompt("Recording title", item.title || item.display_title);
        if (title == null) return;
        await api(`/api/v1/recordings/${id}`, { method: "POST", body: { title } });
      } else if (action === "duplicate") {
        await api(`/api/v1/recordings/${id}`, { method: "POST", body: { action: "duplicate" } });
      } else if (action === "move") {
        const folder = window.prompt("Archive folder", item.archive_folder || "Personal");
        if (!folder) return;
        await api(`/api/v1/recordings/${id}`, { method: "POST", body: { action: "move", folder } });
      }
      toast(`${friendly(action)} complete.`);
      loadRecordings();
    } catch (error) { toast(error.message, true); }
  }

  async function switchTheme(id) {
    try {
      await api("/api/v1/themes/active", { method: "POST", body: { theme_id: id } });
      toast("Theme switched on SHAeR.");
      await Promise.all([loadThemes(), loadDashboard()]);
    } catch (error) { toast(error.message, true); }
  }

  async function loadSettings() {
    try {
      state.settings = await api("/api/v1/settings");
      renderSettings();
    } catch (error) { toast(error.message, true); }
  }

  function renderSettings() {
    const root = $("#settingsForm");
    root.className = "settings-groups";
    root.innerHTML = Object.entries(state.settings).map(([category, values]) => `<section class="setting-group"><h2>${escapeHtml(category)}</h2>${Object.entries(values).map(([key, value]) => settingControl(category, key, value)).join("")}</section>`).join("");
  }

  function settingControl(category, key, value) {
    let control;
    const data = `data-setting-category="${escapeHtml(category)}" data-setting-key="${escapeHtml(key)}"`;
    if (typeof value === "boolean") control = `<input type="checkbox" ${value ? "checked" : ""} ${data}>`;
    else if (typeof value === "number") control = `<input type="number" value="${value}" ${Number.isInteger(value) ? "step=\"1\"" : "step=\"0.1\""} ${data}>`;
    else if (Array.isArray(value)) control = `<input type="text" value="${escapeHtml(value.join(", "))}" ${data} data-array="true">`;
    else control = `<input type="text" value="${escapeHtml(value)}" ${data}>`;
    return `<div class="setting-row"><label>${escapeHtml(friendly(key))}</label>${control}</div>`;
  }

  async function saveSetting(input) {
    let value = input.type === "checkbox" ? input.checked : input.type === "number" ? Number(input.value) : input.value;
    if (input.dataset.array) value = String(value).split(",").map((item) => item.trim()).filter(Boolean);
    $("#settingsState").textContent = "Syncing…";
    try {
      await api("/api/v1/settings", { method: "POST", body: { [input.dataset.settingCategory]: { [input.dataset.settingKey]: value } } });
      $("#settingsState").textContent = "Synced";
    } catch (error) {
      $("#settingsState").textContent = "Sync failed";
      toast(error.message, true);
    }
  }

  async function loadLinkedDevices() {
    const root = $("#linkedDeviceList");
    root.className = "linked-device-list empty-state";
    root.textContent = "Loading linked devices...";
    try {
      const data = await client.linkedDevices();
      const devices = data.devices || [];
      if (!devices.length) {
        root.textContent = "No linked companion devices.";
        return;
      }
      root.className = "linked-device-list";
      root.innerHTML = devices.map((device) => `<article class="linked-device"><div><strong>${escapeHtml(device.name || "Unnamed companion")}${device.id === state.deviceId ? " (this device)" : ""}</strong><small>${escapeHtml(friendly(device.role || "owner"))} · Paired ${formatDate(device.paired_at)} · Last seen ${formatDate(device.last_seen_at)}</small></div><button class="danger" type="button" data-revoke-device="${escapeHtml(device.id)}">Revoke</button></article>`).join("");
    } catch (error) {
      root.textContent = error.message;
    }
  }

  async function revokeDevice(deviceId) {
    if (!window.confirm("Revoke this companion's access to SHAeR?")) return;
    try {
      await client.revokeDevice(deviceId);
      if (deviceId === state.deviceId) {
        state.token = "";
        state.deviceId = "";
        state.paired = false;
        await vault.clear();
        setAppState("unauthenticated", "Access revoked", "Pair again using SHAeR's physical OK button to reconnect.");
        $("#pairingDialog").showModal();
      } else await loadLinkedDevices();
      toast("Companion access revoked.");
    } catch (error) { toast(error.message, true); }
  }

  async function loadDiagnostics() {
    try {
      const data = await api("/api/v1/diagnostics");
      const node = $("#diagnosticList");
      node.className = "diagnostic-list";
      node.innerHTML = (data.available || []).map((name) => `<div class="diagnostic-item"><span>${escapeHtml(friendly(name.replace(/_test$/, "")))}</span><button type="button" data-diagnostic="${escapeHtml(name)}">Run</button></div>`).join("");
    } catch (error) { toast(error.message, true); }
  }

  async function runDiagnostic(name) {
    const output = $("#diagnosticOutput");
    output.hidden = false;
    output.textContent = `Running ${name}…`;
    try {
      const data = await api("/api/v1/diagnostics/run", { method: "POST", body: { name, hardware: false } });
      output.textContent = `${data.passed ? "PASS" : "FAIL"}: ${friendly(name)}\n\n${data.output || data.error || "No output"}`;
    } catch (error) { output.textContent = `ERROR: ${error.message}`; }
  }

  async function runAllDiagnostics() {
    const buttons = $$("[data-diagnostic]");
    for (const button of buttons) await runDiagnostic(button.dataset.diagnostic);
    toast("Contract diagnostics finished.");
  }

  async function loadUpdateStatus() {
    try {
      const status = await api("/api/v1/updates/status");
      $("#updateState").textContent = friendly(status.state || "idle");
    } catch (error) { toast(error.message, true); }
  }

  async function stageUpdate() {
    const file = $("#updatePackage").files[0];
    if (!file) return toast("Choose an update package.", true);
    try {
      const content = await fileBase64(file);
      await api("/api/v1/updates/stage", { method: "POST", body: { manifest: { version: $("#updateVersion").value, sha256: $("#updateChecksum").value, development_unsigned: $("#developmentUnsigned").checked }, content_base64: content } });
      toast("Update verified and staged.");
      loadUpdateStatus();
    } catch (error) { toast(error.message, true); }
  }

  async function installUpdate() {
    if (!window.confirm("Install the staged update and reboot SHAeR?")) return;
    try {
      await api("/api/v1/updates/install", { method: "POST", body: {} });
      toast("Update installation started.");
    } catch (error) { toast(error.message, true); }
  }

  async function createBackup() {
    try {
      const include = $$('[name="backupInclude"]:checked').map((input) => input.value);
      const data = await api("/api/v1/backup/create", { method: "POST", body: { passphrase: $("#backupPassphrase").value, include } });
      downloadBase64(data.filename, data.content_base64, "application/octet-stream");
      toast("Encrypted backup created.");
    } catch (error) { toast(error.message, true); }
  }

  async function restoreBackup() {
    const file = $("#restoreFile").files[0];
    if (!file || !window.confirm("Restore selected data to SHAeR?")) return;
    try {
      const include = $$('[name="restoreInclude"]:checked').map((input) => input.value);
      await api("/api/v1/backup/restore", { method: "POST", body: { passphrase: $("#restorePassphrase").value, include, content_base64: await fileBase64(file) } });
      toast("Selected backup data restored.");
    } catch (error) { toast(error.message, true); }
  }

  function fileBase64(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onerror = () => reject(reader.error);
      reader.onload = () => resolve(String(reader.result).split(",", 2)[1] || "");
      reader.readAsDataURL(file);
    });
  }

  function downloadBase64(name, content, type) {
    const binary = window.atob(content);
    const bytes = Uint8Array.from(binary, (character) => character.charCodeAt(0));
    const url = URL.createObjectURL(new Blob([bytes], { type }));
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = name;
    anchor.click();
    URL.revokeObjectURL(url);
  }

  function friendly(value) { return String(value).replace(/^shaer_/, "").replace(/_/g, " ").replace(/\b\w/g, (letter) => letter.toUpperCase()); }
  function filename(path) { return String(path || "Unknown track").split(/[\\/]/).pop(); }
  function formatBytes(value) { if (!value) return "0 B"; const units = ["B", "KB", "MB", "GB", "TB"]; const rank = Math.min(units.length - 1, Math.floor(Math.log(value) / Math.log(1024))); return `${(value / 1024 ** rank).toFixed(rank > 1 ? 1 : 0)} ${units[rank]}`; }
  function formatDuration(seconds) { const hours = Math.floor(seconds / 3600); const minutes = Math.floor(seconds % 3600 / 60); return hours ? `${hours}h ${minutes}m` : `${minutes}m`; }
  function formatTrackTime(seconds) { if (seconds == null) return "--"; return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, "0")}`; }
  function formatMilliseconds(milliseconds) { const seconds = Math.max(0, Math.round((milliseconds || 0) / 1000)); return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, "0")}`; }
  function formatDate(seconds) { if (!seconds) return "never"; return new Intl.DateTimeFormat(undefined, { dateStyle: "medium", timeStyle: "short" }).format(new Date(Number(seconds) * 1000)); }
  function safeFilename(value) { return String(value || "recording").replace(/[^a-z0-9._-]+/gi, "_").slice(0, 80); }

  $$("[data-view]").forEach((button) => button.addEventListener("click", () => showView(button.dataset.view)));
  $$("[data-control]").forEach((button) => button.addEventListener("click", () => control(button.dataset.control)));
  $("#refreshButton").addEventListener("click", discover);
  $("#startPairing").addEventListener("click", startPairing);
  $("#connectManual").addEventListener("click", () => connectAddress($("#manualDeviceAddress").value));
  $("#discoveredDevices").addEventListener("click", (event) => { const button = event.target.closest("[data-device-url]"); if (button) connectAddress(button.dataset.deviceUrl); });
  $$('[data-music-source]').forEach((button) => button.addEventListener("click", () => showMusicSource(button.dataset.musicSource)));
  $$('[data-spotify-view]').forEach((button) => button.addEventListener("click", () => loadSpotifyView(button.dataset.spotifyView)));
  $("#spotifyRefresh").addEventListener("click", () => loadSpotifyView(state.spotifyView));
  $("#spotifySearch").addEventListener("input", () => {
    window.clearTimeout(state.spotifySearchTimer);
    state.spotifySearchTimer = window.setTimeout(() => loadSpotifyView("search"), 400);
  });
  $("#spotifyCollection").addEventListener("click", (event) => { const button = event.target.closest("[data-spotify-uri], [data-spotify-context]"); if (button) playSpotify(button); });
  $("#spotifyLoadMore").addEventListener("click", () => loadSpotifyView(state.spotifyView, { append: true }));
  $("#musicSearch").addEventListener("input", () => { window.clearTimeout(state.searchTimer); state.searchTimer = window.setTimeout(loadMusic, 250); });
  $("#newPlaylist").addEventListener("click", createPlaylist);
  $("#recordingSearch").addEventListener("input", () => { window.clearTimeout(state.recordingSearchTimer); state.recordingSearchTimer = window.setTimeout(loadRecordings, 250); });
  $("#recordingFilter").addEventListener("change", loadRecordings);
  $("#recordingMonth").addEventListener("change", loadRecordings);
  $("#recordingList").addEventListener("click", (event) => { const button = event.target.closest("[data-recording-action]"); if (button) recordingAction(button); });
  $("#musicUpload").addEventListener("change", async (event) => { try { await uploadFiles(event.target.files, "/api/v1/music/upload"); toast("Music added and indexed."); loadMusic(); } catch (error) { toast(error.message, true); } event.target.value = ""; });
  $("#trackList").addEventListener("click", (event) => { const button = event.target.closest("[data-delete-track]"); if (button) deleteTrack(Number(button.dataset.deleteTrack)); });
  $("#themeGrid").addEventListener("click", (event) => { const button = event.target.closest("[data-theme]"); if (button) switchTheme(button.dataset.theme); });
  $("#settingsForm").addEventListener("change", (event) => { if (event.target.dataset.settingKey) saveSetting(event.target); });
  $("#diagnosticList").addEventListener("click", (event) => { const button = event.target.closest("[data-diagnostic]"); if (button) runDiagnostic(button.dataset.diagnostic); });
  $("#runAllDiagnostics").addEventListener("click", runAllDiagnostics);
  $("#stageUpdate").addEventListener("click", stageUpdate);
  $("#installUpdate").addEventListener("click", installUpdate);
  $("#createBackup").addEventListener("click", createBackup);
  $("#restoreBackup").addEventListener("click", restoreBackup);
  $("#linkedDeviceList").addEventListener("click", (event) => { const button = event.target.closest("[data-revoke-device]"); if (button) revokeDevice(button.dataset.revokeDevice); });
  $("#pairAnotherDevice").addEventListener("click", () => $("#pairingDialog").showModal());

  async function boot() {
    const currentOrigin = /^https?:$/.test(window.location.protocol) ? window.location.origin : "http://shaer.local:8775";
    const credential = await vault.load() || await vault.migrateLegacy(currentOrigin);
    if (credential) {
      state.token = credential.token || "";
      state.baseUrl = credential.baseUrl || currentOrigin;
      state.deviceId = credential.deviceId || "";
      client.setBaseUrl(state.baseUrl);
    }
    if ("serviceWorker" in navigator && !window.Capacitor) {
      navigator.serviceWorker.register("service-worker.js").catch(() => {});
    }
    await discover();
  }

  boot();
}());
