# SHAeR Theme Package Manifest V1

Status: Required before hardware testing.

Each theme lives under:

```text
assets/themes/<theme_id>/
```

Required files:

| File/Folder | Purpose |
|---|---|
| `theme.json` | Versioned manifest |
| `theme.properties` | Firmware-readable fallback profile |
| `fonts/` | Optional fonts |
| `icons/` | Optional icon pack |
| `animations/` | Optional animation descriptors |
| `sounds/` | Optional UI sounds |
| `layouts/` | Optional layout descriptors |
| `colors/` | Optional palette descriptors |

## Manifest Fields

| Field | Meaning |
|---|---|
| `theme_api` | Must be `1` for V1 |
| `id` | Stable theme id |
| `display_name` | User-facing name |
| `palette.background` | Main background |
| `palette.accent` | Primary highlight |
| `palette.secondary` | Secondary highlight |
| `motion.default_fps` | Theme FPS before power caps |
| `motion.power_cost` | `low`, `medium`, or `high` |

## Validation

The theme validator must check:

- Manifest exists.
- ID matches folder.
- ThemeAPI version is supported.
- Required palette colors exist.
- Referenced fonts/icons/animations exist.
- Animation FPS does not exceed firmware policy.

