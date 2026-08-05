#pragma once

#include "types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace shaer {

struct UiColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct UiRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

enum class UiCommandType {
    Rect,
    Text,
    Icon,
    Progress,
    Transition,
    Image,
};

struct UiCommand {
    UiCommandType type = UiCommandType::Text;
    UiRect rect;
    UiColor fg;
    UiColor bg;
    std::string text;
    int scale = 1;
    int value = 0;
    int max_value = 100;
    bool selected = false;
};

struct UiFrame {
    int width = 240;
    int height = 320;
    Screen screen = Screen::Boot;
    std::vector<UiCommand> commands;
};

class FontMetrics {
public:
    static int glyph_width(int scale);
    static int glyph_height(int scale);
    static int text_width(const std::string& text, int scale);
    static std::string fit_text(const std::string& text, int max_width, int scale);
};

class UiFramework {
public:
    UiFrame build_frame(const RenderModel& model) const;

private:
    UiColor background_for(const RenderModel& model) const;
    UiColor panel_for(const RenderModel& model) const;
    UiColor accent_for(const RenderModel& model) const;
    UiColor text_for(const RenderModel& model) const;
    UiColor muted_for(const RenderModel& model) const;

    void add_rect(UiFrame& frame, UiRect rect, UiColor color) const;
    void add_outline(UiFrame& frame, UiRect rect, UiColor color, int thickness = 1) const;
    void add_text(UiFrame& frame, int x, int y, int max_width, std::string text, UiColor color, int scale = 1, bool selected = false) const;
    void add_icon(UiFrame& frame, int x, int y, std::string icon, UiColor color, bool selected = false) const;
    void add_progress(UiFrame& frame, UiRect rect, int value, int max_value, UiColor fg, UiColor bg) const;
    void add_transition(UiFrame& frame, const TransitionPlan& transition, UiColor color) const;
    void add_status_bar(UiFrame& frame, const RenderModel& model) const;
    void add_sidebar(UiFrame& frame, const RenderModel& model) const;
    void add_footer(UiFrame& frame, const RenderModel& model) const;
    void add_menu_items(UiFrame& frame, const RenderModel& model, int start_y, const std::vector<std::string>& items) const;
    void add_theme_chrome(UiFrame& frame, const RenderModel& model, std::string title, UiColor bg, UiColor fg, UiColor bar, UiColor accent) const;
    void add_theme_menu(UiFrame& frame, const RenderModel& model, int x, int y, int w, int row_h, const std::vector<std::string>& items, UiColor fg, UiColor selection, UiColor selected_fg) const;
    void add_theme_panels(UiFrame& frame, UiColor line, int density) const;
    bool add_themed_boot(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_home(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_library(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_now_playing(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_voice_archive(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_bluetooth(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_settings(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_about(UiFrame& frame, const RenderModel& model) const;
    bool add_themed_popup(UiFrame& frame, const RenderModel& model) const;
    void add_boot(UiFrame& frame, const RenderModel& model) const;
    void add_home(UiFrame& frame, const RenderModel& model) const;
    void add_library(UiFrame& frame, const RenderModel& model) const;
    void add_now_playing(UiFrame& frame, const RenderModel& model) const;
    void add_marginalia(UiFrame& frame, const RenderModel& model) const;
    void add_voice_archive(UiFrame& frame, const RenderModel& model) const;
    void add_bluetooth_connect(UiFrame& frame, const RenderModel& model) const;
    void add_settings(UiFrame& frame, const RenderModel& model) const;
    void add_about(UiFrame& frame, const RenderModel& model) const;
    void add_popup(UiFrame& frame, const RenderModel& model) const;
    void add_power_screen(UiFrame& frame, const RenderModel& model, const std::string& title, const std::string& body) const;
    void add_charging_screen(UiFrame& frame, const RenderModel& model) const;
    void add_archive_charging(UiFrame& frame, const RenderModel& model) const;
    void add_bombay_charging(UiFrame& frame, const RenderModel& model) const;
    void add_japanese_punk_charging(UiFrame& frame, const RenderModel& model) const;
    void add_windows_xp_charging(UiFrame& frame, const RenderModel& model) const;
    void add_ghibli_charging(UiFrame& frame, const RenderModel& model) const;
    void add_indian_raga_charging(UiFrame& frame, const RenderModel& model) const;
};

}  // namespace shaer
