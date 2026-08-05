#pragma once

#include "hal.hpp"
#include "console_renderer.hpp"

#include <deque>
#include <string>

namespace shaer {

class DesktopDisplay final : public Display {
public:
    DesktopDisplay();
    void render(const RenderModel& model) override;

private:
    ConsoleRenderer renderer_;
};

class DesktopAudioOutput final : public AudioOutput {
public:
    void play_local(const Track& track) override;
    void play_spotify(const Track& track) override;
    void set_volume(int volume) override;
    void stop() override;
    void pause() override;

private:
    PlaybackState state_ = PlaybackState::Stopped;
    PlaybackSource source_ = PlaybackSource::None;
    Track track_;
    int volume_ = 50;
};

class DesktopBattery final : public Battery {
public:
    int percent() const override;
    bool is_charging() const override;
    void set_percent(int percent);

private:
    int percent_ = 87;
};

class DesktopBluetooth final : public Bluetooth {
public:
    bool connected() const override;
    void set_connected(bool connected);

private:
    bool connected_ = true;
};

class DesktopInput final : public Input {
public:
    InputAction next_action() override;
    InputAction poll_action() override;

private:
    InputAction parse(const std::string& line) const;
};

}  // namespace shaer
