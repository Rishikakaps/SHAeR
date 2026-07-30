#!/usr/bin/env python3
import tempfile
import wave
from pathlib import Path

from shaer_recording import RecordingArchive, RecordingService, SyntheticCaptureBackend


def main():
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        archive = RecordingArchive(root / "recordings.db", root / "Recordings")
        service = RecordingService(archive, lambda: SyntheticCaptureBackend(seconds=0.08), minimum_free_bytes=1)
        assert service.start("shaer_dark_archive")["state"] == "recording"
        assert service.pause()["state"] == "paused"
        assert service.resume()["state"] == "recording"
        item = service.stop()
        with wave.open(item["file_path"], "rb") as audio:
            assert audio.getnframes() > 0
            assert audio.getnchannels() == 1
        assert Path(item["sidecar_path"]).is_file()
        archive.close()
    print("recording_test ok lifecycle=start,pause,resume,stop sidecar=true hardware=PENDING")


if __name__ == "__main__":
    main()
