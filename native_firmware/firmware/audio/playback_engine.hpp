#pragma once

#include "types.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace shaer {

enum class RepeatMode {
    Off,
    One,
    All,
};

enum class PlaybackCommandType {
    None,
    PlayTrack,
    PauseOutput,
    StopOutput,
    SetVolume,
    SeekTo,
    PrebufferTrack,
    SleepTimerExpired,
};

struct PlaybackCommand {
    PlaybackCommandType type = PlaybackCommandType::None;
    Track track;
    int value = 0;
    std::string reason;
};

struct PlaybackEngineConfig {
    int fade_in_ms = 180;
    int fade_out_ms = 180;
    int volume_ramp_step = 4;
    int max_crossfade_seconds = 5;
    int normal_buffer_ms = 1200;
    int low_power_buffer_ms = 600;
};

struct PlaybackEngineTelemetry {
    int buffer_target_ms = 1200;
    int underruns = 0;
    int recoveries = 0;
    bool prebuffer_ready = false;
    bool low_power = false;
    std::string focus_owner = "local";
};

class PlaybackEngine {
public:
    explicit PlaybackEngine(PlaybackEngineConfig config = {});

    std::vector<PlaybackCommand> load_queue(std::vector<Track> queue, int start_index);
    std::vector<PlaybackCommand> play_index(int index);
    std::vector<PlaybackCommand> pause();
    std::vector<PlaybackCommand> resume();
    std::vector<PlaybackCommand> stop(std::string reason);
    std::vector<PlaybackCommand> next();
    std::vector<PlaybackCommand> previous();
    std::vector<PlaybackCommand> seek_relative(int seconds);
    std::vector<PlaybackCommand> seek_to(int seconds);
    std::vector<PlaybackCommand> set_volume_target(int volume);
    std::vector<PlaybackCommand> update(std::chrono::milliseconds delta);

    void set_crossfade_seconds(int seconds);
    void set_repeat_mode(RepeatMode mode);
    void set_sleep_timer_minutes(int minutes);
    void set_low_power(bool low_power);
    bool request_focus(std::string owner);
    void release_focus(std::string owner);
    void note_underrun();
    void note_recovery();

    PlaybackSnapshot snapshot() const;
    PlaybackEngineTelemetry telemetry() const;
    const std::vector<Track>& queue() const;
    int current_index() const;
    int target_volume() const;
    RepeatMode repeat_mode() const;
    int crossfade_seconds() const;

private:
    bool has_current() const;
    bool has_next() const;
    int next_index() const;
    int previous_index() const;
    std::vector<PlaybackCommand> start_current(std::string reason);
    PlaybackCommand command(PlaybackCommandType type, std::string reason = {}) const;

    PlaybackEngineConfig config_;
    std::vector<Track> queue_;
    PlaybackSnapshot playback_;
    PlaybackEngineTelemetry telemetry_;
    int current_index_ = -1;
    int target_volume_ = 50;
    int output_volume_ = 50;
    int crossfade_seconds_ = 0;
    RepeatMode repeat_mode_ = RepeatMode::Off;
    std::chrono::milliseconds sleep_remaining_{0};
    std::chrono::milliseconds volume_elapsed_{0};
    bool prebuffer_sent_ = false;
};

const char* to_string(RepeatMode mode);
const char* to_string(PlaybackCommandType type);

}  // namespace shaer
