#pragma once

#include "types.hpp"

namespace shaer {

// HAL v1 keeps the application portable between the desktop simulator and the
// Pi. Each physical part that may be upgraded or replaced should sit behind one
// of these interfaces instead of leaking GPIO, ALSA, SPI, or module-specific
// names into the app layer.
class Display {
public:
    virtual ~Display() = default;
    virtual void render(const RenderModel& model) = 0;
};

class AudioOutput {
public:
    virtual ~AudioOutput() = default;
    virtual void play_local(const Track& track) = 0;
    virtual void play_spotify(const Track& track) = 0;
    virtual void set_volume(int volume) = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() {}
    virtual void seek_seconds(int seconds) { (void)seconds; }
    virtual void prebuffer_local(const Track& track) { (void)track; }
    virtual void set_crossfade_seconds(int seconds) { (void)seconds; }
};

// Battery HAL v1 is intentionally tiny: the app needs a reliable percentage
// and charging state, while the Pi implementation can hide whether that comes
// from MAX17048 now or a different fuel gauge in a later compact revision.
class Battery {
public:
    virtual ~Battery() = default;
    virtual int percent() const = 0;
    virtual bool is_charging() const = 0;
};

class Bluetooth {
public:
    virtual ~Bluetooth() = default;
    virtual bool connected() const = 0;
};

// Input HAL v1 maps desktop keys, EC11 encoders, and GPIO buttons into the
// same semantic actions so navigation bugs can be tested before hardware is
// assembled.
class Input {
public:
    virtual ~Input() = default;
    virtual InputAction next_action() = 0;
    virtual InputAction poll_action() { return InputAction::None; }
};

}  // namespace shaer
