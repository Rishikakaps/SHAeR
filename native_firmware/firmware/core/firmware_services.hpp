#pragma once

#include "hal.hpp"
#include "input_manager.hpp"
#include "music_library_store.hpp"
#include "notification_manager.hpp"
#include "playback_engine.hpp"
#include "screen_manager.hpp"
#include "service.hpp"
#include "settings_catalog.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace shaer {

class LoggingService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;
};

class InputService final : public FirmwareService {
public:
    explicit InputService(Input& input);
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    Input& input_;
    InputManager input_manager_;
};

class NavigationService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    void change_screen(ServiceContext& context, Screen screen, FirmwareState state);
    NotificationManager notifications_;
};

class ThemeService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    std::vector<std::string> theme_ids_{
        "default",
        "archive_dark",
        "bombay_ticket",
        "indian_raga",
        "windows_xp",
        "japanese_punk",
        "ghibli_garden",
    };
};

class SettingsService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    std::string database_path_for(const RuntimeSettings& settings) const;
    void refresh_catalog(ServiceContext& context);
    bool persist(const RuntimeSettings& settings, const std::string& key, const std::string& value) const;
};

class DeviceInfoService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    DeviceInfoSnapshot read_info(const AppState& state) const;
    std::chrono::milliseconds elapsed_{0};
};

class LocalLibraryService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    std::string database_path_for(const std::string& directory) const;
};

class AudioService final : public FirmwareService {
public:
    explicit AudioService(AudioOutput& audio);
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    void apply_commands(ServiceContext& context, const std::vector<PlaybackCommand>& commands);
    AudioOutput& audio_;
    PlaybackEngine engine_;
};

class ConnectivityService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;
};

class ClockService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    ClockSnapshot read_system_clock() const;
    std::string last_minute_;
};

class PowerService final : public FirmwareService {
public:
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    std::chrono::milliseconds sleep_elapsed_{0};
};

class BatteryService final : public FirmwareService {
public:
    explicit BatteryService(Battery& battery);
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    Battery& battery_;
    std::chrono::milliseconds elapsed_{0};
};

class BluetoothService final : public FirmwareService {
public:
    explicit BluetoothService(Bluetooth& bluetooth);
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    Bluetooth& bluetooth_;
    std::chrono::milliseconds elapsed_{0};
};

class RenderService final : public FirmwareService {
public:
    explicit RenderService(Display& display);
    const char* name() const override;
    void init(ServiceContext& context) override;
    void start(ServiceContext& context) override;
    void handle_event(const Event& event, ServiceContext& context) override;
    void update(ServiceContext& context, std::chrono::milliseconds delta) override;
    void shutdown(ServiceContext& context) override;

private:
    Display& display_;
    ScreenManager screen_manager_;
    std::chrono::milliseconds elapsed_{0};
};

}  // namespace shaer
