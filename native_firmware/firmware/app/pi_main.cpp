#include "pi_hal.hpp"
#include "pi_system.hpp"
#include "boot_firmware.hpp"
#include "firmware_runtime.hpp"
#include "settings_store.hpp"
#include "structured_logger.hpp"

#include <csignal>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {

volatile sig_atomic_t g_stop = 0;

void handle_signal(int) {
    g_stop = 1;
}

}  // namespace

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    shaer::PiPinMap pins;
    shaer::PiDisplay display({"/dev/spidev0.0", 240, 320, 1000000, pins});
    shaer::PiAudioOutput audio;
    shaer::PiBattery battery;
    shaer::PiBluetooth bluetooth;
    shaer::PiInput input(pins);

    shaer::PiDiagnostics diagnostics;
    shaer::LinuxWatchdog watchdog;
    shaer::SettingsStore settings("/var/lib/shaer/settings.db");
    shaer::StructuredLogger logger("/var/log/shaer");
    shaer::BootFirmware boot(
        {"/var/lib/shaer", "/var/log/shaer", 8000, true},
        diagnostics,
        settings,
        logger,
        watchdog,
        &display);
    const shaer::BootReport boot_report = boot.cold_boot();

    shaer::FirmwareRuntime runtime({display, audio, input, battery, bluetooth}, &logger);
    runtime.apply_boot_report(boot_report);
    boot.mark_boot_successful();
    std::cout << "[SHAeR Pi] Bring-up running. Press Ctrl+C to stop.\n";
    while (!g_stop && runtime.state().running) {
        runtime.run_for_ticks(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    if (runtime.state().firmware_state == shaer::FirmwareState::Shutdown) {
        boot.clean_shutdown(runtime.state().reboot_requested ? "safe reboot requested" : "power button long press");
        if (runtime.state().reboot_requested) {
            std::cout << "[SHAeR Pi] Reboot requested. Calling Linux reboot.\n";
            std::system("sync");
            std::system("reboot");
            return 0;
        }
        std::cout << "[SHAeR Pi] Shutdown requested. Calling Linux shutdown.\n";
        std::system("sync");
        std::system("shutdown -h now");
    }

    audio.stop();
    std::cout << "[SHAeR Pi] Bring-up stopped.\n";
    return 0;
}
