# For Adi: How SHAeR Was Built

SHAeR started as a simple idea: music should feel physical again.

Not just a playlist on a screen, and not only a speaker. We wanted a small companion device that could sit in a room, hold local music, connect to Spotify, record voice notes, remember annotations, and feel like an object with its own personality.

The working basis became this:

```text
hardware object
-> Raspberry Pi runtime
-> local API
-> phone companion
-> desktop companion
-> release/download flow
```

## 1. The Hardware First

We treated SHAeR as a device before treating it as an app.

The hardware side is built around a Raspberry Pi, audio output, display rendering, controls, storage, and future physical checks for battery, DAC, microphone, Bluetooth, Wi-Fi, and buttons. The native firmware and renderer define how SHAeR looks and behaves on its own screen.

The key decision was: the device should not depend on a cloud dashboard to be useful. It should run locally, expose its own API, and keep music and memory close to the hardware.

That is why the repo has:

```text
native_firmware/
pi_runtime/
```

The firmware owns the object feel. The Pi runtime owns the live device behavior.

## 2. The Pi Became The Center

The Raspberry Pi runtime became the bridge between physical SHAeR and every companion app.

It provides local endpoints for:

- dashboard state
- playback
- local music
- Spotify status
- settings
- themes
- recordings
- archive/marginalia
- Wi-Fi and Bluetooth inspection
- pairing and linked devices
- branding and app icon sync

The important design choice was to make every app read from the same SHAeR device API. Phone and desktop should not become two separate products with two separate truths. They are different windows into the same local device.

## 3. The Phone Companion

The phone app became the everyday controller.

It is built as a web/PWA companion with Android packaging through Capacitor. It discovers SHAeR, pairs securely, shows now-playing state, browses music, handles Spotify views, exposes recordings and marginalia, edits settings, and gives access to diagnostics, updates, backup, and linked devices.

The phone layout is compact and thumb-friendly:

```text
top status bar
main content
bottom navigation on small screens
```

This made the phone version feel like the remote control and pocket companion for SHAeR.

## 4. The Desktop Companion

The desktop app started as a narrower control panel, then was expanded to match the phone app's functionality.

It now uses the same major sections:

```text
Device
Music
Marginalia
Recordings
Themes
Settings
Linked devices
Diagnostics
Updates
Backup
```

The desktop version is adjusted for a larger screen: persistent sidebar, wider library tables, larger now-playing panel, visible connectivity inspection, and a settings surface that feels more like a traditional music-management app.

The point is not for desktop to replace the phone. The phone is quick and portable. The desktop is better for inspection, setup, files, releases, diagnostics, and deeper management.

## 5. The UI Direction

The current visual direction is early-2000s Apple music software: glossy chrome, compact panels, library rows, rounded player controls, and a clear selected state.

We moved both phone and desktop toward the same visual language:

- light brushed-grey surfaces
- pink selected states
- glossy controls
- compact music-library tables
- sidebar navigation
- red vinyl SHAeR icon as the base identity

The goal is nostalgic without becoming fake. It should feel like a music utility from the era when software still looked touchable, but it still needs to work as a modern local-first device companion.

## 6. The App Icon And Branding

The red vinyl SHAeR Companion image is now the base icon.

One icon source feeds:

- desktop PNG icons
- macOS `.icns`
- Windows `.ico`
- Android launcher icons
- phone/PWA icons
- in-app synced branding

This matters because SHAeR should look like the same product everywhere: phone, desktop, download page, and installed launcher.

## 7. The Download Flow

The repo includes a download page for phone and desktop versions.

The intended flow is:

```text
GitHub repo
-> GitHub Actions builds desktop installers
-> APK/PWA/runtime artifacts are attached to releases
-> downloads page links to those files
-> QR code can point people to the installer/download page
```

Android uses APK/PWA-style distribution. Windows uses `.exe` or `.msi`, not APK. The Windows installer is built by GitHub Actions on a Windows runner.

## 8. What Is Verified

We are careful about claims.

Host-side software checks can verify code paths, UI builds, route behavior, and packaging scripts. They do not prove the physical hardware works in the real world.

So the honest status is:

- source/docs/UI integration are in place
- phone companion tests pass
- phone web build has passed
- desktop source has been updated to match the phone structure
- Windows build workflow exists
- physical device acceptance still needs real SHAeR hardware testing

That boundary matters. SHAeR is a hardware-adjacent product, so real acceptance has to happen on the actual device, not only in a terminal.

## 9. The Product In One Line

SHAeR is a local-first music object with memory: built from hardware upward, controlled by phone and desktop companions, styled like a tactile music app, and designed to keep the experience personal, inspectable, and close to the device.

