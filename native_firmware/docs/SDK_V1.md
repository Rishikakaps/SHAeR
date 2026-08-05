# SHAeR SDK V1

This document describes the interfaces that exist in the current firmware. It is an implementation contract, not a promise of future behavior.

## Compatibility

| Field | Current value |
|---|---|
| SDK version | `1` |
| Firmware line | `0.1.0-alpha.1` |
| Minimum compatible firmware | `0.1.0-alpha.1` |
| Maximum tested firmware | `0.1.0-alpha.1` |
| Layout profile | `240x320-safe-grid` |

## Theme Registration

`shaer::ThemeRegistry::register_theme(ThemeDefinition)` is the registration boundary. It returns `true` only when the package manifest is complete and targets SDK V1. `ThemeRegistry::validate()` returns a `ThemeValidationResult` containing a stable boolean and human-readable field errors.

Theme ownership is limited to presentation metadata: palette, typography, spacing, icons, assets, animation preferences, and screen blueprints. Themes do not receive storage, playback, Spotify, Bluetooth, recorder, or queue services.

The manifest fields represented by `ThemeManifest` are:

- `theme_version`
- `sdk_version`
- `author`
- `description`
- `preview_image`
- `layout_profile`
- `font_family`
- `icon_pack`
- `animation_profile`
- `supported_features`
- `mandatory_screens`

## Screen Registration

`shaer::ScreenManager::registrations()` is the single read-only registry for screen identifiers, display names, layout profiles, widget composition, and navigation targets. Application behavior remains in the firmware app/state layers; registration contains metadata only.

## Rendering Boundary

`ThemeEngine::render_profile()`, `screen_blueprint()`, `transition_plan()`, and `animation_policy()` return immutable view data. `UiFramework` consumes `RenderModel` and emits drawing commands. The renderer does not modify playback, navigation, storage, or service state.

## Asset Boundary

`ThemeAssets` resolves theme-relative asset identifiers and falls back to the reference asset root. Callers should pass identifiers such as `icons/pen.png`, not storage paths. Full manifest JSON parsing and asset-reference validation remain planned; the current package loader reads `theme.properties` for firmware-safe fallback values.

## Extension Status

Implemented: theme registration validation, screen registration metadata, semantic render profiles, theme-relative asset resolution, and versioned manifest data.

Planned: third-party widget registration, new layout-profile registration, animation registration, and optional-service lifecycle hooks. These must be added to core and documented before extensions depend on them.
