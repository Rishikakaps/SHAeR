# SHAeR Companion

Run:

```bash
python3 run_companion.py
```

If your Python build does not include Tkinter, the same command automatically opens a local web interface instead.

You can also force the browser interface:

```bash
python3 run_companion.py --ui web
```

Command-line import and sync:

```bash
python3 run_companion.py --no-ui --import-folder /path/to/music
python3 run_companion.py --no-ui --sync-device /Volumes/SHAER_SD
```

The app uses Python stdlib only: Tkinter for the native desktop UI when available, a local browser UI as fallback, and SQLite for local databases.
