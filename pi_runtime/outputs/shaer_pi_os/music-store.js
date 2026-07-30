(function (root) {
  "use strict";

  const listeners = new Set();
  const requestControllers = new Map();
  let pollTimer = 0;
  let stopped = true;

  const initialState = () => ({
    status: "loading",
    spotify: { configured: false, authenticated: false, status: "loading" },
    currentPlayback: {
      currentTrack: null,
      isPlaying: false,
      progressMs: 0,
      durationMs: 0,
      volumePercent: null,
      shuffle: false,
      repeatMode: "off",
      activeDeviceId: null,
      activeDeviceName: null,
      source: "unknown"
    },
    queue: [],
    playlists: [],
    localPlaylists: [],
    selectedPlaylist: null,
    playlistTracks: [],
    savedTracks: [],
    recentTracks: [],
    localTracks: [],
    searchResults: { tracks: [], playlists: [], albums: [], artists: [] },
    error: null,
    lastSuccessfulRefresh: null
  });

  let state = initialState();

  function formatDuration(milliseconds) {
    const seconds = Math.max(0, Math.round((Number(milliseconds) || 0) / 1000));
    return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, "0")}`;
  }

  function artistNames(value) {
    if (Array.isArray(value)) return value.map((artist) => artist && artist.name).filter(Boolean);
    if (typeof value === "string" && value.trim()) return [value.trim()];
    return [];
  }

  function normalizeTrack(input, sourceHint) {
    const wrapper = input && typeof input === "object" ? input : null;
    const track = wrapper && (wrapper.track || wrapper.item || wrapper);
    if (!track || typeof track !== "object") return null;
    const artists = artistNames(track.artists || track.artist);
    const albumObject = track.album && typeof track.album === "object" ? track.album : null;
    const album = albumObject ? String(albumObject.name || "") : String(track.album || "");
    const images = albumObject && Array.isArray(albumObject.images) ? albumObject.images : (Array.isArray(track.images) ? track.images : []);
    const durationMs = Math.max(0, Number(track.durationMs ?? track.duration_ms ?? (Number(track.duration_s) || 0) * 1000) || 0);
    const uri = track.uri ? String(track.uri) : null;
    const source = String(track.source || sourceHint || (uri && uri.startsWith("spotify:") ? "spotify" : "local"));
    const title = String(track.title || track.name || "").trim();
    if (!title && !uri && track.id == null && !track.filepath) return null;
    return {
      id: String(track.id ?? track.media_id ?? uri ?? track.filepath ?? ""),
      uri,
      title: title || "Unknown track",
      artists,
      artistText: artists.length ? artists.join(", ") : "Unknown artist",
      artist: artists.length ? artists.join(", ") : "Unknown artist",
      album,
      artworkUrl: String(track.artworkUrl || track.cover_art || track.cover_art_path || (images[0] && images[0].url) || "") || null,
      durationMs,
      duration: formatDuration(durationMs),
      explicit: Boolean(track.explicit),
      isLocal: source === "local" || Boolean(track.is_local),
      trackNumber: track.track_number == null ? null : Number(track.track_number),
      contextUri: String(wrapper.contextUri || (wrapper.context && wrapper.context.uri) || "") || null,
      playedAt: String(wrapper.playedAt || wrapper.played_at || "") || null,
      source
    };
  }

  function normalizePlaylist(input) {
    if (!input || typeof input !== "object") return null;
    const images = Array.isArray(input.images) ? input.images : [];
    const total = input.trackCount ?? input.track_count ?? (input.tracks && input.tracks.total) ?? (input.items && input.items.total) ?? 0;
    const name = String(input.name || input.title || "").trim();
    const uri = input.uri ? String(input.uri) : null;
    const source = String(input.source || input.provider || (uri && uri.startsWith("spotify:") ? "spotify" : input.spotify_playlist_id ? "spotify" : "local"));
    if (!name && input.id == null && !input.uri) return null;
    return {
      id: String(input.id ?? input.uri ?? ""),
      uri,
      name: name || "Untitled playlist",
      title: name || "Untitled playlist",
      description: String(input.description || ""),
      artworkUrl: String(input.artworkUrl || (images[0] && images[0].url) || "") || null,
      ownerName: String((input.owner && (input.owner.display_name || input.owner.id)) || ""),
      trackCount: Math.max(0, Number(total) || 0),
      count: Math.max(0, Number(total) || 0),
      isPublic: typeof input.public === "boolean" ? input.public : null,
      collaborative: Boolean(input.collaborative),
      source,
      isLocal: Boolean(input.isLocal || input.is_local || source === "local")
    };
  }

  function normalizePlayback(input) {
    const payload = input && typeof input === "object" ? input : {};
    let track = normalizeTrack(payload.currentTrack || payload.item, "spotify");
    if (!track && (payload.title || payload.uri)) {
      track = normalizeTrack({
        id: payload.uri || "",
        uri: payload.uri || null,
        title: payload.title,
        artist: payload.artist,
        album: payload.album,
        cover_art: payload.cover_art,
        duration_ms: payload.duration_ms,
        source: payload.source
      }, payload.source);
    }
    const device = payload.device && typeof payload.device === "object" ? payload.device : {};
    return {
      currentTrack: track,
      isPlaying: Boolean(payload.isPlaying ?? payload.is_playing ?? String(payload.status || "").toLowerCase() === "playing"),
      progressMs: Math.max(0, Number(payload.progressMs ?? payload.progress_ms) || 0),
      durationMs: Math.max(0, Number(payload.durationMs ?? payload.duration_ms ?? (track && track.durationMs)) || 0),
      volumePercent: payload.volumePercent ?? payload.volume_percent ?? device.volume_percent ?? null,
      shuffle: Boolean(payload.shuffle ?? payload.shuffle_state),
      repeatMode: String(payload.repeatMode || payload.repeat_state || "off"),
      activeDeviceId: String(payload.activeDeviceId || device.id || "") || null,
      activeDeviceName: String(payload.activeDeviceName || device.name || "") || null,
      source: String(payload.source || (track && track.source) || "unknown")
    };
  }

  function collectionItems(payload) {
    if (Array.isArray(payload)) return payload;
    if (payload && Array.isArray(payload.items)) return payload.items;
    if (payload && payload.data && Array.isArray(payload.data.items)) return payload.data.items;
    return [];
  }

  function normalizeQueue(payload) {
    const items = payload && Array.isArray(payload.queue) ? payload.queue : collectionItems(payload);
    return items.map((item) => normalizeTrack(item, "spotify")).filter(Boolean);
  }

  function errorFor(kind, message, status) {
    const error = new Error(message || "SHAeR request failed.");
    error.kind = kind;
    error.status = Number(status) || 0;
    return error;
  }

  async function request(path, key = path, options = {}) {
    const previous = requestControllers.get(key);
    if (previous) previous.abort();
    const controller = new AbortController();
    requestControllers.set(key, controller);
    let response;
    try {
      const headers = { Accept: "application/json", ...(options.headers || {}) };
      if (options.body && !headers["Content-Type"]) headers["Content-Type"] = "application/json";
      response = await root.fetch(path, {
        cache: "no-store",
        signal: controller.signal,
        method: options.method || "GET",
        headers,
        body: options.body
      });
    } catch (error) {
      if (error && error.name === "AbortError") throw errorFor("cancelled", "A newer request replaced this one.");
      throw errorFor("network", "SHAeR cannot reach its music service.");
    } finally {
      if (requestControllers.get(key) === controller) requestControllers.delete(key);
    }
    const text = await response.text();
    let payload = {};
    if (text.trim()) {
      try { payload = JSON.parse(text); }
      catch { throw errorFor("parse", "The music service returned an unreadable response.", response.status); }
    }
    if (!response.ok) {
      const detail = payload && payload.error;
      const message = typeof detail === "string" ? detail : (detail && detail.message) || `Music request failed (${response.status}).`;
      throw errorFor(response.status === 401 ? "unauthenticated" : "http", message, response.status);
    }
    return payload && payload.data && payload.protocol_version ? payload.data : payload;
  }

  function publish(patch) {
    state = { ...state, ...patch };
    const snapshot = getState();
    listeners.forEach((listener) => listener(snapshot));
    if (root.dispatchEvent && root.CustomEvent) root.dispatchEvent(new root.CustomEvent("shaer:music-state", { detail: snapshot }));
    return snapshot;
  }

  function getState() {
    return {
      ...state,
      spotify: { ...state.spotify },
      currentPlayback: { ...state.currentPlayback },
      queue: state.queue.slice(),
      playlists: state.playlists.slice(),
      localPlaylists: state.localPlaylists.slice(),
      selectedPlaylist: state.selectedPlaylist ? { ...state.selectedPlaylist } : null,
      playlistTracks: state.playlistTracks.slice(),
      savedTracks: state.savedTracks.slice(),
      recentTracks: state.recentTracks.slice(),
      localTracks: state.localTracks.slice(),
      searchResults: { ...state.searchResults }
    };
  }

  async function refreshLocal() {
    try {
      const [tracksPayload, playlistsPayload] = await Promise.all([
        request("/api/music/tracks?limit=50", "local-library"),
        request("/api/music/playlists", "local-playlists")
      ]);
      const trackItems = (tracksPayload && tracksPayload.tracks) || collectionItems(tracksPayload);
      const playlistItems = (playlistsPayload && playlistsPayload.playlists) || collectionItems(playlistsPayload);
      return {
        tracks: trackItems.map((item) => normalizeTrack(item, "local")).filter(Boolean),
        playlists: playlistItems.map((item) => normalizePlaylist({ ...item, source: "local", isLocal: true })).filter(Boolean)
      };
    } catch (error) {
      if (error.kind === "cancelled") return { tracks: state.localTracks, playlists: state.localPlaylists };
      return { tracks: state.localTracks, playlists: state.localPlaylists };
    }
  }

  async function refreshCollections() {
    const local = await refreshLocal();
    if (!state.spotify.authenticated) {
      return publish({
        localTracks: local.tracks,
        localPlaylists: local.playlists,
        playlists: local.playlists,
        status: local.tracks.length || local.playlists.length ? "ready" : "unauthenticated"
      });
    }
    const [saved, playlists, recent] = await Promise.all([
      request("/api/spotify/library/tracks?limit=20", "saved"),
      request("/api/spotify/library/playlists?limit=20", "playlists"),
      request("/api/spotify/library/recent?limit=10", "recent")
    ]);
    const spotifyPlaylists = collectionItems(playlists).map((item) => normalizePlaylist({ ...item, source: "spotify" })).filter(Boolean);
    return publish({
      savedTracks: collectionItems(saved).map((item) => normalizeTrack(item, "spotify")).filter(Boolean),
      playlists: [...local.playlists, ...spotifyPlaylists],
      localPlaylists: local.playlists,
      recentTracks: collectionItems(recent).map((item) => normalizeTrack(item, "spotify")).filter(Boolean),
      localTracks: local.tracks,
      status: "ready",
      error: null,
      lastSuccessfulRefresh: new Date().toISOString()
    });
  }

  async function refreshNow() {
    try {
      const status = await request("/api/spotify/status", "spotify-status");
      const spotify = {
        configured: Boolean(status.configured),
        authenticated: Boolean(status.authenticated),
        status: status.authenticated ? "ready" : "unauthenticated"
      };
      publish({ spotify, error: null });
      if (!spotify.authenticated) {
        const [local, localPlayback] = await Promise.all([
          refreshLocal(),
          request("/api/music/playback", "local-playback").catch(() => null)
        ]);
        return publish({
          localTracks: local.tracks,
          localPlaylists: local.playlists,
          playlists: local.playlists,
          currentPlayback: normalizePlayback(localPlayback || { source: "local", status: "stopped" }),
          queue: [],
          status: local.tracks.length || local.playlists.length ? "ready" : "unauthenticated",
          lastSuccessfulRefresh: new Date().toISOString()
        });
      }
      const [playbackPayload, queuePayload, localPlayback] = await Promise.all([
        request("/api/spotify/playback", "playback"),
        request("/api/spotify/queue", "queue"),
        request("/api/music/playback", "local-playback").catch(() => null)
      ]);
      const localIsActive = localPlayback && localPlayback.uri && ["playing", "paused", "buffering"].includes(String(localPlayback.status || "").toLowerCase());
      return publish({
        currentPlayback: normalizePlayback(localIsActive ? localPlayback : playbackPayload),
        queue: normalizeQueue(queuePayload),
        status: "ready",
        error: null,
        lastSuccessfulRefresh: new Date().toISOString()
      });
    } catch (error) {
      if (error.kind === "cancelled") return getState();
      return publish({
        status: error.kind === "unauthenticated" ? "unauthenticated" : (error.kind === "network" ? "offline" : "error"),
        error: { kind: error.kind || "error", message: error.message, status: error.status || 0 }
      });
    }
  }

  async function refreshAll() {
    publish({ status: "loading", error: null });
    await refreshNow();
    try { await refreshCollections(); }
    catch (error) {
      if (error.kind !== "cancelled") publish({ status: error.kind === "network" ? "offline" : "error", error: { kind: error.kind, message: error.message, status: error.status || 0 } });
    }
    return getState();
  }

  async function search(query) {
    const term = String(query || "").trim();
    if (!term) return publish({ searchResults: initialState().searchResults });
    try {
      const payload = await request(`/api/spotify/search?q=${encodeURIComponent(term)}&limit=10`, "search");
      const results = {
        tracks: collectionItems(payload.tracks).map((item) => normalizeTrack(item, "spotify")).filter(Boolean),
        playlists: collectionItems(payload.playlists).map(normalizePlaylist).filter(Boolean),
        albums: collectionItems(payload.albums).filter(Boolean),
        artists: collectionItems(payload.artists).filter(Boolean)
      };
      return publish({ searchResults: results, error: null });
    } catch (error) {
      if (error.kind !== "cancelled") publish({ error: { kind: error.kind, message: error.message, status: error.status || 0 } });
      return getState();
    }
  }

  async function openPlaylist(playlist) {
    const selectedPlaylist = normalizePlaylist(playlist);
    if (!selectedPlaylist || !selectedPlaylist.id) {
      return publish({ status: "error", error: { kind: "invalid", message: "This playlist is unavailable.", status: 0 } });
    }
    publish({ selectedPlaylist, playlistTracks: [], status: "loading", error: null });
    try {
      const local = selectedPlaylist.isLocal || selectedPlaylist.source === "local";
      const path = local
        ? `/api/music/playlists/${encodeURIComponent(selectedPlaylist.id)}/tracks`
        : `/api/spotify/playlists/${encodeURIComponent(selectedPlaylist.id)}/tracks?limit=50`;
      const payload = await request(path, "playlist-tracks");
      const playlistTracks = ((payload && payload.tracks) || collectionItems(payload))
        .map((item) => normalizeTrack(item, local ? "local" : "spotify")).filter(Boolean);
      return publish({ selectedPlaylist, playlistTracks, status: "ready", error: null });
    } catch (error) {
      if (error.kind !== "cancelled") {
        publish({
          status: error.kind === "network" ? "offline" : (error.kind === "unauthenticated" ? "unauthenticated" : "error"),
          error: { kind: error.kind || "error", message: error.message, status: error.status || 0 }
        });
      }
      return getState();
    }
  }

  function closePlaylist() {
    return publish({ selectedPlaylist: null, playlistTracks: [], error: null });
  }

  async function controlLocalPlayback(action, track) {
    const payload = { action };
    if (track) {
      payload.id = track.id;
      payload.media_id = track.id;
      payload.uri = track.uri;
    }
    try {
      const playback = await request("/api/music/playback", "local-playback-control", {
        method: "POST",
        body: JSON.stringify(payload)
      });
      return publish({ currentPlayback: normalizePlayback(playback), status: "ready", error: null });
    } catch (error) {
      if (error.kind !== "cancelled") {
        publish({
          status: "error",
          error: { kind: error.kind || "error", message: error.message, status: error.status || 0 }
        });
      }
      return getState();
    }
  }

  function schedule() {
    if (stopped) return;
    const delay = state.currentPlayback.isPlaying ? 3000 : (root.document && root.document.hidden ? 15000 : 8000);
    root.clearTimeout(pollTimer);
    pollTimer = root.setTimeout(async () => {
      await refreshNow();
      schedule();
    }, delay);
  }

  async function start() {
    if (!stopped) return getState();
    stopped = false;
    await refreshAll();
    schedule();
    return getState();
  }

  function stop() {
    stopped = true;
    root.clearTimeout(pollTimer);
    requestControllers.forEach((controller) => controller.abort());
    requestControllers.clear();
  }

  function subscribe(listener) {
    listeners.add(listener);
    listener(getState());
    return () => listeners.delete(listener);
  }

  function viewMessage(status) {
    if (status === "loading") return "LOADING MUSIC";
    if (status === "offline") return "MUSIC OFFLINE";
    if (status === "unauthenticated") return "SPOTIFY SIGN IN";
    if (status === "error") return "MUSIC UNAVAILABLE";
    return "NO MUSIC FOUND";
  }

  root.SHAeRMusic = Object.freeze({
    normalizeTrack,
    normalizePlaylist,
    normalizePlayback,
    normalizeQueue,
    formatDuration,
    viewMessage,
    getState,
    subscribe,
    start,
    stop,
    refreshAll,
    refreshNow,
    refreshCollections,
    openPlaylist,
    closePlaylist,
    controlLocalPlayback,
    search
  });
})(typeof window !== "undefined" ? window : globalThis);
