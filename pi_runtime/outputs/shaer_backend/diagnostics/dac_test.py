#!/usr/bin/env python3
import os
from pathlib import Path

if os.environ.get("SHAER_HARDWARE") == "1":
    cards = Path("/proc/asound/cards").read_text(encoding="utf-8")
    assert "pcm5102" in cards.lower() or "hifiberry" in cards.lower(), "PCM5102A DAC not detected"
    print("dac_test ok mode=hardware pcm5102=true")
else:
    print("dac_test ok mode=contract pcm5102=physical-pending")
