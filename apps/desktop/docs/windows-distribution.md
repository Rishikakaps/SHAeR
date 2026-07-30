# Windows Distribution

Windows is the primary recipient build for SHAeR Desktop Companion.

The Windows deliverable is not an APK. The equivalent user-installable files are:

- `.msi` installer
- `.exe` installer from NSIS

## Why It Is Not Built On This Mac

Tauri can compile the macOS `.app` on macOS, but the real Windows installer should be built on Windows. The repository now includes a GitHub Actions workflow at:

```text
.github/workflows/desktop-windows-release.yml
```

That workflow uses `windows-latest`, installs Node and Rust, runs typecheck/tests/build, then creates Windows `.msi` and `.exe` installer artifacts.

## Manual Windows Build

On a Windows machine:

```powershell
cd apps/desktop
npm ci --no-audit --no-fund
npm run typecheck
npm run test
npm run build
npm run tauri:build:windows
```

Expected artifacts:

```text
apps/desktop/src-tauri/target/release/bundle/msi/*.msi
apps/desktop/src-tauri/target/release/bundle/nsis/*.exe
```

## QR Download Page

A phone cannot install a Windows app like an APK. The QR code should point to a hosted `.msi` or `.exe` download URL.

Template:

```text
apps/desktop/release/download.html
```

Publish that HTML page somewhere, then add the Windows installer URL:

```text
download.html?url=https://example.com/SHAeR-Desktop-Companion-0.1.0-x64-setup.exe
```

The page shows a direct download button and QR code for the same installer URL.
