#include "shaer_app.hpp"
#include "ui_framework.hpp"

#include <iostream>
#include <sstream>
#include <string>

namespace {

class PreviewDisplay final : public shaer::Display {
public:
    void render(const shaer::RenderModel& model) override { last = model; }
    shaer::RenderModel last;
};

class PreviewAudio final : public shaer::AudioOutput {
public:
    void play_local(const shaer::Track&) override {}
    void play_spotify(const shaer::Track&) override {}
    void set_volume(int) override {}
    void pause() override {}
    void resume() override {}
    void stop() override {}
};

class PreviewBattery final : public shaer::Battery {
public:
    int percent() const override { return value; }
    bool is_charging() const override { return charging_state; }
    int value = 82;
    bool charging_state = false;
};

class PreviewBluetooth final : public shaer::Bluetooth {
public:
    bool connected() const override { return is_connected; }
    bool is_connected = false;
};

std::string escape(const std::string& value) {
    std::string output;
    for (const char character : value) {
        if (character == '\\') output += "\\\\";
        else if (character == '"') output += "\\\"";
        else if (character == '\n') output += "\\n";
        else if (character == '\r') output += "\\r";
        else output += character;
    }
    return output;
}

const char* command_type(shaer::UiCommandType type) {
    switch (type) {
        case shaer::UiCommandType::Rect: return "rect";
        case shaer::UiCommandType::Text: return "text";
        case shaer::UiCommandType::Icon: return "icon";
        case shaer::UiCommandType::Progress: return "progress";
        case shaer::UiCommandType::Transition: return "transition";
        case shaer::UiCommandType::Image: return "image";
    }
    return "unknown";
}

void print_color(std::ostream& output, const shaer::UiColor& color) {
    output << "[" << static_cast<int>(color.r) << "," << static_cast<int>(color.g)
           << "," << static_cast<int>(color.b) << "]";
}

void print_frame(const shaer::RenderModel& model, const shaer::UiFrame& frame) {
    std::cout << "{\"screen\":\"" << shaer::to_string(model.screen)
              << "\",\"theme\":\"" << escape(model.theme.id)
              << "\",\"themeName\":\"" << escape(model.theme.display_name)
              << "\",\"playback\":\"" << shaer::to_string(model.playback.state)
              << "\",\"song\":\"" << escape(model.playback.track.artist + " - " + model.playback.track.title)
              << "\",\"battery\":" << model.battery_percent
              << ",\"bluetooth\":" << (model.bluetooth_connected ? "true" : "false")
              << ",\"spotify\":" << (model.connection_state == shaer::ConnectionState::SpotifyActive ? "true" : "false")
              << ",\"wifi\":" << (model.wifi_connected ? "true" : "false")
              << ",\"fps\":" << model.animation.target_fps
              << ",\"width\":240,\"height\":320,\"commands\":[";
    for (size_t index = 0; index < frame.commands.size(); ++index) {
        const auto& command = frame.commands[index];
        if (index) std::cout << ',';
        std::cout << "{\"type\":\"" << command_type(command.type) << "\",\"rect\":["
                  << command.rect.x << ',' << command.rect.y << ',' << command.rect.w << ',' << command.rect.h
                  << "],\"fg\":";
        print_color(std::cout, command.fg);
        std::cout << ",\"bg\":";
        print_color(std::cout, command.bg);
        std::cout << ",\"text\":\"" << escape(command.text) << "\",\"scale\":"
                  << command.scale << ",\"value\":" << command.value
                  << ",\"max\":" << command.max_value << ",\"selected\":"
                  << (command.selected ? "true" : "false") << '}';
    }
    std::cout << "]}\n" << std::flush;
}

shaer::InputAction parse_action(const std::string& command) {
    if (command == "cw") return shaer::InputAction::Down;
    if (command == "ccw") return shaer::InputAction::Up;
    if (command == "click") return shaer::InputAction::Confirm;
    if (command == "back") return shaer::InputAction::Back;
    if (command == "play") return shaer::InputAction::PlayPause;
    if (command == "pause") return shaer::InputAction::PlayPause;
    if (command == "next") return shaer::InputAction::Next;
    if (command == "previous") return shaer::InputAction::Previous;
    if (command == "theme") return shaer::InputAction::CycleTheme;
    if (command == "sleep") return shaer::InputAction::EnterSleep;
    if (command == "wake") return shaer::InputAction::ToggleBatterySaver;
    if (command == "spotify") return shaer::InputAction::StartSpotify;
    if (command == "local") return shaer::InputAction::OpenLibrary;
    if (command == "marginalia") return shaer::InputAction::OpenMarginalia;
    if (command == "bluetooth") return shaer::InputAction::OpenSettings;
    if (command == "battery") return shaer::InputAction::SimulateLowBattery;
    if (command == "wifi") return shaer::InputAction::SimulateSpotifyDisconnect;
    if (command == "sdcard") return shaer::InputAction::OpenSettings;
    return shaer::InputAction::None;
}

}  // namespace

int main() {
    PreviewDisplay display;
    PreviewAudio audio;
    PreviewBattery battery;
    PreviewBluetooth bluetooth;
    shaer::ShaerApp app({&display, &audio, &battery, &bluetooth});
    shaer::UiFramework ui;
    app.boot();
    print_frame(display.last, ui.build_frame(display.last));

    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "quit") break;
        if (command == "charge") battery.charging_state = true;
        if (command.rfind("theme:", 0) == 0) {
            const auto requested = command.substr(6);
            app.select_theme(requested);
        } else if (command == "home" || command == "library" || command == "playlist" || command == "settings") {
            while (app.screen() != shaer::Screen::Home && app.navigation_depth() > 1) app.handle(shaer::InputAction::Back);
            if (command == "library" || command == "playlist") app.handle(shaer::InputAction::OpenLibrary);
            if (command == "playlist") app.handle(shaer::InputAction::Down);
            if (command == "settings") app.handle(shaer::InputAction::OpenSettings);
        } else if (command == "recorder" || command == "bluetooth") {
            while (app.screen() != shaer::Screen::Home && app.navigation_depth() > 1) app.handle(shaer::InputAction::Back);
            const int steps = command == "recorder" ? 2 : 1;
            for (int step = 0; step < steps; ++step) app.handle(shaer::InputAction::Down);
            app.handle(shaer::InputAction::Confirm);
        } else if (command == "marginalia") {
            if (app.playback_state() == shaer::PlaybackState::Stopped) {
                app.handle(shaer::InputAction::OpenLibrary);
                app.handle(shaer::InputAction::Confirm);
            }
            app.handle(shaer::InputAction::OpenMarginalia);
        } else if (command == "sdcard") {
            app.show_diagnostic_popup("SD CARD", "STORAGE READY");
        } else {
            app.handle(parse_action(command));
        }
        print_frame(display.last, ui.build_frame(display.last));
    }
    return 0;
}
