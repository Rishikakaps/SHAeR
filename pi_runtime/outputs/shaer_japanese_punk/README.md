# SHAeR Japanese Punk Theme

Japanese Punk is a separate UI world, based on the black / hot-pink zine references. It starts with the first navigation page: Local Music, Spotify Connect, Voice Memos, and Settings.

## Run

```bash
python3 -m http.server 8775 --bind 127.0.0.1
```

Open:

```text
http://127.0.0.1:8775/shaer_japanese_punk/
```

## OS / Hardware Mode

```text
http://127.0.0.1:8775/shaer_japanese_punk/?mode=device
```

Clean boot:

```text
http://127.0.0.1:8775/shaer_japanese_punk/?mode=device&reset=1
```

Controls:

- `ArrowDown` / `ArrowRight`: next selectable element
- `ArrowUp` / `ArrowLeft`: previous selectable element
- `Enter` / `Space`: activate
- `Escape` / `Backspace`: back
