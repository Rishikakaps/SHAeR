import {
  EMPTY_PLAYBACK,
  normalizePlayback,
  normalizePlaylistCollection,
  normalizeQueue,
  normalizeSearch,
  normalizeTrackCollection
} from "./models.js";

const initialCollection = () => ({ status: "idle", items: [], error: null, total: 0, offset: 0 });

export class MusicStore {
  constructor(client) {
    this.client = client;
    this.listeners = new Set();
    this.state = {
      connection: "unknown",
      authenticated: false,
      playback: { status: "idle", data: EMPTY_PLAYBACK, error: null, refreshedAt: 0 },
      queue: { status: "idle", currentTrack: null, items: [], error: null },
      saved: initialCollection(),
      playlists: initialCollection(),
      recent: initialCollection(),
      search: { status: "idle", query: "", results: normalizeSearch({}), error: null },
      profile: null
    };
  }

  subscribe(listener) {
    this.listeners.add(listener);
    listener(this.state);
    return () => this.listeners.delete(listener);
  }

  patch(mutator) {
    mutator(this.state);
    this.listeners.forEach((listener) => listener(this.state));
  }

  async refreshStatus() {
    try {
      const status = await this.client.spotifyStatus();
      this.patch((state) => {
        state.connection = "online";
        state.authenticated = Boolean(status?.authenticated);
      });
      return status;
    } catch (error) {
      this.patch((state) => { state.connection = error.kind === "network" ? "offline" : "error"; });
      throw error;
    }
  }

  async refreshPlayback() {
    this.patch((state) => { state.playback.status = "loading"; state.playback.error = null; });
    try {
      const payload = await this.client.spotifyPlayback();
      this.patch((state) => {
        state.playback = { status: "ready", data: normalizePlayback(payload), error: null, refreshedAt: Date.now() };
      });
    } catch (error) {
      this.patch((state) => {
        state.playback.status = error.status === 401 ? "unauthenticated" : error.kind === "network" ? "offline" : "error";
        state.playback.error = error;
      });
    }
  }

  async loadCollection(name, loader, normalizer, { append = false, offset = 0 } = {}) {
    this.patch((state) => { state[name].status = "loading"; state[name].error = null; });
    try {
      const payload = await loader();
      const items = normalizer(payload);
      const raw = payload?.[name] || payload || {};
      this.patch((state) => {
        state[name].items = append ? state[name].items.concat(items) : items;
        state[name].total = Number(raw.total ?? state[name].items.length);
        state[name].offset = offset;
        state[name].status = state[name].items.length ? "ready" : "empty";
      });
    } catch (error) {
      this.patch((state) => {
        state[name].status = error.status === 401 ? "unauthenticated" : error.kind === "network" ? "offline" : "error";
        state[name].error = error;
      });
    }
  }

  loadSaved(offset = 0, append = false) {
    return this.loadCollection("saved", () => this.client.spotifySaved(25, offset), (payload) => normalizeTrackCollection(payload, "tracks"), { append, offset });
  }

  loadPlaylists(offset = 0, append = false) {
    return this.loadCollection("playlists", () => this.client.spotifyPlaylists(25, offset), normalizePlaylistCollection, { append, offset });
  }

  loadRecent() {
    return this.loadCollection("recent", () => this.client.spotifyRecent(20), (payload) => normalizeTrackCollection(payload, "items"));
  }

  async loadQueue() {
    this.patch((state) => { state.queue.status = "loading"; state.queue.error = null; });
    try {
      const normalized = normalizeQueue(await this.client.spotifyQueue());
      this.patch((state) => {
        state.queue = { status: normalized.upcoming.length || normalized.currentTrack ? "ready" : "empty", currentTrack: normalized.currentTrack, items: normalized.upcoming, error: null };
      });
    } catch (error) {
      this.patch((state) => { state.queue.status = error.status === 401 ? "unauthenticated" : error.kind === "network" ? "offline" : "error"; state.queue.error = error; });
    }
  }

  async search(query) {
    const clean = String(query || "").trim();
    if (!clean) {
      this.patch((state) => { state.search = { status: "idle", query: "", results: normalizeSearch({}), error: null }; });
      return;
    }
    this.patch((state) => { state.search.status = "loading"; state.search.query = clean; state.search.error = null; });
    try {
      const results = normalizeSearch(await this.client.spotifySearch(clean, 10));
      this.patch((state) => {
        if (state.search.query !== clean) return;
        const count = results.tracks.length + results.playlists.length + results.albums.length + results.artists.length;
        state.search = { status: count ? "ready" : "empty", query: clean, results, error: null };
      });
    } catch (error) {
      if (error.kind === "cancelled") return;
      this.patch((state) => { state.search.status = error.status === 401 ? "unauthenticated" : error.kind === "network" ? "offline" : "error"; state.search.error = error; });
    }
  }
}
