# SHAeR Documentation

This directory collects the GitHub-facing guide layer for SHAeR.

## Product

- [For Adi: How SHAeR Was Built](for-adi.md)
- SHAeR is a local-first music, recording, annotation, and theme-driven audio device.
- The device runtime runs on Raspberry Pi.
- Phone and desktop companion apps connect to SHAeR over local network pairing.
- Desktop packaging uses Tauri 2.
- Phone packaging uses the existing Android/Capacitor companion app.

## Build And Install

- [Download Page](downloads.html)
- [Desktop App Guide](../apps/desktop/README.md)
- [Windows Distribution](../apps/desktop/docs/windows-distribution.md)
- [App Icon Sync](../apps/desktop/docs/app-icon-sync.md)
- [Native Firmware Build](../native_firmware/README.md)
- [Deployment](../native_firmware/docs/DEPLOYMENT.md)
- [Release Process](../native_firmware/docs/RELEASE_PROCESS.md)

## Architecture

- [System Architecture](../native_firmware/docs/SYSTEM_ARCHITECTURE.md)
- [Dependency Graph](../native_firmware/docs/DEPENDENCY_GRAPH.md)
- [State Machine](../native_firmware/docs/STATE_MACHINE.md)
- [Event Ownership](../native_firmware/docs/EVENT_OWNERSHIP.md)
- [Data Storage And Logging](../native_firmware/docs/DATA_STORAGE_AND_LOGGING.md)

## UI And Themes

- [Design Language](../native_firmware/docs/DESIGN_LANGUAGE.md)
- [Renderer Spec](../native_firmware/docs/RENDERER_SPEC.md)
- [Theme API](../native_firmware/docs/THEME_API_V1.md)
- [Theme Package Manifest](../native_firmware/docs/THEME_PACKAGE_MANIFEST.md)

## Hardware

- [Hardware Freeze](../native_firmware/docs/HARDWARE_FREEZE_V1.md)
- [BOM](../native_firmware/docs/BOM_V1.md)
- [GPIO Master Table](../native_firmware/docs/GPIO_MASTER_TABLE.md)
- [Wiring Diagram](../native_firmware/docs/WIRING_DIAGRAM_V1.md)
- [Assembly Guide](../native_firmware/docs/ASSEMBLY_GUIDE_V1.md)

## Verification

- [Desktop Feature Verification](desktop-feature-verification.md)
- [Testing Themes](../TESTING_THEMES.md)
- [Performance Budgets](../native_firmware/docs/PERFORMANCE_BUDGETS.md)

## Important Verification Boundary

Host builds, unit tests, and generated installers do not equal physical SHAeR acceptance. Physical-device verification must be recorded separately after testing on the actual hardware.
