#pragma once

#include "hal.hpp"
#include "ui_framework.hpp"

#include <chrono>
#include <cstdint>
#include <string>

namespace shaer {

struct PiPinMap {
    int power_button = 4;
    int encoder_a = 5;
    int encoder_b = 6;
    int encoder_push = 7;
    int button_back = 12;
    int button_play = 13;
    int button_options = 16;
    int display_cs = 8;
    int display_dc = 25;
    int display_reset = 24;
    int display_backlight = 26;
};

struct PiDisplayConfig {
    std::string spi_device = "/dev/spidev0.0";
    int width = 240;
    int height = 320;
    int speed_hz = 1000000;
    PiPinMap pins;
};

class PiDisplay final : public Display {
public:
    explicit PiDisplay(PiDisplayConfig config = {});
    ~PiDisplay() override;
    void render(const RenderModel& model) override;
    void run_diagnostic_pattern();
    void run_solid_color_loop();
    bool ready() const;

private:
    void initialize();
    void reset_panel();
    void command(uint8_t value);
    void data(const uint8_t* values, size_t length);
    void data(uint8_t value);
    void set_window(int x0, int y0, int x1, int y1);
    void fill_rect(int x, int y, int w, int h, uint16_t color);
    void fill_screen(uint16_t color);
    uint16_t color_for(UiColor color) const;
    void draw_command(const UiCommand& command);
    void draw_icon(const UiCommand& command);
    void draw_image(const UiCommand& command);
    void draw_progress(const UiCommand& command);
    uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) const;
    void draw_text(int x, int y, const std::string& text, uint16_t color, int scale = 2);
    void draw_char(int x, int y, char c, uint16_t color, int scale);
    void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
    void draw_checkerboard(int cell);

    PiDisplayConfig config_;
    UiFramework ui_;
    int spi_fd_ = -1;
    bool ready_ = false;
    bool warned_ = false;
    bool verbose_spi_ = false;
};

class PiAudioOutput final : public AudioOutput {
public:
    void play_local(const Track& track) override;
    void play_spotify(const Track& track) override;
    void set_volume(int volume) override;
    void stop() override;
    void pause() override;
    void resume() override;
    void seek_seconds(int seconds) override;
    void prebuffer_local(const Track& track) override;
    void set_crossfade_seconds(int seconds) override;

private:
    int volume_ = 50;
    bool paused_ = false;
    int crossfade_seconds_ = 0;
};

class PiBattery final : public Battery {
public:
    int percent() const override;
    bool is_charging() const override;
};

class PiBluetooth final : public Bluetooth {
public:
    bool connected() const override;
};

class PiInput final : public Input {
public:
    explicit PiInput(PiPinMap pins = {});
    InputAction next_action() override;
    InputAction poll_action() override;

private:
    bool pressed(int gpio) const;
    InputAction scan_buttons();
    InputAction scan_encoder();
    InputAction scan_power();

    PiPinMap pins_;
    int last_a_ = 1;
    int last_b_ = 1;
    bool confirm_latched_ = false;
    bool confirm_long_sent_ = false;
    std::chrono::steady_clock::time_point confirm_down_at_;
    bool back_latched_ = false;
    bool play_latched_ = false;
    bool options_latched_ = false;
    bool power_latched_ = false;
    int power_press_count_ = 0;
    std::chrono::steady_clock::time_point power_down_at_;
    std::chrono::steady_clock::time_point last_power_release_;
};

void pi_export_gpio(int gpio);
void pi_set_gpio_direction(int gpio, const std::string& direction);
void pi_write_gpio(int gpio, int value);
int pi_read_gpio(int gpio, int fallback = 1);

}  // namespace shaer
