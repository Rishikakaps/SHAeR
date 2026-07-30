function object(value) {
  return value && typeof value === "object" && !Array.isArray(value) ? value : {};
}

function text(value, fallback = "") {
  return typeof value === "string" && value.trim() ? value.trim() : fallback;
}

function number(value, fallback = 0) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function firstImage(input) {
  const images = Array.isArray(input) ? input : [];
  const image = images.find((item) => object(item).url);
  return image ? text(object(image).url) || null : null;
}

export function normalizeTrack(input) {
  if (!input || typeof input !== "object") return null;
  const wrapper = object(input);
  const track = object(wrapper.track && typeof wrapper.track === "object" ? wrapper.track : input);
  if (!Object.keys(track).length) return null;

  const artistObjects = Array.isArray(track.artists) ? track.artists : [];
  const artists = artistObjects.map((artist) => text(object(artist).name)).filter(Boolean);
  const legacyArtist = text(track.artist);
  if (!artists.length && legacyArtist) artists.push(legacyArtist);
  const albumObject = object(track.album);
  const album = text(albumObject.name || track.album, "");
  const artworkUrl = firstImage(albumObject.images) || firstImage(track.images) || text(track.cover_art || track.artworkUrl) || null;
  const durationMs = Math.max(0, number(track.duration_ms ?? track.durationMs ?? number(track.duration_s) * 1000));

  return {
    id: text(track.id || track.media_id || track.uri),
    uri: text(track.uri) || null,
    title: text(track.name || track.title, "Unknown track"),
    artists,
    artistText: artists.length ? artists.join(", ") : "Unknown artist",
    album,
    artworkUrl,
    durationMs,
    explicit: Boolean(track.explicit),
    isLocal: Boolean(track.is_local || track.isLocal || track.provider === "local"),
    trackNumber: track.track_number == null ? null : number(track.track_number),
    contextUri: text(object(wrapper.context).uri || wrapper.context_uri || track.contextUri) || null,
    playedAt: text(wrapper.played_at || track.playedAt) || null
  };
}

export function normalizePlaylist(input) {
  if (!input || typeof input !== "object") return null;
  const playlist = object(input);
  if (!Object.keys(playlist).length) return null;
  const tracks = object(playlist.tracks);
  const items = object(playlist.items);
  return {
    id: text(playlist.id || playlist.uri),
    uri: text(playlist.uri) || null,
    name: text(playlist.name, "Untitled playlist"),
    description: text(playlist.description),
    artworkUrl: firstImage(playlist.images) || text(playlist.artworkUrl) || null,
    ownerName: text(object(playlist.owner).display_name || object(playlist.owner).id),
    trackCount: Math.max(0, number(tracks.total ?? items.total ?? playlist.track_count)),
    isPublic: typeof playlist.public === "boolean" ? playlist.public : null,
    collaborative: Boolean(playlist.collaborative)
  };
}

export function normalizePlayback(input) {
  const playback = object(input);
  const rawTrack = playback.item || playback.current_track || (
    playback.title || playback.name ? {
      id: playback.uri,
      uri: playback.uri,
      title: playback.title,
      artist: playback.artist,
      album: playback.album,
      cover_art: playback.cover_art,
      duration_ms: playback.duration_ms
    } : null
  );
  const currentTrack = normalizeTrack(rawTrack);
  const repeat = text(playback.repeat_state || playback.repeatMode, "off");
  const device = object(playback.device);
  return {
    currentTrack,
    isPlaying: playback.is_playing === true || playback.status === "playing",
    progressMs: Math.max(0, number(playback.progress_ms ?? playback.progressMs)),
    durationMs: currentTrack?.durationMs || Math.max(0, number(playback.duration_ms ?? playback.durationMs)),
    volumePercent: device.volume_percent == null && playback.volume_percent == null
      ? null
      : Math.min(100, Math.max(0, number(device.volume_percent ?? playback.volume_percent))),
    shuffle: Boolean(playback.shuffle_state ?? playback.shuffle),
    repeatMode: ["off", "context", "track"].includes(repeat) ? repeat : "off",
    activeDeviceId: text(device.id || playback.activeDeviceId) || null,
    activeDeviceName: text(device.name || playback.activeDeviceName) || null,
    source: ["spotify", "local"].includes(text(playback.source)) ? text(playback.source) : "unknown"
  };
}

function collection(payload, key) {
  const source = object(payload);
  const nested = object(source[key]);
  if (Array.isArray(nested.items)) return nested.items;
  if (Array.isArray(source.items)) return source.items;
  return [];
}

export function normalizeTrackCollection(payload, key = "tracks") {
  return collection(payload, key).map(normalizeTrack).filter(Boolean);
}

export function normalizePlaylistCollection(payload) {
  return collection(payload, "playlists").map(normalizePlaylist).filter(Boolean);
}

export function normalizeQueue(payload) {
  const source = object(payload);
  return {
    currentTrack: normalizeTrack(source.currently_playing || source.current_track),
    upcoming: (Array.isArray(source.queue) ? source.queue : []).map(normalizeTrack).filter(Boolean)
  };
}

export function normalizeSearch(payload) {
  const source = object(payload);
  return {
    tracks: normalizeTrackCollection(source, "tracks"),
    playlists: normalizePlaylistCollection(source),
    albums: collection(source, "albums").filter((item) => item && typeof item === "object"),
    artists: collection(source, "artists").filter((item) => item && typeof item === "object")
  };
}

export const EMPTY_PLAYBACK = Object.freeze(normalizePlayback({}));
