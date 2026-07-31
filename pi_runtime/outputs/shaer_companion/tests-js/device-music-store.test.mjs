import assert from "node:assert/strict";
import test from "node:test";

const requests = [];
globalThis.fetch = async (path, options = {}) => {
  requests.push(String(path));
  if (String(path) === "/api/music/playlists") {
    return new Response(JSON.stringify({
      playlists: [{ id: 5, name: "Local Morning", track_count: 1 }]
    }), { status: 200, headers: { "content-type": "application/json" } });
  }
  if (String(path) === "/api/music/playlists/5/tracks") {
    return new Response(JSON.stringify({
      tracks: [{ id: 7, title: "Raag Bhairavi", artist: "Archive", filepath: "/music/raag.flac", duration_s: 122 }]
    }), { status: 200, headers: { "content-type": "application/json" } });
  }
  if (String(path) === "/api/music/playback") {
    const body = options.body ? JSON.parse(options.body) : {};
    return new Response(JSON.stringify({
      title: "Raag Bhairavi",
      artist: "Archive",
      album: "Morning",
      duration_ms: 122000,
      progress_ms: 0,
      status: body.action === "pause" ? "paused" : "playing",
      source: "local",
      uri: "local:track:7",
      id: "7"
    }), { status: 200, headers: { "content-type": "application/json" } });
  }
  return new Response(JSON.stringify({
    items: [
      null,
      {
        item: {
          id: "track-1",
          uri: "spotify:track:track-1",
          name: "Actual Playlist Track",
          artists: [{ name: "First Artist" }, { name: "Second Artist" }],
          album: { name: "Actual Album", images: [] },
          duration_ms: 181000
        }
      }
    ]
  }), { status: 200, headers: { "content-type": "application/json" } });
};

await import("../../shaer_pi_os/music-store.js");
const music = globalThis.SHAeRMusic;

test("device model normalizes Spotify wrappers and local metadata", () => {
  const wrapped = music.normalizeTrack({
    played_at: "2026-07-18T10:00:00Z",
    track: {
      id: "spotify-one",
      uri: "spotify:track:spotify-one",
      name: "A Real Track",
      artists: [{ name: "One" }, { name: "Two" }],
      album: { name: "A Real Album", images: [] },
      duration_ms: 125000
    }
  });
  assert.equal(wrapped.title, "A Real Track");
  assert.equal(wrapped.artistText, "One, Two");
  assert.equal(wrapped.artworkUrl, null);
  assert.equal(wrapped.playedAt, "2026-07-18T10:00:00Z");

  const local = music.normalizeTrack({ media_id: 9, title: "Field Note", artist: "Rishika", filepath: "/music/field-note.flac", duration_s: 61 }, "local");
  assert.equal(local.isLocal, true);
  assert.equal(local.duration, "1:01");
  assert.equal(music.normalizeTrack(null), null);
  assert.equal(music.normalizeTrack({}), null);
});

test("device model tolerates missing playlists and empty queues", () => {
  assert.equal(music.normalizePlaylist(null), null);
  const playlist = music.normalizePlaylist({ id: "mix", name: "Real Mix", tracks: { total: 12 }, images: [] });
  assert.equal(playlist.trackCount, 12);
  assert.equal(playlist.artworkUrl, null);
  assert.deepEqual(music.normalizeQueue({ queue: [null] }), []);
});

test("opening a playlist publishes loading then its real tracks without fallback rows", async () => {
  const statuses = [];
  const unsubscribe = music.subscribe((state) => statuses.push(state.status));
  const result = await music.openPlaylist({ id: "ABC123xyz", uri: "spotify:playlist:ABC123xyz", name: "My Real Mix" });
  unsubscribe();

  assert.equal(requests.at(-1), "/api/spotify/playlists/ABC123xyz/tracks?limit=50");
  assert.equal(result.selectedPlaylist.title, "My Real Mix");
  assert.equal(result.playlistTracks.length, 1);
  assert.equal(result.playlistTracks[0].title, "Actual Playlist Track");
  assert.deepEqual(statuses.slice(-2), ["loading", "ready"]);

  const closed = music.closePlaylist();
  assert.equal(closed.selectedPlaylist, null);
  assert.deepEqual(closed.playlistTracks, []);
});

test("local playlists and local playback use device music routes", async () => {
  const playlist = music.normalizePlaylist({ id: 5, name: "Local Morning", track_count: 1, source: "local" });
  assert.equal(playlist.isLocal, true);
  assert.equal(playlist.trackCount, 1);

  const opened = await music.openPlaylist(playlist);
  assert.equal(requests.at(-1), "/api/music/playlists/5/tracks");
  assert.equal(opened.playlistTracks[0].title, "Raag Bhairavi");
  assert.equal(opened.playlistTracks[0].isLocal, true);

  const playback = await music.controlLocalPlayback("play-track", opened.playlistTracks[0]);
  assert.equal(requests.at(-1), "/api/music/playback");
  assert.equal(playback.currentPlayback.source, "local");
  assert.equal(playback.currentPlayback.currentTrack.title, "Raag Bhairavi");
  assert.equal(playback.currentPlayback.isPlaying, true);
});
