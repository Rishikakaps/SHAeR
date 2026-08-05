# SHAeR Audio Architecture V1

Status: V1 freeze draft.

## Playback Paths

```text
Local MP3/FLAC/WAV
  -> Playback Queue Manager
  -> Decoder
  -> ReplayGain
  -> Crossfade/Gapless Scheduler
  -> ALSA/PipeWire
  -> I2S
  -> PCM5102A DAC
  -> 3.5 mm line output
  -> Headphones or powered input
```

```text
Spotify Connect
  -> Spotify service/player
  -> Playback Queue Snapshot
  -> ALSA/PipeWire
  -> I2S
  -> PCM5102A DAC
  -> 3.5 mm line output
```

```text
Bluetooth audio output
  -> BlueZ
  -> Paired headphones only
```

```text
MEMS microphone
  -> I2S capture
  -> Recorder
  -> WAV or MP3 encoder
  -> Storage
  -> Memory Mode metadata link
```

## Supported Formats

Fully supported local playback:

- MP3.
- FLAC.
- WAV.

Voice recording output:

- WAV for archival/raw capture.
- MP3 for compact sharing and long recordings.

## Sample Rates

Playback should preserve source sample rate when stable and affordable. V1 policy:

- 44.1 kHz: default music path.
- 48 kHz: supported for recordings/video-derived files.
- 88.2/96 kHz: optional Archive Quality mode if CPU/battery tests pass.
- 176.4/192 kHz and above: do not target for V1 battery mode, even if the DAC supports it.

The PCM5102A supports much higher sample rates, but SHAeR chooses the highest practical rate that does not compromise battery life, thermals, UI smoothness, or gapless playback.

## Bit Depth

- Internal decode path: 24-bit or 32-bit float where library support is clean.
- I2S output: 16/24/32-bit supported by DAC; V1 default should be 24-bit if ALSA path is stable.
- Recordings: 16-bit WAV default, optional higher mode later.

## Gapless Playback

Gapless local album playback is required. The Playback Queue Manager must pre-open and pre-buffer the next local track before the current track ends.

Crossfade is user configurable:

- Off.
- 3 seconds.
- 6 seconds.
- 12 seconds.

Crossfade must be disabled automatically for albums marked gapless unless the user explicitly overrides it.

## ReplayGain

ReplayGain is configurable in the sync/companion app and exposed on device:

- Off.
- Track.
- Album.

Clipping prevention is on by default. ReplayGain settings are stored in SQLite and mirrored in settings JSON for debug readability.

## Spotify Disconnect Behavior

If Wi-Fi disconnects mid-song:

1. Stop playback.
2. Log to `spotify.log` and `audio.log`.
3. Show calm popup.
4. OK opens Local Library.
5. Do not resume stale Spotify playback without an explicit fresh session.

## Bluetooth

Bluetooth V1 is headphones-only:

- Automatic reconnect to known headphones.
- No Bluetooth speaker hosting.
- No phone-to-SHAeR Bluetooth receiver mode in V1.
- Expected codecs: SBC required, AAC optional if stable, aptX/LDAC not required.

Bluetooth loss pauses/stops safely and offers output recovery.

## Memory Mode Audio

Voice memories can link to:

- Track.
- Album.
- Playlist.
- Spotify URI.
- Local library item.

Memory Mode is metadata, not a new hardware path. Recordings remain normal audio files with database links.

## Future DAC Upgrade

DAC upgrades must keep the same AudioOutput HAL and preferably the same I2S GPIOs. Candidate future upgrades:

- PCM5122.
- External headphone amp board.
- Balanced output board.

No UI or queue logic may depend on a DAC model.

