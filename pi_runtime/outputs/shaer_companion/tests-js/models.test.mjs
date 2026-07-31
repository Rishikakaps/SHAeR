import assert from "node:assert/strict";
import test from "node:test";
import { normalizePlayback, normalizePlaylist, normalizePlaylistCollection, normalizeQueue, normalizeSearch, normalizeTrack } from "../src/core/models.js";

test("normalizes a complete Spotify track", () => {
  const track = normalizeTrack({ id: "1", uri: "spotify:track:1", name: "Real Song", artists: [{ name: "One" }, { name: "Two" }], album: { name: "Real Album", images: [{ url: "https://art" }] }, duration_ms: 1234, explicit: true, track_number: 3 });
  assert.deepEqual(track.artists, ["One", "Two"]);
  assert.equal(track.artistText, "One, Two");
  assert.equal(track.artworkUrl, "https://art");
  assert.equal(track.explicit, true);
});

test("normalizes recent wrappers, local tracks, and missing metadata honestly", () => {
  const recent = normalizeTrack({ played_at: "2026-01-01", context: null, track: { name: "Field Recording", artist: "Rishika", is_local: true } });
  assert.equal(recent.title, "Field Recording");
  assert.equal(recent.isLocal, true);
  assert.equal(recent.playedAt, "2026-01-01");
  assert.equal(recent.album, "");
  assert.equal(normalizeTrack(null), null);
  assert.equal(normalizeTrack({}), null);
});

test("playlist normalizer tolerates null entries and track-count variants", () => {
  assert.equal(normalizePlaylist(null), null);
  assert.equal(normalizePlaylist({ name: "Mix", owner: { id: "owner" }, tracks: { total: 147 }, images: [] }).trackCount, 147);
  const collection = normalizePlaylistCollection({ items: [null, { id: "2", name: "Kept", items: { total: 4 } }] });
  assert.equal(collection.length, 1);
  assert.equal(collection[0].trackCount, 4);
});

test("playback, queue, and search share source-neutral models", () => {
  const playback = normalizePlayback({ is_playing: true, progress_ms: 100, device: { id: "d", name: "SHAeR", volume_percent: 60 }, item: { name: "Song", artists: [{ name: "Artist" }], duration_ms: 500 } });
  assert.equal(playback.currentTrack.title, "Song");
  assert.equal(playback.activeDeviceName, "SHAeR");
  const queue = normalizeQueue({ currently_playing: { name: "Now" }, queue: [null, { name: "Next" }] });
  assert.equal(queue.upcoming.length, 1);
  const search = normalizeSearch({ tracks: { items: [{ name: "Found" }] }, playlists: { items: [null] } });
  assert.equal(search.tracks[0].title, "Found");
  assert.deepEqual(search.playlists, []);
});
