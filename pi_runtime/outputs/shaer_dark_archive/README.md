# SHAeR Dark Archive Exact Pass

This folder is the focused Dark Archive implementation pass. It intentionally does not include Bombay Local, XP, Japanese Punk, Ghibli, or Raga yet.

## Run

```bash
python3 -m http.server 8770
```

Open:

```text
http://127.0.0.1:8770/
```

## OS / Hardware Mode

When running from the shared `outputs` preview server:

```text
http://127.0.0.1:8775/shaer_dark_archive/?mode=device
```

Clean boot:

```text
http://127.0.0.1:8775/shaer_dark_archive/?mode=device&reset=1
```

Controls:

- `ArrowDown` / `ArrowRight`: next selectable element
- `ArrowUp` / `ArrowLeft`: previous selectable element
- `Enter` / `Space`: activate
- `Escape` / `Backspace`: back

## Implemented Screens

- Boot constellation
- Boot loading identity
- Home
- Loading card
- Library list
- Library folders
- Now Playing
- Memos
- Settings
- ASCII Home
- Animated Charging
- Usable OS-style navigation hotspots

## Rule For Next Work

Do not begin Bombay Local until Dark Archive is visually accepted.
