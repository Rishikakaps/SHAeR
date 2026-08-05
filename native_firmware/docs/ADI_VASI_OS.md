# आदि Vasi OS

Status: V1 operating-environment contract.

`आदि Vasi OS` is the name of the SHAeR embedded operating environment. It runs on top of Raspberry Pi OS Lite, but Linux must remain invisible to the user.

## User Contract

From power applied to shutdown complete:

- No kernel console.
- No login prompt.
- No desktop.
- No mouse cursor.
- No Raspberry Pi branding.
- No terminal.
- Every visible pixel belongs to आदि Vasi OS.

## Boot Contract

```text
Power button
-> Linux boots silently
-> systemd launches आदि Vasi OS
-> framebuffer/display is claimed
-> Libra constellation animation
-> Loading D: Drive...
-> mhm mhm
-> hardware init runs behind animation
-> Home
```

Cold boot always starts at Home. Cold boot never restores Now Playing.

Home options:

1. Spotify Connect
2. Local Library
3. Voice Archive

## Linux Role

Linux is only:

- Driver provider.
- Filesystem provider.
- Process supervisor.
- Network/Bluetooth/audio substrate.

Linux is not a visible product surface.

## systemd Role

`shaer-app.service` launches the foreground OS shell and restarts it if it crashes. If repeated crash-loop starts occur, firmware Safe Mode takes over.

## Shutdown Contract

```text
Long press
-> graceful shutdown animation
-> flush SQLite
-> save playback state
-> save logs
-> stop services
-> disarm watchdog
-> Linux shutdown
-> power off
```

## Sleep Contract

Sleep keeps Linux alive, stops renderer/audio, turns the display off, and wakes instantly into the previous sleep context. Cold boot is different and returns to Home.

