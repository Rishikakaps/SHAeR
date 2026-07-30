# SHAeR Layer 12: Spotify on Raspberry Pi

## 1. Spotify developer setup

Create one app in the Spotify Developer Dashboard and register this exact redirect URI:

```text
http://127.0.0.1:8775/api/spotify/callback
```

SHAeR uses OAuth 2.0 Authorization Code with PKCE. It needs a client ID, but it
does not use or store a client secret.

## 2. Pi dependencies

```bash
sudo apt update
sudo apt install -y python3-gpiozero python3-lgpio python3-qrcode python3-pil chromium-browser alsa-utils
```

Install a Raspberry Pi build of `librespot` at `/usr/local/bin/librespot`, then
confirm it runs:

```bash
/usr/local/bin/librespot --version
```

Find the PCM5102 output:

```bash
aplay -L
```

## 3. Configure SHAeR

```bash
mkdir -p ~/.config/shaer
cp ~/shaer/outputs/shaer_pi_os/spotify.env.example ~/.config/shaer/spotify.env
nano ~/.config/shaer/spotify.env
chmod 600 ~/.config/shaer/spotify.env
```

Set `SPOTIFY_CLIENT_ID` and the correct `SHAER_ALSA_DEVICE`. Do not add a client
secret.

## 4. Install services

```bash
~/shaer/outputs/shaer_pi_os/install_layer12.sh
sudo systemctl restart shaer-librespot shaer-pi-os
sudo systemctl status shaer-librespot shaer-pi-os --no-pager
```

## 5. Log in once

From the Pi desktop:

```bash
python3 ~/shaer/outputs/shaer_pi_os/spotify_setup.py login --qr ~/spotify-login.png
```

The browser opens automatically. The token is stored at
`~/.config/shaer/spotify-token.json` with owner-only permissions and survives
reboots. The same login starts when the user selects Spotify Connect in a SHAeR
theme.

For a phone-scanned QR flow, Spotify requires the registered redirect to be
reachable from the phone. Plain HTTP is allowed only for loopback
`127.0.0.1`; a phone flow therefore needs a registered HTTPS callback routed to
the Pi. The on-Pi browser flow works with the default loopback URI.

## 6. Runtime checks

```bash
python3 ~/shaer/outputs/shaer_pi_os/spotify_setup.py status
curl -s http://127.0.0.1:8775/api/spotify/connect/status
curl -s http://127.0.0.1:8775/api/spotify/playback
```

Then open Spotify on the phone and select `SHAeR` from available devices.

## 7. Hardware acceptance test

1. Transfer playback from the phone to `SHAeR`.
2. Confirm PCM5102 audio.
3. Open Now Playing in each theme and confirm live metadata and progress.
4. Use the encoder to focus Previous, Play/Pause, and Next.
5. Press OK and confirm Spotify responds.
6. Press Back and confirm the previous SHAeR page returns.
7. Reboot and confirm login persists.
8. Disconnect Wi-Fi briefly and confirm both services recover.

Do not call Layer 12 hardware-complete until all eight checks pass on the Pi.
