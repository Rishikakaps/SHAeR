# SHAeR Windows XP Theme

Windows XP is a separate UI world with the permanent Bliss-style backdrop image, XP menus, windows, taskbar, media screens, full settings list, and black SHAeR boot screen.

## Run

```bash
python3 -m http.server 8775 --bind 127.0.0.1
```

Open:

```text
http://127.0.0.1:8775/shaer_windows_xp/
```

## OS / Hardware Mode

```text
http://127.0.0.1:8775/shaer_windows_xp/?mode=device
```

Clean boot:

```text
http://127.0.0.1:8775/shaer_windows_xp/?mode=device&reset=1
```

Controls:

- `ArrowDown` / `ArrowRight`: next selectable element
- `ArrowUp` / `ArrowLeft`: previous selectable element
- `Enter` / `Space`: activate
- `Escape` / `Backspace`: back
