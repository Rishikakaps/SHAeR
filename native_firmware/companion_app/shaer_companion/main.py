from __future__ import annotations

import argparse
from pathlib import Path

from .app_core import CompanionAppCore


def default_root() -> Path:
    return Path.home() / ".shaer_companion"


def main() -> int:
    parser = argparse.ArgumentParser(description="SHAeR Companion")
    parser.add_argument("--data-root", type=Path, default=default_root())
    parser.add_argument("--import-folder", type=Path)
    parser.add_argument("--sync-device", type=Path)
    parser.add_argument("--no-ui", action="store_true")
    parser.add_argument("--ui", choices=["auto", "native", "web"], default="auto")
    parser.add_argument("--port", type=int, default=8782)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()

    core = CompanionAppCore(args.data_root)
    core.initialize()

    if args.import_folder:
        result = core.library.import_folder(args.import_folder)
        print(f"Imported {len(result.tracks)} tracks, {len(result.errors)} errors")
    if args.sync_device:
        plan = core.sync.execute(args.sync_device)
        print(f"Copied {len(plan.items)} files ({plan.total_bytes} bytes)")
    if args.no_ui:
        core.events.drain()
        return 0

    if args.ui == "web":
        from .web_ui import CompanionWebServer

        CompanionWebServer(core, port=args.port).serve(open_browser=not args.no_browser)
        return 0

    try:
        from .ui import CompanionWindow
    except ModuleNotFoundError as exc:
        if exc.name != "_tkinter":
            raise
        if args.ui == "native":
            raise RuntimeError(
                "Native UI requested, but this Python build does not include Tkinter. "
                "Run with --ui web or install a Python build with Tk support."
            ) from exc
        print("[SHAeR Companion] Tkinter is not available in this Python build.")
        print("[SHAeR Companion] Starting the local web interface instead.")
        from .web_ui import CompanionWebServer

        CompanionWebServer(core, port=args.port).serve(open_browser=not args.no_browser)
        return 0

    window = CompanionWindow(core)
    window.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
