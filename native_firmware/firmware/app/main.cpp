#include "desktop_hal.hpp"
#include "desktop_system.hpp"
#include "boot_firmware.hpp"
#include "firmware_runtime.hpp"
#include "settings_store.hpp"
#include "structured_logger.hpp"

int main() {
    shaer::DesktopDisplay display;
    shaer::DesktopAudioOutput audio;
    shaer::DesktopBattery battery;
    shaer::DesktopBluetooth bluetooth;
    shaer::DesktopInput input;

    shaer::DesktopDiagnostics diagnostics;
    shaer::DesktopWatchdog watchdog;
    shaer::SettingsStore settings("data/settings.db");
    shaer::StructuredLogger logger("logs");
    shaer::BootFirmware boot(
        {"data", "logs", 8000, true},
        diagnostics,
        settings,
        logger,
        watchdog,
        &display);
    const shaer::BootReport boot_report = boot.cold_boot();

    shaer::FirmwareRuntime runtime({display, audio, input, battery, bluetooth}, &logger);
    runtime.apply_boot_report(boot_report);
    boot.mark_boot_successful();
    runtime.run();
    boot.clean_shutdown("runtime stopped");

    return 0;
}
