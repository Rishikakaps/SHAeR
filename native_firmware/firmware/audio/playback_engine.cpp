#include "playback_engine.hpp"

#include <algorithm>
#include <utility>

namespace shaer {

PlaybackEngine::PlaybackEngine(PlaybackEngineConfig config) : config_(config) {
    telemetry_.buffer_target_ms = config_.normal_buffer_ms;
}

std::vector<PlaybackCommand> PlaybackEngine::load_queue(std::vector<Track> queue, int start_index) {
    queue_ = std::move(queue);
    if (queue_.empty()) {
        current_index_ = -1;
        playback_ = {};
        return {command(PlaybackCommandType::StopOutput, "empty queue")};
    }
    current_index_ = std::clamp(start_index, 0, static_cast<int>(queue_.size()) - 1);
    return start_current("queue loaded");
}

std::vector<PlaybackCommand> PlaybackEngine::play_index(int index) {
    if (queue_.empty()) return {command(PlaybackCommandType::StopOutput, "empty queue")};
    current_index_ = std::clamp(index, 0, static_cast<int>(queue_.size()) - 1);
    return start_current("index selected");
}

std::vector<PlaybackCommand> PlaybackEngine::pause() {
    if (playback_.state != PlaybackState::Playing) return {};
    playback_.state = PlaybackState::Paused;
    return {command(PlaybackCommandType::PauseOutput, "pause fade-out")};
}

std::vector<PlaybackCommand> PlaybackEngine::resume() {
    if (!has_current()) return {};
    playback_.state = PlaybackState::Playing;
    return {command(PlaybackCommandType::PlayTrack, "resume fade-in")};
}

std::vector<PlaybackCommand> PlaybackEngine::stop(std::string reason) {
    playback_.state = PlaybackState::Stopped;
    playback_.source = PlaybackSource::None;
    playback_.progress_seconds = 0;
    prebuffer_sent_ = false;
    telemetry_.prebuffer_ready = false;
    return {command(PlaybackCommandType::StopOutput, std::move(reason))};
}

std::vector<PlaybackCommand> PlaybackEngine::next() {
    if (!has_next()) return stop("queue ended");
    current_index_ = next_index();
    return start_current("next track");
}

std::vector<PlaybackCommand> PlaybackEngine::previous() {
    if (queue_.empty()) return {};
    current_index_ = previous_index();
    return start_current("previous track");
}

std::vector<PlaybackCommand> PlaybackEngine::seek_relative(int seconds) {
    return seek_to(playback_.progress_seconds + seconds);
}

std::vector<PlaybackCommand> PlaybackEngine::seek_to(int seconds) {
    if (!has_current()) return {};
    const int duration = std::max(0, playback_.duration_seconds);
    playback_.progress_seconds = duration > 0
        ? std::clamp(seconds, 0, duration)
        : std::max(0, seconds);
    prebuffer_sent_ = false;
    telemetry_.prebuffer_ready = false;
    return {command(PlaybackCommandType::SeekTo, "seek")};
}

std::vector<PlaybackCommand> PlaybackEngine::set_volume_target(int volume) {
    target_volume_ = std::clamp(volume, 0, 100);
    if (output_volume_ == target_volume_) return {};
    return {};
}

std::vector<PlaybackCommand> PlaybackEngine::update(std::chrono::milliseconds delta) {
    std::vector<PlaybackCommand> out;
    if (playback_.state == PlaybackState::Playing) {
        const int before = playback_.progress_seconds;
        playback_.progress_seconds += static_cast<int>(delta.count() / 1000);
        if (delta.count() > 0 && playback_.progress_seconds == before && delta >= std::chrono::milliseconds(900)) {
            ++playback_.progress_seconds;
        }

        const int remaining = playback_.duration_seconds - playback_.progress_seconds;
        const int prebuffer_at = std::max(1, crossfade_seconds_ > 0 ? crossfade_seconds_ : 1);
        if (has_next() && playback_.duration_seconds > 0 && remaining <= prebuffer_at && !prebuffer_sent_) {
            prebuffer_sent_ = true;
            telemetry_.prebuffer_ready = true;
            PlaybackCommand prebuffer;
            prebuffer.type = PlaybackCommandType::PrebufferTrack;
            prebuffer.track = queue_[static_cast<size_t>(next_index())];
            prebuffer.value = crossfade_seconds_;
            prebuffer.reason = crossfade_seconds_ > 0 ? "crossfade prebuffer" : "gapless prebuffer";
            out.push_back(prebuffer);
        }

        if (playback_.duration_seconds > 0 && playback_.progress_seconds >= playback_.duration_seconds) {
            if (repeat_mode_ == RepeatMode::One) {
                out = start_current("repeat one");
            } else if (has_next()) {
                current_index_ = next_index();
                out = start_current(crossfade_seconds_ > 0 ? "crossfade transition" : "gapless transition");
            } else if (repeat_mode_ == RepeatMode::All && !queue_.empty()) {
                current_index_ = 0;
                out = start_current("repeat all");
            } else {
                auto stopped = stop("queue complete");
                out.insert(out.end(), stopped.begin(), stopped.end());
            }
        }
    }

    if (sleep_remaining_.count() > 0 && playback_.state == PlaybackState::Playing) {
        if (delta >= sleep_remaining_) {
            sleep_remaining_ = std::chrono::milliseconds(0);
            PlaybackCommand expired = command(PlaybackCommandType::SleepTimerExpired, "sleep timer expired");
            out.push_back(expired);
            auto stopped = stop("sleep timer");
            out.insert(out.end(), stopped.begin(), stopped.end());
        } else {
            sleep_remaining_ -= delta;
        }
    }

    volume_elapsed_ += delta;
    if (volume_elapsed_ >= std::chrono::milliseconds(40) && output_volume_ != target_volume_) {
        volume_elapsed_ = std::chrono::milliseconds(0);
        if (output_volume_ < target_volume_) {
            output_volume_ = std::min(target_volume_, output_volume_ + config_.volume_ramp_step);
        } else {
            output_volume_ = std::max(target_volume_, output_volume_ - config_.volume_ramp_step);
        }
        PlaybackCommand volume = command(PlaybackCommandType::SetVolume, "smooth volume ramp");
        volume.value = output_volume_;
        out.push_back(volume);
    }

    return out;
}

void PlaybackEngine::set_crossfade_seconds(int seconds) {
    const int allowed[] = {0, 1, 2, 3, 5};
    int best = 0;
    int best_distance = 100;
    for (int allowed_value : allowed) {
        const int distance = std::abs(seconds - allowed_value);
        if (distance < best_distance) {
            best = allowed_value;
            best_distance = distance;
        }
    }
    crossfade_seconds_ = std::min(best, config_.max_crossfade_seconds);
}

void PlaybackEngine::set_repeat_mode(RepeatMode mode) {
    repeat_mode_ = mode;
    playback_.repeat = mode != RepeatMode::Off;
}

void PlaybackEngine::set_sleep_timer_minutes(int minutes) {
    sleep_remaining_ = minutes <= 0
        ? std::chrono::milliseconds(0)
        : std::chrono::minutes(minutes);
}

void PlaybackEngine::set_low_power(bool low_power) {
    telemetry_.low_power = low_power;
    telemetry_.buffer_target_ms = low_power ? config_.low_power_buffer_ms : config_.normal_buffer_ms;
}

bool PlaybackEngine::request_focus(std::string owner) {
    if (telemetry_.focus_owner.empty() || telemetry_.focus_owner == owner || playback_.state == PlaybackState::Stopped) {
        telemetry_.focus_owner = std::move(owner);
        return true;
    }
    return false;
}

void PlaybackEngine::release_focus(std::string owner) {
    if (telemetry_.focus_owner == owner) {
        telemetry_.focus_owner = {};
    }
}

void PlaybackEngine::note_underrun() {
    ++telemetry_.underruns;
    telemetry_.prebuffer_ready = false;
}

void PlaybackEngine::note_recovery() {
    ++telemetry_.recoveries;
}

PlaybackSnapshot PlaybackEngine::snapshot() const {
    return playback_;
}

PlaybackEngineTelemetry PlaybackEngine::telemetry() const {
    return telemetry_;
}

const std::vector<Track>& PlaybackEngine::queue() const {
    return queue_;
}

int PlaybackEngine::current_index() const {
    return current_index_;
}

int PlaybackEngine::target_volume() const {
    return target_volume_;
}

RepeatMode PlaybackEngine::repeat_mode() const {
    return repeat_mode_;
}

int PlaybackEngine::crossfade_seconds() const {
    return crossfade_seconds_;
}

bool PlaybackEngine::has_current() const {
    return current_index_ >= 0 && current_index_ < static_cast<int>(queue_.size());
}

bool PlaybackEngine::has_next() const {
    if (queue_.empty()) return false;
    if (repeat_mode_ == RepeatMode::All) return true;
    return current_index_ + 1 < static_cast<int>(queue_.size());
}

int PlaybackEngine::next_index() const {
    if (queue_.empty()) return -1;
    if (current_index_ + 1 < static_cast<int>(queue_.size())) return current_index_ + 1;
    return repeat_mode_ == RepeatMode::All ? 0 : current_index_;
}

int PlaybackEngine::previous_index() const {
    if (queue_.empty()) return -1;
    if (playback_.progress_seconds > 3) return current_index_;
    if (current_index_ > 0) return current_index_ - 1;
    return repeat_mode_ == RepeatMode::All ? static_cast<int>(queue_.size()) - 1 : 0;
}

std::vector<PlaybackCommand> PlaybackEngine::start_current(std::string reason) {
    if (!has_current()) return {};
    playback_.state = PlaybackState::Playing;
    playback_.source = PlaybackSource::Local;
    playback_.track = queue_[static_cast<size_t>(current_index_)];
    playback_.progress_seconds = 0;
    playback_.duration_seconds = std::max(0, playback_.track.duration_seconds);
    playback_.queue_index = current_index_ + 1;
    playback_.queue_size = static_cast<int>(queue_.size());
    playback_.repeat = repeat_mode_ != RepeatMode::Off;
    prebuffer_sent_ = false;
    telemetry_.prebuffer_ready = false;
    PlaybackCommand play = command(PlaybackCommandType::PlayTrack, std::move(reason));
    play.track = playback_.track;
    return {play};
}

PlaybackCommand PlaybackEngine::command(PlaybackCommandType type, std::string reason) const {
    PlaybackCommand out;
    out.type = type;
    out.track = has_current() ? queue_[static_cast<size_t>(current_index_)] : Track{};
    out.value = type == PlaybackCommandType::SetVolume ? output_volume_ : playback_.progress_seconds;
    out.reason = std::move(reason);
    return out;
}

const char* to_string(RepeatMode mode) {
    switch (mode) {
        case RepeatMode::Off: return "Off";
        case RepeatMode::One: return "One";
        case RepeatMode::All: return "All";
    }
    return "Unknown";
}

const char* to_string(PlaybackCommandType type) {
    switch (type) {
        case PlaybackCommandType::None: return "None";
        case PlaybackCommandType::PlayTrack: return "PlayTrack";
        case PlaybackCommandType::PauseOutput: return "PauseOutput";
        case PlaybackCommandType::StopOutput: return "StopOutput";
        case PlaybackCommandType::SetVolume: return "SetVolume";
        case PlaybackCommandType::SeekTo: return "SeekTo";
        case PlaybackCommandType::PrebufferTrack: return "PrebufferTrack";
        case PlaybackCommandType::SleepTimerExpired: return "SleepTimerExpired";
    }
    return "Unknown";
}

}  // namespace shaer
