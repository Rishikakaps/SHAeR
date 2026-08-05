#include "input_manager.hpp"

namespace shaer {

Event InputManager::map(InputAction action) const {
    switch (action) {
        case InputAction::Confirm: return {EventType::ButtonOkPressed, 0, 0, action};
        case InputAction::Back: return {EventType::ButtonBackPressed, 0, 0, action};
        case InputAction::Up: return {EventType::NavigateUp, 0, 0, action};
        case InputAction::Down: return {EventType::NavigateDown, 0, 0, action};
        case InputAction::PlayPause: return {EventType::PlayPausePressed, 0, 0, action};
        case InputAction::Next: return {EventType::NextTrackRequested, 0, 0, action};
        case InputAction::Previous: return {EventType::PreviousTrackRequested, 0, 0, action};
        case InputAction::SeekForward: return {EventType::SeekForwardRequested, 0, 0, action};
        case InputAction::SeekBackward: return {EventType::SeekBackwardRequested, 0, 0, action};
        case InputAction::VolumeUp: return {EventType::VolumeUpRequested, 0, 0, action};
        case InputAction::VolumeDown: return {EventType::VolumeDownRequested, 0, 0, action};
        case InputAction::OpenLibrary: return {EventType::OpenLibraryRequested, 0, 0, action};
        case InputAction::OpenSettings: return {EventType::OpenSettingsRequested, 0, 0, action};
        case InputAction::StartSpotify: return {EventType::SpotifyConnectRequested, 0, 0, action};
        case InputAction::CycleTheme: return {EventType::CycleThemeRequested, 0, 0, action};
        case InputAction::SimulateSpotifyDisconnect: return {EventType::SpotifyConnectionLost, 0, 0, action};
        case InputAction::SimulateBluetoothDisconnect: return {EventType::BluetoothConnectionLost, 0, 0, action};
        case InputAction::SimulateLowBattery: return {EventType::LowBatterySimulated, 0, 0, action};
        case InputAction::ToggleBatterySaver: return {EventType::BatterySaverToggleRequested, 0, 0, action};
        case InputAction::EnterSleep: return {EventType::SleepRequested, 0, 0, action};
        case InputAction::BeginShutdown: return {EventType::ShutdownRequested, 0, 0, action};
        case InputAction::Reboot: return {EventType::RebootRequested, 0, 0, action};
        case InputAction::Quit: return {EventType::QuitRequested, 0, 0, action};
        default: return {EventType::InputReceived, 0, 0, action};
    }
}

}  // namespace shaer
