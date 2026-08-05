#include "firmware_runtime.hpp"
#include "settings_catalog.hpp"
#include "settings_store.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {

class TestInput final : public shaer::Input {
public:
    shaer::InputAction next_action() override { return poll_action(); }
    shaer::InputAction poll_action() override {
        const auto out = next;
        next = shaer::InputAction::None;
        return out;
    }
    shaer::InputAction next = shaer::InputAction::None;
};

class TestDisplay final : public shaer::Display {
public:
    void render(const shaer::RenderModel& model) override {
        last = model;
        ++frames;
    }
    shaer::RenderModel last;
    int frames = 0;
};

class TestAudio final : public shaer::AudioOutput {
public:
    void play_local(const shaer::Track&) override {}
    void play_spotify(const shaer::Track&) override {}
    void set_volume(int volume) override { last_volume = volume; }
    void stop() override {}
    void pause() override {}
    int last_volume = 0;
};

class TestBattery final : public shaer::Battery {
public:
    int percent() const override { return 76; }
    bool is_charging() const override { return true; }
};

class TestBluetooth final : public shaer::Bluetooth {
public:
    bool connected() const override { return false; }
};

shaer::FirmwareRuntime make_runtime(TestDisplay& display, TestAudio& audio, TestInput& input, TestBattery& battery, TestBluetooth& bluetooth, const std::string& data_dir) {
    shaer::StructuredLogger* logger = nullptr;
    shaer::FirmwareRuntime runtime({display, audio, input, battery, bluetooth}, logger);
    shaer::BootReport report;
    report.settings.music_directory = data_dir + "/music";
    report.settings.values = shaer::default_settings_values();
    report.settings.values["audio.volume"] = "50";
    report.settings.volume = 50;
    runtime.apply_boot_report(report);
    return runtime;
}

const shaer::SettingsItem& current_settings_item(const shaer::AppState& state) {
    assert(state.current_screen == shaer::Screen::Settings);
    assert(state.settings_ui.inside_category);
    assert(!state.settings_ui.categories.empty());
    const auto& category = state.settings_ui.categories[static_cast<size_t>(state.settings_ui.category_index)];
    assert(!category.items.empty());
    return category.items[static_cast<size_t>(state.settings_ui.item_index)];
}

void open_settings_category(shaer::FirmwareRuntime& runtime, TestInput& input, const std::string& title) {
    input.next = shaer::InputAction::OpenSettings;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Settings);
    int guard = 0;
    while (runtime.state().settings_ui.categories[static_cast<size_t>(runtime.state().settings_ui.category_index)].title != title) {
        input.next = shaer::InputAction::Down;
        runtime.run_for_ticks(2);
        assert(++guard < 20);
    }
    input.next = shaer::InputAction::Confirm;
    runtime.run_for_ticks(2);
    assert(runtime.state().settings_ui.inside_category);
}

void select_settings_item(shaer::FirmwareRuntime& runtime, TestInput& input, const std::string& key) {
    int guard = 0;
    while (current_settings_item(runtime.state()).key != key) {
        input.next = shaer::InputAction::Down;
        runtime.run_for_ticks(2);
        assert(++guard < 20);
    }
}

void catalog_contains_required_categories() {
    shaer::RuntimeSettings settings;
    settings.values = shaer::default_settings_values();
    shaer::DeviceInfoSnapshot device;
    const auto categories = shaer::build_settings_catalog(settings, device, 88, false, {});
    assert(categories.size() == 10);
    assert(categories.front().title == "Appearance");
    assert(categories.back().title == "About");
    bool has_developer = false;
    for (const auto& category : categories) {
        if (category.title == "Advanced") has_developer = true;
    }
    assert(has_developer);
}

void setting_action_contract_is_explicit() {
    assert(shaer::setting_is_action("storage.manual_rescan"));
    assert(shaer::setting_is_action("storage.rebuild_database"));
    assert(shaer::setting_is_action("privacy.clear_recently_played"));
    assert(shaer::setting_is_action("privacy.clear_cache"));
    assert(shaer::setting_is_action("advanced.recovery_mode"));
    assert(!shaer::setting_is_action("appearance.brightness"));
    assert(!shaer::setting_is_action("advanced.developer_mode"));
}

void settings_persist_across_store_reload() {
    const std::string dir = "/tmp/shaer_settings_services_" + std::to_string(getpid());
    std::filesystem::remove_all(dir);
    shaer::SettingsStore store(dir + "/settings.db");
    assert(store.open());
    assert(store.migrate());
    assert(store.put("appearance.brightness", "40"));
    assert(store.put("advanced.developer_mode", "on"));
    const auto loaded = store.load_runtime_settings();
    assert(loaded.values.at("appearance.brightness") == "40");
    assert(loaded.values.at("advanced.developer_mode") == "on");
    std::filesystem::remove_all(dir);
}

void runtime_settings_navigation_and_save() {
    const std::string dir = "/tmp/shaer_settings_runtime_" + std::to_string(getpid());
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir + "/music");
    TestDisplay display;
    TestAudio audio;
    TestInput input;
    TestBattery battery;
    TestBluetooth bluetooth;
    auto runtime = make_runtime(display, audio, input, battery, bluetooth, dir);
    runtime.run_for_ticks(2);
    assert(!runtime.state().settings_ui.categories.empty());

    input.next = shaer::InputAction::OpenSettings;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Settings);

    input.next = shaer::InputAction::Confirm;
    runtime.run_for_ticks(2);
    assert(runtime.state().settings_ui.inside_category);
    assert(runtime.state().settings_ui.categories[runtime.state().settings_ui.category_index].title == "Appearance");

    input.next = shaer::InputAction::Down;
    runtime.run_for_ticks(2);
    assert(runtime.state().settings_ui.item_index == 1);

    input.next = shaer::InputAction::Confirm;
    runtime.run_for_ticks(3);
    assert(runtime.state().settings.values.at("appearance.brightness") == "100");

    input.next = shaer::InputAction::Back;
    runtime.run_for_ticks(2);
    assert(!runtime.state().settings_ui.inside_category);
    std::filesystem::remove_all(dir);
}

void settings_shutdown_and_restart_are_confirmed_actions() {
    {
        const std::string dir = "/tmp/shaer_settings_shutdown_" + std::to_string(getpid());
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir + "/music");
        TestDisplay display;
        TestAudio audio;
        TestInput input;
        TestBattery battery;
        TestBluetooth bluetooth;
        auto runtime = make_runtime(display, audio, input, battery, bluetooth, dir);
        runtime.run_for_ticks(2);

        open_settings_category(runtime, input, "Power");
        select_settings_item(runtime, input, "power.shutdown");
        input.next = shaer::InputAction::Confirm;
        runtime.run_for_ticks(2);
        assert(runtime.state().current_screen == shaer::Screen::Popup);
        assert(runtime.state().notification.confirm_action == "shutdown");
        assert(runtime.state().running);

        input.next = shaer::InputAction::Confirm;
        runtime.run_for_ticks(2);
        assert(!runtime.state().running);
        assert(runtime.state().firmware_state == shaer::FirmwareState::Shutdown);
        assert(!runtime.state().reboot_requested);
        std::filesystem::remove_all(dir);
    }

    {
        const std::string dir = "/tmp/shaer_settings_restart_" + std::to_string(getpid());
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir + "/music");
        TestDisplay display;
        TestAudio audio;
        TestInput input;
        TestBattery battery;
        TestBluetooth bluetooth;
        auto runtime = make_runtime(display, audio, input, battery, bluetooth, dir);
        runtime.run_for_ticks(2);

        open_settings_category(runtime, input, "Power");
        select_settings_item(runtime, input, "power.restart");
        input.next = shaer::InputAction::Confirm;
        runtime.run_for_ticks(2);
        assert(runtime.state().current_screen == shaer::Screen::Popup);
        assert(runtime.state().notification.confirm_action == "restart");
        assert(runtime.state().running);

        input.next = shaer::InputAction::Confirm;
        runtime.run_for_ticks(2);
        assert(!runtime.state().running);
        assert(runtime.state().firmware_state == shaer::FirmwareState::Shutdown);
        assert(runtime.state().reboot_requested);
        std::filesystem::remove_all(dir);
    }
}

void developer_mode_sleep_shutdown_and_reboot() {
    const std::string dir = "/tmp/shaer_settings_power_" + std::to_string(getpid());
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir + "/music");
    TestDisplay display;
    TestAudio audio;
    TestInput input;
    TestBattery battery;
    TestBluetooth bluetooth;
    auto runtime = make_runtime(display, audio, input, battery, bluetooth, dir);
    runtime.run_for_ticks(2);

    input.next = shaer::InputAction::EnterSleep;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Sleep);

    input.next = shaer::InputAction::ToggleBatterySaver;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Home);

    input.next = shaer::InputAction::Reboot;
    runtime.run_for_ticks(2);
    assert(!runtime.state().running);
    assert(runtime.state().reboot_requested);
    assert(runtime.state().firmware_state == shaer::FirmwareState::Shutdown);
    std::filesystem::remove_all(dir);
}

}  // namespace

int main() {
    catalog_contains_required_categories();
    setting_action_contract_is_explicit();
    settings_persist_across_store_reload();
    runtime_settings_navigation_and_save();
    settings_shutdown_and_restart_are_confirmed_actions();
    developer_mode_sleep_shutdown_and_reboot();
    std::cout << "settings_services_tests passed\n";
    return 0;
}
