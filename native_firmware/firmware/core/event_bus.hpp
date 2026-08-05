#pragma once

#include "types.hpp"

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <utility>

namespace shaer {

enum class EventType {
    BootStarted,
    BootCompleted,
    BootDiagnosticRaised,
    Tick,
    InputReceived,
    ButtonOkPressed,
    ButtonBackPressed,
    NavigateUp,
    NavigateDown,
    PlayPausePressed,
    OpenLibraryRequested,
    OpenSettingsRequested,
    SettingActivated,
    SettingsChanged,
    BatterySaverToggleRequested,
    CycleThemeRequested,
    SleepRequested,
    WakeRequested,
    ShutdownRequested,
    RebootRequested,
    LocalLibraryScanRequested,
    LocalLibraryLoaded,
    PlaySelectedTrackRequested,
    SpotifyConnectRequested,
    SpotifyConnectionLost,
    BluetoothConnectionLost,
    WifiConnectionLost,
    LowBatterySimulated,
    PlaybackStarted,
    PlaybackPaused,
    PlaybackStopped,
    NextTrackRequested,
    PreviousTrackRequested,
    SeekForwardRequested,
    SeekBackwardRequested,
    VolumeUpRequested,
    VolumeDownRequested,
    ClockChanged,
    ScreenChanged,
    RenderRequested,
    ServiceStarted,
    ServiceStopped,
    QuitRequested,
};

struct Event {
    Event() = default;
    Event(
        EventType type_value,
        uint64_t sequence_value = 0,
        int tick_value = 0,
        InputAction input_value = InputAction::None,
        Screen screen_value = Screen::Home,
        std::string source_value = {},
        std::string message_value = {},
        std::map<std::string, std::string> fields_value = {})
        : type(type_value),
          sequence(sequence_value),
          tick(tick_value),
          input(input_value),
          screen(screen_value),
          source(std::move(source_value)),
          message(std::move(message_value)),
          fields(std::move(fields_value)) {}

    EventType type = EventType::Tick;
    uint64_t sequence = 0;
    int tick = 0;
    InputAction input = InputAction::None;
    Screen screen = Screen::Home;
    std::string source;
    std::string message;
    std::map<std::string, std::string> fields;
};

class EventBus {
public:
    void publish(Event event);
    std::vector<Event> drain();
    bool empty() const;
    uint64_t next_sequence() const;

private:
    std::deque<Event> queue_;
    uint64_t sequence_ = 1;
};

const char* to_string(EventType type);

}  // namespace shaer
