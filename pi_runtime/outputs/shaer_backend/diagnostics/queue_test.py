#!/usr/bin/env python3
from pathlib import Path
import tempfile

from shaer_music import PlaybackQueue, QueueRepository, RepeatMode


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        queue = PlaybackQueue([1, 2, 3], repeat_mode=RepeatMode.ALL)
        queue.next_track_id()
        store = QueueRepository(Path(tmp) / "last_queue_state.json")
        store.save(queue)
        restored = store.load()
    print(f"queue_test ok current={restored.current()} tracks={restored.track_ids}")


if __name__ == "__main__":
    main()
