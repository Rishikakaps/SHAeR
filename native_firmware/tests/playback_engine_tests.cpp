#include "playback_engine.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace {

shaer::Track track(std::string title, int duration = 5) {
    shaer::Track out;
    out.title = std::move(title);
    out.artist = "Test Artist";
    out.album = "Test Album";
    out.file_path = "/music/" + out.title + ".mp3";
    out.source = shaer::PlaybackSource::Local;
    out.duration_seconds = duration;
    out.codec = "mp3";
    return out;
}

bool has_command(const std::vector<shaer::PlaybackCommand>& commands, shaer::PlaybackCommandType type) {
    for (const auto& command : commands) {
        if (command.type == type) return true;
    }
    return false;
}

void queue_play_pause_resume() {
    shaer::PlaybackEngine engine;
    auto commands = engine.load_queue({track("one"), track("two")}, 0);
    assert(has_command(commands, shaer::PlaybackCommandType::PlayTrack));
    assert(engine.snapshot().state == shaer::PlaybackState::Playing);
    assert(engine.snapshot().queue_size == 2);

    commands = engine.pause();
    assert(has_command(commands, shaer::PlaybackCommandType::PauseOutput));
    assert(engine.snapshot().state == shaer::PlaybackState::Paused);

    commands = engine.resume();
    assert(has_command(commands, shaer::PlaybackCommandType::PlayTrack));
    assert(engine.snapshot().state == shaer::PlaybackState::Playing);
}

void gapless_and_crossfade_prebuffer() {
    shaer::PlaybackEngine engine;
    engine.set_crossfade_seconds(3);
    engine.load_queue({track("one", 6), track("two", 6)}, 0);
    auto commands = engine.update(std::chrono::seconds(3));
    assert(has_command(commands, shaer::PlaybackCommandType::PrebufferTrack));
    assert(engine.telemetry().prebuffer_ready);

    commands = engine.update(std::chrono::seconds(3));
    assert(has_command(commands, shaer::PlaybackCommandType::PlayTrack));
    assert(engine.snapshot().track.title == "two");
}

void volume_ramps_without_jumps() {
    shaer::PlaybackEngine engine({180, 180, 5, 5, 1200, 600});
    engine.set_volume_target(80);
    int last = 50;
    bool saw_volume = false;
    for (int i = 0; i < 10; ++i) {
        const auto commands = engine.update(std::chrono::milliseconds(40));
        for (const auto& command : commands) {
            if (command.type == shaer::PlaybackCommandType::SetVolume) {
                saw_volume = true;
                assert(command.value >= last);
                assert(command.value - last <= 5);
                last = command.value;
            }
        }
    }
    assert(saw_volume);
    assert(engine.target_volume() == 80);
}

void seek_repeat_and_sleep_timer() {
    shaer::PlaybackEngine engine;
    engine.load_queue({track("one", 10), track("two", 10)}, 0);
    auto commands = engine.seek_relative(4);
    assert(has_command(commands, shaer::PlaybackCommandType::SeekTo));
    assert(engine.snapshot().progress_seconds == 4);

    engine.set_repeat_mode(shaer::RepeatMode::One);
    commands = engine.update(std::chrono::seconds(10));
    assert(has_command(commands, shaer::PlaybackCommandType::PlayTrack));
    assert(engine.snapshot().track.title == "one");

    engine.set_sleep_timer_minutes(15);
    commands = engine.update(std::chrono::minutes(15));
    assert(has_command(commands, shaer::PlaybackCommandType::SleepTimerExpired));
    assert(engine.snapshot().state == shaer::PlaybackState::Stopped);
}

void focus_and_low_power_hooks() {
    shaer::PlaybackEngine engine;
    assert(engine.request_focus("local"));
    engine.load_queue({track("one")}, 0);
    assert(!engine.request_focus("spotify"));
    engine.release_focus("local");
    engine.stop("done");
    assert(engine.request_focus("spotify"));

    engine.set_low_power(true);
    assert(engine.telemetry().low_power);
    assert(engine.telemetry().buffer_target_ms == 600);
    engine.note_underrun();
    engine.note_recovery();
    assert(engine.telemetry().underruns == 1);
    assert(engine.telemetry().recoveries == 1);
}

}  // namespace

int main() {
    queue_play_pause_resume();
    gapless_and_crossfade_prebuffer();
    volume_ramps_without_jumps();
    seek_repeat_and_sleep_timer();
    focus_and_low_power_hooks();
    std::cout << "playback_engine_tests passed\n";
    return 0;
}
