#include "firmware_runtime.hpp"

#include <algorithm>
#include <memory>
#include <thread>

namespace shaer {

FirmwareRuntime::FirmwareRuntime(RuntimeHardware hardware, StructuredLogger* logger)
    : hardware_(hardware),
      logger_(logger),
      scheduler_(std::chrono::milliseconds(16)) {
    scheduler_.add_service(std::make_unique<LoggingService>(), std::chrono::milliseconds(16));
    scheduler_.add_service(std::make_unique<InputService>(hardware_.input), std::chrono::milliseconds(16));
    scheduler_.add_service(std::make_unique<LocalLibraryService>(), std::chrono::seconds(10));
    scheduler_.add_service(std::make_unique<AudioService>(hardware_.audio), std::chrono::milliseconds(16));
    scheduler_.add_service(std::make_unique<ConnectivityService>(), std::chrono::seconds(2));
    scheduler_.add_service(std::make_unique<ClockService>(), std::chrono::seconds(5));
    scheduler_.add_service(std::make_unique<ThemeService>(), std::chrono::milliseconds(250));
    scheduler_.add_service(std::make_unique<DeviceInfoService>(), std::chrono::seconds(5));
    scheduler_.add_service(std::make_unique<SettingsService>(), std::chrono::milliseconds(250));
    scheduler_.add_service(std::make_unique<BatteryService>(hardware_.battery), std::chrono::seconds(1));
    scheduler_.add_service(std::make_unique<BluetoothService>(hardware_.bluetooth), std::chrono::seconds(1));
    scheduler_.add_service(std::make_unique<NavigationService>(), std::chrono::milliseconds(16));
    scheduler_.add_service(std::make_unique<PowerService>(), std::chrono::milliseconds(250));
    scheduler_.add_service(std::make_unique<RenderService>(hardware_.display), std::chrono::milliseconds(16));
}

const AppState& FirmwareRuntime::state() const {
    return state_.state();
}

EventBus& FirmwareRuntime::events() {
    return events_;
}

void FirmwareRuntime::apply_boot_report(const BootReport& report) {
    state_.apply_boot_settings(report.settings);
    if (report.has_failures()) {
        state_.set_notification({
            report.diagnostic_title(),
            report.diagnostic_body(),
            true,
            "return_previous",
        });
        state_.set_screen(Screen::Popup, FirmwareState::Popup, false);
    }
    events_.publish({EventType::BootCompleted, 0, 0, InputAction::None, state_.state().current_screen, "BootFirmware", "boot report applied"});
}

void FirmwareRuntime::run() {
    ServiceContext context{events_, state_, logger_};
    if (!initialized_) {
        scheduler_.init(context);
        initialized_ = true;
    }
    while (state_.state().running) {
        scheduler_.tick(context);
        std::this_thread::sleep_for(scheduler_.tick_interval());
    }
    scheduler_.shutdown(context);
}

void FirmwareRuntime::run_for_ticks(int ticks) {
    ServiceContext context{events_, state_, logger_};
    if (!initialized_) {
        scheduler_.init(context);
        initialized_ = true;
    }
    for (int i = 0; i < ticks && state_.state().running; ++i) {
        scheduler_.tick(context);
    }
}

void FirmwareRuntime::shutdown() {
    state_.set_running(false);
}

}  // namespace shaer
