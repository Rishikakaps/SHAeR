"""SHAeR voice recording and personal archive subsystem."""

from .archive import RecordingArchive, RecordingError
from .service import (
    CaptureBackend,
    GStreamerCaptureBackend,
    RecordingService,
    RecordingState,
    SyntheticCaptureBackend,
)

__all__ = [
    "CaptureBackend",
    "GStreamerCaptureBackend",
    "RecordingArchive",
    "RecordingError",
    "RecordingService",
    "RecordingState",
    "SyntheticCaptureBackend",
]
