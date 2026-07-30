# App Icon And Branding Sync

SHAeR uses one source image for both phone and desktop app icons.

## Build-Time Installed App Icons

Installed app icons are baked into the app package. To change the phone APK icon or Windows/macOS desktop icon, update the icon source and rebuild the installers.

```bash
cd apps/desktop
npm run icon:update -- /absolute/path/to/icon.png
```

This regenerates:

- Tauri desktop icons under `apps/desktop/src-tauri/icons`
- PWA icons under `pi_runtime/outputs/shaer_companion/icons`
- Built PWA icons under `pi_runtime/outputs/shaer_companion/dist/icons`
- Android native launcher icon PNGs under `pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-*`
- Android web assets under `pi_runtime/outputs/shaer_companion/android/app/src/main/assets/public/icons`

Then rebuild:

```bash
npm run tauri:build:windows
```

and rebuild the Android companion APK from `pi_runtime/outputs/shaer_companion/android`.

## Runtime Synced Branding

The desktop app can upload a PNG icon to SHAeR through:

```text
POST /api/v1/branding/app-icon
GET /api/v1/branding
```

That runtime branding is shared by desktop and phone companion UIs as in-app branding. It does not replace already-installed OS launcher icons until the apps are rebuilt and reinstalled.
