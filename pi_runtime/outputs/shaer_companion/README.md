# SHAeR Companion

## Bluetooth-assisted setup

Android 0.18.0 and later discovers nearby SHAeR devices using Bluetooth Low Energy. SHAeR advertises only its current private LAN address and API port; credentials, Spotify tokens, music, and controls are not broadcast over Bluetooth.

After discovery, the companion uses the local Wi-Fi API and the existing physical-confirmation pairing flow. The user must still approve the displayed code with SHAeR's OK button. The resulting revocable credential is stored in Android's encrypted credential vault.

Install the Pi discovery service with:

```bash
cd ~/shaer/outputs/shaer_pi_os
chmod +x shaer_ble_discovery.py install_layer14.sh
./install_layer14.sh
sudo systemctl status shaer-ble-discovery --no-pager
```

Bluetooth discovery does not expose SHAeR to the public internet and does not replace Wi-Fi for artwork, playback, library, or settings traffic.

One shared companion application now powers three delivery surfaces:

- Android: Capacitor project and installable APK.
- Windows: installable PWA package using the same application shell.
- Browser: fallback served directly by `shaer-pi-os` at `/companion`.

The app uses real SHAeR and Spotify responses. It does not replace failed requests with demo tracks.

## Shared Architecture

`src/core/` is platform-neutral:

- `models.js`: null-safe SHAeR track, playlist, playback, queue, and search normalisers.
- `api-client.js`: one base-URL and bearer-auth strategy with structured network, HTTP, parse, cancellation, and authentication errors.
- `music-store.js`: one observable source of truth for Spotify status, playback, queue, saved tracks, playlists, recent tracks, and search.
- `credential-vault.js`: Android native vault when available; non-extractable WebCrypto key plus IndexedDB ciphertext in capable browsers; session-only fallback otherwise.
- `discovery.js`: remembered device, same-origin, `shaer.local`, and Android NSD discovery with manual-address fallback.

The Android-only code is limited to Android Keystore credential storage and NSD/mDNS discovery under `android/app/src/main/java/in/shaer/companion/`.

## Browser Fallback

On SHAeR:

```bash
cd /home/aditya/shaer/outputs
python3 shaer_pi_os/server.py --host 0.0.0.0 --port 8775 --theme auto
```

Open `http://shaer.local:8775/companion`, request pairing, compare the six-digit code, and approve with SHAeR's physical OK button. Back denies the request.

## Shared Tests

```bash
cd outputs/shaer_companion
npm test
```

## Browser And Windows PWA Package

```bash
npm ci
npm run package:pwa
```

Artifact:

`releases/shaer-companion-pwa-0.17.0.zip`

For Windows acceptance, serve the extracted package from an HTTPS or localhost origin, install it through Microsoft Edge, then test discovery, pairing, reconnection, controls, revocation, restart, and removal on a real Windows laptop. The Pi-hosted browser fallback remains available without installing the PWA.

## Android APK

Requirements:

- Node.js 22 or newer
- JDK 21
- Android SDK Platform 36 and Build Tools 36

Build:

```bash
npm ci
npm run android:apk
```

Artifact:

`releases/SHAeR-Companion-0.17.0-debug.apk`

Install on a connected Android device:

```bash
adb install -r releases/SHAeR-Companion-0.17.0-debug.apk
```

The debug APK is signed with the standard Android debug certificate. It is suitable for installation and physical acceptance testing, not public distribution. A private release key, signing policy, Play/enterprise distribution choice, and upgrade-key custody process remain release gates.

## Security Boundaries

- Pairing still requires a one-use physical capability from SHAeR's OK button.
- Trusted tokens are hashed on SHAeR and immediately revocable.
- Android credentials are encrypted using an AES-GCM key generated inside Android Keystore.
- Browser credentials are not stored as plaintext in `localStorage`.
- Spotify tokens remain on SHAeR and are never returned to the companion.
- API and service-worker caches never persist protected API responses.
- Remote internet exposure is not supported.

The current Pi server uses local HTTP. Do not market local transport as encrypted until device TLS and certificate trust/pinning are implemented and physically validated.

## Product Status

See `COMPANION_PRODUCT_STATUS.md`. A compiled artifact is not the same as physical acceptance: Android and Windows completion remain pending until installation and end-to-end tests pass on real devices with the physical SHAeR unit.
