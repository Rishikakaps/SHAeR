# Voice Recording Architecture

## Scope

Layer 15 adds a voice recorder and personal archive without changing the music, Spotify, theme, navigation, settings, or companion protocols. Recording data is intentionally separate from the music library.

## Components

- `shaer_recording.service.RecordingService` owns the state machine: `idle`, `recording`, `paused`, `finalizing`, and `error`.
- `GStreamerCaptureBackend` captures mono 48 kHz, signed 16-bit PCM from the ALSA microphone device and writes a WAV stream.
- `SyntheticCaptureBackend` is allowed only in automated tests and contract diagnostics.
- `shaer_recording.archive.RecordingArchive` stores searchable metadata in a dedicated SQLite database and mirrors each row to a JSON sidecar.
- `shaer_pi_os/server.py` exposes loopback-only recorder controls and authenticated companion archive endpoints.
- `hardware-bridge.js` maps OK to start/pause/resume, long OK to finish, and Back to a discard confirmation.

## Capture Pipeline

```text
INMP441 / ALSA
  -> GStreamer alsasrc
  -> audioconvert
  -> audioresample
  -> S16LE mono 48 kHz
  -> level meter
  -> wavenc
  -> .wav.partial
  -> atomic rename to .wav
```

The production backend requires `gst-launch-1.0`. The ALSA input defaults to `hw:MIC,0` and can be overridden with `SHAER_MIC_DEVICE`.

## Audio Ownership

Starting a recording is rejected while playback reports `playing` or `buffering`. This prevents simultaneous ownership of incompatible I2S capture and playback modes. Recorded WAV files are streamed to the source-neutral Now Playing UI, which retains the same play, pause, previous, next, progress, and metadata widgets used by music.

## Hardware Controls

- Rotate: move through visible controls or archive rows.
- OK: start, pause, or resume.
- Long OK: safely finalize and save.
- Back during capture: open Keep/Discard confirmation.
- Back outside capture: return to the previous page.

No keyboard is required on the device.

## Environment

- `SHAER_RECORDINGS_DIR`: archive root.
- `SHAER_RECORDINGS_DB`: recording SQLite database.
- `SHAER_MIC_DEVICE`: ALSA capture device.
- `SHAER_RECORDING_MAX_SECONDS`: maximum recording duration.
- `SHAER_RECORDING_MIN_FREE_BYTES`: required free space.
- `SHAER_RECORDING_TEST_MODE=1`: synthetic test capture only; never enable on the deployed daily-driver service.

## Hardware Status

Microphone capture, PCM5102 DAC playback, and forced power-loss recovery remain `PENDING` until run on the Raspberry Pi with the final microphone and DAC wiring.
