# Source Provenance

Unified on 13 July 2026.

## Native firmware snapshot

Source:

`/Users/rishika/Documents/Codex/2026-06-26/thi/work/shaer`

Git HEAD at copy time:

`4dbfb2b fix(display): add root TFT bringup probe`

The working tree contained substantial tracked and untracked Layer 3, audio, renderer, theme, and settings work. The unified copy captures the working-tree files, not merely HEAD.

Excluded as generated or machine-local:

- nested `.git/`
- `build/`, `releases/`, runtime `data/`, and `logs/`
- SQLite/WAL/rollback files
- Python caches

The original folder was not modified.

## Pi runtime snapshot

Source:

`/Users/rishika/Documents/Codex/2026-07-04/co`

Firmware/runtime version at copy time:

`0.16.0`

Last standalone Pi bundle checksum before the unified copy:

`c22580b1b17833524103953a0144497002dd3d505256a315545c4d9920d0b0cb`

Excluded as generated:

- top-level build output
- pre-existing bundle/checksum, which `build_all.sh` recreates
- theme validation artifacts and Python caches
- the `shaer_unified/` destination itself

The source files remain available in the original dated workspace, but future implementation should occur only in the unified workspace.
