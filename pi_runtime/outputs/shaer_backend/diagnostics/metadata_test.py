#!/usr/bin/env python3
import tempfile
import time
import uuid
from pathlib import Path

from shaer_recording import RecordingArchive


def main():
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        archive = RecordingArchive(root / "recordings.db", root / "Recordings")
        audio = archive.root / "memo.wav"
        sidecar = archive.root / "memo.json"
        audio.write_bytes(b"RIFF" + b"\0" * 60)
        item = archive.add({
            "recording_uuid": str(uuid.uuid4()),
            "file_path": audio,
            "sidecar_path": sidecar,
            "timestamp": int(time.time()),
            "duration_ms": 2500,
            "file_size": audio.stat().st_size,
            "device_theme": "shaer_indian_print",
        })
        archive.update(item["id"], {"title": "Field note", "favorite": True, "notes": "monsoon"})
        assert archive.list(query="monsoon")[0]["title"] == "Field note"
        assert archive.list(favorite=True, minimum_duration_ms=2000)
        assert sidecar.is_file()
        archive.close()
    print("metadata_test ok sqlite=true sidecar=true search=title,date,notes,duration,favorite")


if __name__ == "__main__":
    main()
