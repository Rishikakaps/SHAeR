# SHAeR Bombay Ticket Theme Pass

This is the focused Bombay Ticket implementation pass. It does not reuse the Dark Archive UI structure.

## Run

```bash
python3 -m http.server 8772
```

Open:

```text
http://127.0.0.1:8772/
```

## OS / Hardware Mode

Use this URL for the actual single-screen UI:

```text
http://127.0.0.1:8772/?mode=device
```

Use this for a clean boot during hardware tests:

```text
http://127.0.0.1:8772/?mode=device&reset=1
```

Control contract for GPIO / keyboard testing:

- `ArrowDown` / `ArrowRight`: next selectable UI element
- `ArrowUp` / `ArrowLeft`: previous selectable UI element
- `Enter` / `Space`: activate selected element
- `Escape` / `Backspace`: back

The app keeps a 240x320 render surface in device mode so hardware testing is pixel-faithful.

## Implemented Screens

- Home ticket menu
- Loading with blue circular mark and rail bar
- Library boxes
- Liked songs list
- Now Playing ticket stub
- Memos
- Settings / About
- Chai ticket card

## Rule

Perfect Bombay Ticket here before moving to the next theme.
