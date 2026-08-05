# SHAeR Desktop Preview

This is a browser rendering backend for the native SHAeR firmware UI. It is not a second web application: the native preview process instantiates `ShaerApp` and `UiFramework`, serializes the resulting `UiFrame`, and the browser draws those commands on an exact 240x320 canvas.

## Run

From the repository root:

```bash
npm install
npm run dev
```

Open [http://localhost:3000](http://localhost:3000).

`npm run dev` builds `native_firmware/build/shaer_preview_backend` when needed. After firmware UI changes, restart the dev server so the native backend is rebuilt and the browser receives the new frame definitions.

## Boundaries

- Native `ShaerApp` owns state and semantic input.
- Native `UiFramework` owns the screen/widget drawing commands.
- `desktop_preview/public/app.js` only interprets `UiCommand` values.
- The browser must not add screen-specific layout or playback behavior.
- Theme screens are assembled from shared `Rect`, `Text`, `Icon`, `Progress`,
  and `Transition` primitives. The former full-screen mockup bitmaps are design
  references only and are not emitted by the runtime renderer.
