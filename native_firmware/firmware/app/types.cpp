#include "types.hpp"

namespace shaer {

const char* to_string(Screen screen) {
    switch (screen) {
        case Screen::Boot: return "Boot";
        case Screen::Home: return "Home";
        case Screen::Library: return "Library";
        case Screen::NowPlaying: return "Now Playing";
        case Screen::Marginalia: return "Marginalia";
        case Screen::VoiceArchive: return "Voice Archive";
        case Screen::BluetoothConnect: return "Bluetooth Connect";
        case Screen::Settings: return "Settings";
        case Screen::About: return "About";
        case Screen::Popup: return "Popup";
        case Screen::Aod: return "AOD";
        case Screen::Charging: return "Charging";
        case Screen::Sleep: return "Sleep";
        case Screen::Shutdown: return "Shutdown";
    }
    return "Unknown";
}

const char* to_string(FirmwareState state) {
    switch (state) {
        case FirmwareState::Booting: return "Booting";
        case FirmwareState::Home: return "Home";
        case FirmwareState::LocalLibrary: return "Local Library";
        case FirmwareState::Playback: return "Playback";
        case FirmwareState::Marginalia: return "Marginalia";
        case FirmwareState::Settings: return "Settings";
        case FirmwareState::Popup: return "Popup";
        case FirmwareState::Sleep: return "Sleep";
        case FirmwareState::Charging: return "Charging";
        case FirmwareState::Shutdown: return "Shutdown";
    }
    return "Unknown";
}

const char* to_string(PlaybackSource source) {
    switch (source) {
        case PlaybackSource::None: return "None";
        case PlaybackSource::Local: return "Local";
        case PlaybackSource::Spotify: return "Spotify";
        case PlaybackSource::Bluetooth: return "Bluetooth";
    }
    return "Unknown";
}

const char* to_string(PlaybackState state) {
    switch (state) {
        case PlaybackState::Stopped: return "Stopped";
        case PlaybackState::Playing: return "Playing";
        case PlaybackState::Paused: return "Paused";
    }
    return "Unknown";
}

const char* to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::Ready: return "Ready";
        case ConnectionState::SpotifyActive: return "Spotify Active";
        case ConnectionState::SpotifyRecovering: return "Spotify Recovering";
        case ConnectionState::BluetoothRecovering: return "Bluetooth Recovering";
        case ConnectionState::LocalOffline: return "Local Offline";
        case ConnectionState::WifiRecovering: return "Wi-Fi Recovering";
        case ConnectionState::LowPower: return "Low Power";
    }
    return "Unknown";
}

const char* to_string(ReplayGainMode mode) {
    switch (mode) {
        case ReplayGainMode::Off: return "Off";
        case ReplayGainMode::Track: return "Track";
        case ReplayGainMode::Album: return "Album";
    }
    return "Unknown";
}

const char* to_string(QualityMode mode) {
    switch (mode) {
        case QualityMode::Balanced: return "Balanced";
        case QualityMode::ArchiveQuality: return "Archive Quality";
    }
    return "Unknown";
}

const char* to_string(PowerMode mode) {
    switch (mode) {
        case PowerMode::Normal: return "Normal";
        case PowerMode::BatterySaver: return "Battery Saver";
        case PowerMode::Critical: return "Critical";
    }
    return "Unknown";
}

}  // namespace shaer
