"""Crash-aware voice recording state machine for the SHAeR INMP441 input."""

from __future__ import annotations

import json
import math
import os
import shutil
import signal
import subprocess
import tempfile
import threading
import time
import uuid
import wave
from abc import ABC, abstractmethod
from enum import Enum
from pathlib import Path
from typing import Any, Callable

from .archive import RecordingArchive, RecordingError


class RecordingState(str, Enum):
    IDLE = "idle"
    RECORDING = "recording"
    PAUSED = "paused"
    FINALIZING = "finalizing"
    ERROR = "error"


class CaptureBackend(ABC):
    @abstractmethod
    def start(self, target: Path) -> None: ...

    @abstractmethod
    def pause(self) -> None: ...

    @abstractmethod
    def resume(self) -> None: ...

    @abstractmethod
    def stop(self) -> None: ...

    @abstractmethod
    def cancel(self) -> None: ...

    def level(self) -> float | None:
        return None


class GStreamerCaptureBackend(CaptureBackend):
    def __init__(self, device: str = "hw:MIC,0", sample_rate: int = 48000):
        self.device = device
        self.sample_rate = sample_rate
        self.process: subprocess.Popen[bytes] | None = None

    def start(self, target: Path) -> None:
        binary = shutil.which("gst-launch-1.0")
        if not binary:
            raise RecordingError("capture_unavailable", "GStreamer is not installed.")
        command = [
            binary, "-e", "alsasrc", f"device={self.device}", "!", "audioconvert", "!",
            "audioresample", "!", f"audio/x-raw,format=S16LE,channels=1,rate={self.sample_rate}",
            "!", "level", "interval=100000000", "post-messages=true", "!", "wavenc", "!",
            "filesink", f"location={target}",
        ]
        self.process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        time.sleep(0.12)
        if self.process.poll() is not None:
            message = self.process.stderr.read().decode("utf-8", errors="replace")[-1000:] if self.process.stderr else ""
            raise RecordingError("microphone_unavailable", message or "Microphone capture failed to start.")

    def pause(self) -> None:
        self._signal(signal.SIGSTOP)

    def resume(self) -> None:
        self._signal(signal.SIGCONT)

    def stop(self) -> None:
        process = self._active()
        process.send_signal(signal.SIGINT)
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.terminate()
            process.wait(timeout=3)
        self.process = None

    def cancel(self) -> None:
        if self.process and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.process.kill()
        self.process = None

    def _signal(self, value: signal.Signals) -> None:
        self._active().send_signal(value)

    def _active(self) -> subprocess.Popen[bytes]:
        if not self.process or self.process.poll() is not None:
            raise RecordingError("capture_not_running", "Microphone capture process is not running.")
        return self.process


class SyntheticCaptureBackend(CaptureBackend):
    """Deterministic WAV source used only by tests and contract diagnostics."""

    def __init__(self, seconds: float = 0.25, sample_rate: int = 8000):
        self.seconds = seconds
        self.sample_rate = sample_rate
        self.target: Path | None = None
        self.running = False

    def start(self, target: Path) -> None:
        self.target = target
        self.running = True

    def pause(self) -> None:
        if not self.running:
            raise RecordingError("capture_not_running", "Synthetic capture is not running.")

    def resume(self) -> None:
        if not self.running:
            raise RecordingError("capture_not_running", "Synthetic capture is not running.")

    def stop(self) -> None:
        if not self.running or not self.target:
            raise RecordingError("capture_not_running", "Synthetic capture is not running.")
        frames = bytearray()
        for index in range(int(self.sample_rate * self.seconds)):
            sample = int(5000 * math.sin(2 * math.pi * 440 * index / self.sample_rate))
            frames.extend(sample.to_bytes(2, "little", signed=True))
        with wave.open(str(self.target), "wb") as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(self.sample_rate)
            output.writeframes(bytes(frames))
        self.running = False

    def cancel(self) -> None:
        self.running = False

    def level(self) -> float | None:
        return 0.62 if self.running else 0.0


class RecordingService:
    def __init__(
        self,
        archive: RecordingArchive,
        backend_factory: Callable[[], CaptureBackend] | None = None,
        max_duration_s: int = 3600,
        minimum_free_bytes: int = 128 * 1024 * 1024,
        clock: Callable[[], float] = time.time,
        monotonic: Callable[[], float] = time.monotonic,
    ):
        self.archive = archive
        self.backend_factory = backend_factory or (lambda: GStreamerCaptureBackend(os.environ.get("SHAER_MIC_DEVICE", "hw:MIC,0")))
        self.max_duration_s = max_duration_s
        self.minimum_free_bytes = minimum_free_bytes
        self.clock = clock
        self.monotonic = monotonic
        self.lock = threading.RLock()
        self.state = RecordingState.IDLE
        self.backend: CaptureBackend | None = None
        self.started_wall = 0.0
        self.started_mono = 0.0
        self.paused_started = 0.0
        self.paused_total = 0.0
        self.partial_path: Path | None = None
        self.final_path: Path | None = None
        self.sidecar_path: Path | None = None
        self.journal_path: Path | None = None
        self.recording_uuid: str | None = None
        self.theme = "shaer_dark_archive"
        self.last_error: str | None = None
        self.stop_reason: str | None = None
        self._monitor_stop = threading.Event()
        self.recover_interrupted()

    def start(self, theme: str, playback_active: bool = False) -> dict[str, Any]:
        with self.lock:
            if self.state is not RecordingState.IDLE:
                raise RecordingError("recording_busy", "A recording is already active.")
            if playback_active:
                raise RecordingError("audio_mode_conflict", "Pause playback before recording.")
            self._check_storage()
            now = self.clock()
            timestamp = time.localtime(now)
            folder = self.archive.root / time.strftime("%Y", timestamp) / time.strftime("%B", timestamp)
            folder.mkdir(parents=True, exist_ok=True)
            self.recording_uuid = str(uuid.uuid4())
            stem = time.strftime("%Y-%m-%d_%H-%M-%S", timestamp) + "_" + self.recording_uuid[:8]
            self.final_path = folder / f"{stem}.wav"
            self.partial_path = folder / f"{stem}.wav.partial"
            self.sidecar_path = folder / f"{stem}.json"
            self.journal_path = folder / f"{stem}.recording.json"
            self.theme = theme if theme.startswith("shaer_") else "shaer_dark_archive"
            self.started_wall = now
            self.started_mono = self.monotonic()
            self.paused_total = 0.0
            self.paused_started = 0.0
            self.last_error = None
            self.stop_reason = None
            self.backend = self.backend_factory()
            self._write_journal("starting")
            try:
                self.backend.start(self.partial_path)
            except Exception:
                self._clear_runtime(remove_partial=True)
                raise
            self.state = RecordingState.RECORDING
            self._write_journal("recording")
            self._start_monitor()
            return self.status()

    def pause(self) -> dict[str, Any]:
        with self.lock:
            self._require_state(RecordingState.RECORDING)
            assert self.backend
            self.backend.pause()
            self.paused_started = self.monotonic()
            self.state = RecordingState.PAUSED
            self._write_journal("paused")
            return self.status()

    def resume(self) -> dict[str, Any]:
        with self.lock:
            self._require_state(RecordingState.PAUSED)
            assert self.backend
            self.backend.resume()
            self.paused_total += max(0.0, self.monotonic() - self.paused_started)
            self.paused_started = 0.0
            self.state = RecordingState.RECORDING
            self._write_journal("recording")
            return self.status()

    def stop(self, reason: str = "user") -> dict[str, Any]:
        with self.lock:
            if self.state not in {RecordingState.RECORDING, RecordingState.PAUSED}:
                raise RecordingError("recording_not_active", "No recording is active.")
            if self.state is RecordingState.PAUSED:
                self.paused_total += max(0.0, self.monotonic() - self.paused_started)
                self.paused_started = 0.0
                assert self.backend
                self.backend.resume()
            self.state = RecordingState.FINALIZING
            self._write_journal("finalizing")
            assert self.backend and self.partial_path and self.final_path and self.sidecar_path and self.recording_uuid
            self.backend.stop()
            if not self.partial_path.exists() or self.partial_path.stat().st_size <= 44:
                self.state = RecordingState.ERROR
                self.last_error = "Capture produced no usable audio."
                raise RecordingError("empty_recording", self.last_error)
            os.replace(self.partial_path, self.final_path)
            valid_wav, wav_duration_ms = self._wav_details(self.final_path)
            duration_ms = wav_duration_ms if valid_wav else max(0, int(self._elapsed_s() * 1000))
            metadata = {
                "schema_version": 1,
                "recording_uuid": self.recording_uuid,
                "timestamp": int(self.started_wall),
                "duration_ms": duration_ms,
                "file_size": self.final_path.stat().st_size,
                "device_theme": self.theme,
                "title": None,
                "favorite": False,
                "playback_position_ms": 0,
                "sync_status": "local",
                "status": "complete",
                "notes": None,
                "archive_folder": None,
                "file_path": str(self.final_path),
                "sidecar_path": str(self.sidecar_path),
                "stop_reason": reason,
                "transcript": None,
                "transcript_source": None,
            }
            RecordingArchive._atomic_json(self.sidecar_path, metadata)
            item = self.archive.add(metadata)
            self.stop_reason = reason
            self._clear_runtime(remove_partial=False)
            return item

    def cancel(self) -> dict[str, Any]:
        with self.lock:
            if self.state is RecordingState.IDLE:
                return self.status()
            if self.backend:
                self.backend.cancel()
            self._clear_runtime(remove_partial=True)
            return self.status()

    def status(self) -> dict[str, Any]:
        with self.lock:
            elapsed = self._elapsed_s() if self.state is not RecordingState.IDLE else 0.0
            storage = self.archive.storage()
            return {
                "state": self.state.value,
                "elapsed_ms": int(elapsed * 1000),
                "max_duration_ms": self.max_duration_s * 1000,
                "storage_free": storage["free"],
                "storage_recordings": storage["recordings"],
                "level": self.backend.level() if self.backend else None,
                "recording_uuid": self.recording_uuid,
                "theme": self.theme,
                "last_error": self.last_error,
                "stop_reason": self.stop_reason,
            }

    def recover_interrupted(self) -> list[dict[str, Any]]:
        recovered: list[dict[str, Any]] = []
        for journal in self.archive.root.rglob("*.recording.json"):
            try:
                payload = json.loads(journal.read_text(encoding="utf-8"))
                partial = Path(str(payload["partial_path"]))
                final = Path(str(payload["final_path"]))
                sidecar = Path(str(payload["sidecar_path"]))
                valid, duration_ms = self._wav_details(partial)
                status = "recovered" if valid else "recoverable"
                file_path = final if valid else partial
                if valid:
                    os.replace(partial, final)
                metadata = {
                    "schema_version": 1,
                    "recording_uuid": payload["recording_uuid"],
                    "timestamp": int(payload["timestamp"]),
                    "duration_ms": duration_ms,
                    "file_size": file_path.stat().st_size if file_path.exists() else 0,
                    "device_theme": payload.get("device_theme", "shaer_dark_archive"),
                    "title": "Recovered recording",
                    "favorite": False,
                    "playback_position_ms": 0,
                    "sync_status": "local",
                    "status": status,
                    "notes": "Recovered after interrupted capture." if valid else "Partial audio requires repair or export.",
                    "archive_folder": None,
                    "file_path": str(file_path),
                    "sidecar_path": str(sidecar),
                }
                RecordingArchive._atomic_json(sidecar, metadata)
                recovered.append(self.archive.add(metadata))
                journal.unlink(missing_ok=True)
            except (OSError, ValueError, KeyError, json.JSONDecodeError):
                continue
        return recovered

    def _start_monitor(self) -> None:
        self._monitor_stop.clear()

        def monitor() -> None:
            while not self._monitor_stop.wait(0.5):
                try:
                    status = self.status()
                    if status["elapsed_ms"] >= self.max_duration_s * 1000:
                        self.stop("maximum_duration")
                        return
                    if status["storage_free"] < self.minimum_free_bytes:
                        self.stop("low_storage")
                        return
                except Exception as exc:
                    with self.lock:
                        self.last_error = str(exc)
                    return

        threading.Thread(target=monitor, name="shaer-recording-monitor", daemon=True).start()

    def _check_storage(self) -> None:
        free = shutil.disk_usage(self.archive.root).free
        if free < self.minimum_free_bytes:
            raise RecordingError("low_storage", "Not enough free storage to start a recording.")

    def _elapsed_s(self) -> float:
        if not self.started_mono:
            return 0.0
        now = self.paused_started if self.state is RecordingState.PAUSED and self.paused_started else self.monotonic()
        return max(0.0, now - self.started_mono - self.paused_total)

    def _write_journal(self, phase: str) -> None:
        if not all((self.journal_path, self.partial_path, self.final_path, self.sidecar_path, self.recording_uuid)):
            return
        RecordingArchive._atomic_json(self.journal_path, {
            "phase": phase,
            "recording_uuid": self.recording_uuid,
            "timestamp": int(self.started_wall),
            "device_theme": self.theme,
            "partial_path": str(self.partial_path),
            "final_path": str(self.final_path),
            "sidecar_path": str(self.sidecar_path),
        })

    def _clear_runtime(self, remove_partial: bool) -> None:
        self._monitor_stop.set()
        if remove_partial and self.partial_path:
            self.partial_path.unlink(missing_ok=True)
        if self.journal_path:
            self.journal_path.unlink(missing_ok=True)
        self.state = RecordingState.IDLE
        self.backend = None
        self.started_wall = 0.0
        self.started_mono = 0.0
        self.paused_started = 0.0
        self.paused_total = 0.0
        self.partial_path = None
        self.final_path = None
        self.sidecar_path = None
        self.journal_path = None
        self.recording_uuid = None

    def _require_state(self, expected: RecordingState) -> None:
        if self.state is not expected:
            raise RecordingError("invalid_recording_state", f"Recording must be {expected.value} for this action.")

    @staticmethod
    def _wav_details(path: Path) -> tuple[bool, int]:
        if not path.exists() or path.stat().st_size <= 44:
            return False, 0
        try:
            with wave.open(str(path), "rb") as source:
                frames = source.getnframes()
                rate = source.getframerate()
                return frames > 0 and rate > 0, int(frames / rate * 1000) if rate else 0
        except (wave.Error, OSError):
            return False, 0
