# Recording Recovery

## Normal Finalization

1. Capture begins in `*.wav.partial`.
2. The journal records paths, UUID, timestamp, theme, and state.
3. Stop sends SIGINT to GStreamer so `wavenc` writes the WAV header.
4. The partial file is atomically renamed to `.wav`.
5. WAV duration is read from the finalized header.
6. Sidecar and SQLite row are written.
7. The recovery journal is removed.

## Interrupted Capture

At startup, `RecordingService` scans for `*.recording.json`:

- A valid WAV partial is renamed to the final path, indexed as `recovered`, and given a recovery note.
- An invalid or incomplete partial remains available and is indexed as `recoverable` for export or later repair.
- The journal is retained only when recovery itself cannot be completed safely.

## Low Storage and Maximum Duration

A background monitor finalizes active capture when either limit is reached. The stop reason is preserved for diagnostics. Failure to finalize leaves the recovery journal in place for the next boot.

## Low Battery Shutdown

The power service must invoke the same recorder stop contract before system shutdown. Validation of the final power-service integration is `PENDING` on Raspberry Pi hardware.

## Physical Test

1. Start a recording and speak for at least 15 seconds.
2. Remove power without pressing Stop.
3. Restore power and wait for the boot sequence.
4. Confirm the item appears as `recovered` or `recoverable`.
5. Play/export the item and inspect its WAV header.

This destructive test must be run on a disposable recording before hardware acceptance.
