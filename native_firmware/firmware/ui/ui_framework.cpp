#include "ui_framework.hpp"

#include <algorithm>
#include <utility>

namespace shaer {

namespace {

constexpr int kWidth = 240;
constexpr int kHeight = 320;
constexpr int kStatusH = 10;
constexpr int kSidebarW = 34;
constexpr int kFooterH = 14;

UiColor color(ThemeColor value) {
    return {value.r, value.g, value.b};
}

bool theme_is(const RenderModel& model, const std::string& id) {
    return model.theme.id == id;
}

}  // namespace

int FontMetrics::glyph_width(int scale) {
    return std::max(1, scale) * 6;
}

int FontMetrics::glyph_height(int scale) {
    return std::max(1, scale) * 7;
}

int FontMetrics::text_width(const std::string& text, int scale) {
    return static_cast<int>(text.size()) * glyph_width(scale);
}

std::string FontMetrics::fit_text(const std::string& text, int max_width, int scale) {
    const int glyph = glyph_width(scale);
    if (glyph <= 0 || max_width <= 0) return {};
    const size_t max_chars = static_cast<size_t>(std::max(0, max_width / glyph));
    if (text.size() <= max_chars) return text;
    if (max_chars <= 3) return text.substr(0, max_chars);
    return text.substr(0, max_chars - 3) + "...";
}

UiFrame UiFramework::build_frame(const RenderModel& model) const {
    UiFrame frame;
    frame.screen = model.screen;
    frame.width = kWidth;
    frame.height = kHeight;

    add_rect(frame, {0, 0, kWidth, kHeight}, background_for(model));
    add_transition(frame, model.transition, accent_for(model));

    if (model.screen == Screen::Boot) {
        // Boot logo/loading is SHAeR OS chrome, shared by every theme.
        add_boot(frame, model);
        return frame;
    }

    add_status_bar(frame, model);
    add_footer(frame, model);

    switch (model.screen) {
        case Screen::Home:
            add_home(frame, model);
            break;
        case Screen::Library:
            add_library(frame, model);
            break;
        case Screen::NowPlaying:
            add_now_playing(frame, model);
            break;
        case Screen::Marginalia:
            add_marginalia(frame, model);
            break;
        case Screen::VoiceArchive:
            add_voice_archive(frame, model);
            break;
        case Screen::BluetoothConnect:
            add_bluetooth_connect(frame, model);
            break;
        case Screen::Settings:
            add_settings(frame, model);
            break;
        case Screen::About:
            add_about(frame, model);
            break;
        case Screen::Popup:
            add_popup(frame, model);
            break;
        case Screen::Sleep:
            add_power_screen(frame, model, "SLEEP", "DOUBLE PRESS WAKES");
            break;
        case Screen::Charging:
            add_charging_screen(frame, model);
            break;
        case Screen::Shutdown:
            add_power_screen(frame, model, "GOODBYE", "SHUTTING DOWN");
            break;
        case Screen::Aod:
            add_power_screen(frame, model, model.clock.time_12h, "ALWAYS ON");
            break;
        case Screen::Boot:
            break;
    }
    return frame;
}

UiColor UiFramework::background_for(const RenderModel& model) const {
    return color(model.theme.definition.palette.background);
}

UiColor UiFramework::panel_for(const RenderModel& model) const {
    return color(model.theme.definition.palette.popup);
}

UiColor UiFramework::accent_for(const RenderModel& model) const {
    return color(model.theme.definition.palette.accent);
}

UiColor UiFramework::text_for(const RenderModel& model) const {
    return color(model.theme.definition.palette.foreground);
}

UiColor UiFramework::muted_for(const RenderModel& model) const {
    return color(model.theme.definition.palette.disabled);
}

void UiFramework::add_rect(UiFrame& frame, UiRect rect, UiColor color_value) const {
    frame.commands.push_back({UiCommandType::Rect, rect, color_value, color_value, {}, 1, 0, 100, false});
}

void UiFramework::add_outline(UiFrame& frame, UiRect rect, UiColor color_value, int thickness) const {
    const int t = std::max(1, thickness);
    add_rect(frame, {rect.x, rect.y, rect.w, t}, color_value);
    add_rect(frame, {rect.x, rect.y + rect.h - t, rect.w, t}, color_value);
    add_rect(frame, {rect.x, rect.y, t, rect.h}, color_value);
    add_rect(frame, {rect.x + rect.w - t, rect.y, t, rect.h}, color_value);
}

void UiFramework::add_text(UiFrame& frame, int x, int y, int max_width, std::string text, UiColor color_value, int scale, bool selected) const {
    frame.commands.push_back({
        UiCommandType::Text,
        {x, y, max_width, FontMetrics::glyph_height(scale)},
        color_value,
        {},
        FontMetrics::fit_text(text, max_width, scale),
        scale,
        0,
        100,
        selected,
    });
}

void UiFramework::add_icon(UiFrame& frame, int x, int y, std::string icon, UiColor color_value, bool selected) const {
    frame.commands.push_back({UiCommandType::Icon, {x, y, 18, 18}, color_value, {}, std::move(icon), 1, 0, 100, selected});
}

void UiFramework::add_progress(UiFrame& frame, UiRect rect, int value, int max_value, UiColor fg, UiColor bg) const {
    frame.commands.push_back({UiCommandType::Progress, rect, fg, bg, {}, 1, value, std::max(1, max_value), false});
}

void UiFramework::add_transition(UiFrame& frame, const TransitionPlan& transition, UiColor color_value) const {
    if (transition.duration_ms <= 0) return;
    const int width = std::clamp(transition.duration_ms / 3, 8, kWidth);
    frame.commands.push_back({UiCommandType::Transition, {0, kStatusH, width, 2}, color_value, {}, transition.style, 1, transition.duration_ms, 600, false});
}

void UiFramework::add_status_bar(UiFrame& frame, const RenderModel& model) const {
    const auto bg = color(model.theme.definition.palette.status_bar);
    const auto fg = text_for(model);
    add_rect(frame, {0, 0, kWidth, kStatusH}, bg);
    add_text(frame, 2, 2, 104, to_string(model.screen), fg, 1);
    add_text(frame, 192, 2, 44, model.clock.valid ? model.clock.time_12h : "--:--", fg, 1);
}

void UiFramework::add_sidebar(UiFrame& frame, const RenderModel& model) const {
    const auto panel = panel_for(model);
    const auto fg = text_for(model);
    const auto muted = muted_for(model);
    const auto& icons = model.theme.definition.icons;
    add_rect(frame, {0, kStatusH, kSidebarW, kHeight - kStatusH - kFooterH}, panel);
    add_icon(frame, 8, 36, icons.home, model.screen == Screen::Home ? accent_for(model) : fg, model.screen == Screen::Home);
    add_icon(frame, 8, 72, icons.library, model.screen == Screen::Library ? accent_for(model) : fg, model.screen == Screen::Library);
    add_icon(frame, 8, 108, icons.now_playing, model.screen == Screen::NowPlaying ? accent_for(model) : fg, model.screen == Screen::NowPlaying);
    add_icon(frame, 8, 144, icons.settings, model.screen == Screen::Settings ? accent_for(model) : muted, model.screen == Screen::Settings);
    add_icon(frame, 8, 180, icons.about, model.screen == Screen::About ? accent_for(model) : muted, model.screen == Screen::About);
}

void UiFramework::add_footer(UiFrame& frame, const RenderModel& model) const {
    add_rect(frame, {0, kHeight - kFooterH, kWidth, kFooterH}, color(model.theme.definition.palette.footer));
    add_text(frame, 4, kHeight - 10, 108, model.clock.date_label.empty() ? "17mn June 2026" : model.clock.date_label, text_for(model), 1);
    add_text(frame, 202, kHeight - 10, 30, std::to_string(model.battery_percent) + "%", model.battery_percent <= 15 ? color(model.theme.definition.palette.warning) : accent_for(model), 1);
}

void UiFramework::add_menu_items(UiFrame& frame, const RenderModel& model, int start_y, const std::vector<std::string>& items) const {
    const auto fg = accent_for(model);
    const auto selection = color(model.theme.definition.palette.selection);
    for (size_t i = 0; i < items.size(); ++i) {
        const int y = start_y + static_cast<int>(i) * 16;
        const bool selected = static_cast<int>(i) == model.selected_index;
        if (selected) {
            add_rect(frame, {34, y - 1, 146, 9}, selection);
        }
        add_text(frame, 36, y, 166, items[i], selected ? background_for(model) : fg, 1, selected);
    }
}

void UiFramework::add_theme_chrome(UiFrame& frame, const RenderModel& model, std::string title, UiColor bg, UiColor fg, UiColor bar, UiColor accent) const {
    add_rect(frame, {0, 0, kWidth, kHeight}, bg);
    add_rect(frame, {0, 0, kWidth, 12}, bar);
    add_rect(frame, {0, kHeight - 14, kWidth, 14}, bar);
    add_text(frame, 4, 3, 108, std::move(title), fg, 1);
    add_text(frame, 184, 3, 50, model.clock.valid ? model.clock.time_12h : "12:15 PM", fg, 1);
    add_text(frame, 4, kHeight - 10, 120, model.clock.date_label.empty() ? "12/AUG/26/13:00" : model.clock.date_label, fg, 1);
    add_text(frame, 202, kHeight - 10, 34, std::to_string(model.battery_percent) + "%", accent, 1);
}

void UiFramework::add_theme_menu(
    UiFrame& frame,
    const RenderModel& model,
    int x,
    int y,
    int w,
    int row_h,
    const std::vector<std::string>& items,
    UiColor fg,
    UiColor selection,
    UiColor selected_fg) const {
    for (size_t i = 0; i < items.size(); ++i) {
        const int row_y = y + static_cast<int>(i) * row_h;
        const bool selected = static_cast<int>(i) == model.selected_index;
        if (selected) {
            add_rect(frame, {x - 2, row_y - 1, w + 4, std::max(9, row_h - 2)}, selection);
        }
        add_text(frame, x, row_y, w, items[i], selected ? selected_fg : fg, 1, selected);
    }
}

void UiFramework::add_theme_panels(UiFrame& frame, UiColor line, int density) const {
    const int step = std::max(12, density);
    for (int y = 34; y < 284; y += step) {
        add_rect(frame, {24, y, 192, 1}, line);
    }
    for (int x = 34; x < 216; x += step + 8) {
        add_rect(frame, {x, 34, 1, 238}, line);
    }
}

bool UiFramework::add_themed_boot(UiFrame& frame, const RenderModel& model) const {
    if (theme_is(model, "bombay_ticket")) {
        const UiColor paper{226, 219, 198};
        const UiColor ink{24, 24, 20};
        const UiColor blue{30, 67, 104};
        add_rect(frame, {0, 0, kWidth, kHeight}, paper);
        add_text(frame, 6, 284, 90, "LOADING", ink, 1);
        add_rect(frame, {34, 40, 172, 216}, {235, 229, 210});
        add_rect(frame, {72, 66, 96, 96}, blue);
        add_rect(frame, {88, 84, 64, 58}, paper);
        add_rect(frame, {104, 72, 32, 78}, blue);
        add_text(frame, 100, 190, 64, "MHM MHM", ink, 1);
        add_text(frame, 88, 204, 84, "D DRIVE UPDATING", ink, 1);
        add_progress(frame, {72, 226, 96, 6}, model.battery_percent, 100, ink, {190, 182, 160});
        return true;
    }
    if (theme_is(model, "japanese_punk")) {
        const UiColor pink{238, 0, 94};
        const UiColor cream{232, 226, 210};
        add_rect(frame, {0, 0, kWidth, kHeight}, {7, 7, 10});
        add_text(frame, 24, 44, 120, "LOADING...", cream, 2);
        add_rect(frame, {46, 98, 138, 112}, {24, 20, 24});
        add_rect(frame, {68, 118, 92, 62}, cream);
        add_rect(frame, {90, 92, 36, 96}, pink);
        add_text(frame, 58, 228, 110, "D DRIVE LOADING", pink, 1);
        add_progress(frame, {50, 250, 140, 6}, model.battery_percent, 100, pink, {42, 42, 42});
        return true;
    }
    if (theme_is(model, "windows_xp")) {
        const UiColor sky{74, 147, 232};
        const UiColor grass{70, 150, 50};
        const UiColor panel{211, 211, 203};
        add_rect(frame, {0, 0, kWidth, 210}, sky);
        add_rect(frame, {0, 210, kWidth, 110}, grass);
        add_rect(frame, {56, 94, 128, 92}, panel);
        add_rect(frame, {56, 94, 128, 14}, {36, 87, 205});
        add_text(frame, 84, 122, 70, "Loading...", {0, 0, 0}, 1);
        add_progress(frame, {80, 146, 80, 8}, model.battery_percent, 100, {36, 87, 205}, {160, 160, 160});
        add_text(frame, 72, 166, 40, "OK", {120, 120, 120}, 1);
        add_text(frame, 122, 166, 54, "Cancel", {0, 0, 0}, 1);
        return true;
    }
    if (theme_is(model, "ghibli_garden")) {
        add_rect(frame, {0, 0, kWidth, kHeight}, {102, 151, 164});
        add_rect(frame, {0, 190, kWidth, 130}, {88, 116, 72});
        add_rect(frame, {42, 32, 156, 236}, {218, 174, 112});
        add_rect(frame, {54, 48, 132, 204}, {122, 153, 150});
        add_rect(frame, {74, 76, 92, 92}, {220, 76, 44});
        add_text(frame, 84, 192, 80, "mhm mhm", {240, 226, 200}, 1);
        add_text(frame, 68, 210, 112, "Shaer loading", {240, 226, 200}, 1);
        return true;
    }
    if (theme_is(model, "indian_raga")) {
        const UiColor navy{2, 1, 38};
        const UiColor ivory{226, 214, 184};
        const UiColor blue{28, 62, 104};
        add_rect(frame, {0, 0, kWidth, kHeight}, navy);
        add_rect(frame, {36, 24, 168, 272}, ivory);
        add_rect(frame, {54, 48, 132, 198}, {218, 196, 156});
        add_rect(frame, {72, 76, 96, 130}, blue);
        add_rect(frame, {88, 92, 64, 96}, ivory);
        add_text(frame, 96, 214, 54, "MHM", navy, 1);
        add_text(frame, 78, 230, 88, "D DRIVE LOADING", navy, 1);
        add_progress(frame, {78, 252, 84, 6}, model.battery_percent, 100, blue, {136, 128, 122});
        return true;
    }
    return false;
}

bool UiFramework::add_themed_home(UiFrame& frame, const RenderModel& model) const {
    const std::vector<std::string> items = {"LOCAL MUSIC", "SPOTIFY CONNECT", "VOICE MEMOS", "SETTINGS"};
    if (theme_is(model, "bombay_ticket")) {
        const UiColor paper{226, 219, 198};
        const UiColor ink{18, 18, 14};
        const UiColor red{178, 60, 50};
        add_theme_chrome(frame, model, "HOME", paper, ink, {205, 199, 182}, red);
        add_rect(frame, {32, 34, 176, 220}, {238, 232, 212});
        add_theme_panels(frame, {150, 145, 130}, 24);
        add_text(frame, 44, 48, 78, "MENU BAR", ink, 1);
        add_theme_menu(frame, model, 44, 102, 128, 24, {"SPOTIFY CONNECT", "LOCAL FILES", "SETTINGS", "MEMO"}, ink, red, paper);
        add_text(frame, 80, 236, 74, "SHAER", ink, 1);
        return true;
    }
    if (theme_is(model, "japanese_punk")) {
        const UiColor pink{238, 0, 94};
        const UiColor cream{232, 226, 210};
        add_theme_chrome(frame, model, "MP-001", {7, 7, 10}, cream, {12, 12, 16}, pink);
        add_text(frame, 22, 34, 118, "LIBRARY", cream, 2);
        add_rect(frame, {22, 66, 188, 176}, {14, 13, 20});
        add_theme_menu(frame, model, 36, 84, 150, 28, items, cream, pink, {7, 7, 10});
        add_theme_panels(frame, {52, 30, 42}, 28);
        return true;
    }
    if (theme_is(model, "windows_xp")) {
        add_rect(frame, {0, 0, kWidth, 212}, {84, 154, 232});
        add_rect(frame, {0, 212, kWidth, 108}, {70, 150, 50});
        add_rect(frame, {28, 42, 122, 184}, {225, 225, 216});
        add_rect(frame, {28, 42, 122, 16}, {235, 235, 230});
        add_text(frame, 38, 50, 70, "Menu Bar", {0, 0, 0}, 1);
        add_theme_menu(frame, model, 44, 82, 88, 28, items, {0, 0, 0}, {36, 87, 205}, {255, 255, 255});
        add_rect(frame, {0, 296, kWidth, 24}, {20, 91, 215});
        add_text(frame, 10, 304, 50, "SHAeR", {0, 0, 0}, 1);
        return true;
    }
    if (theme_is(model, "ghibli_garden")) {
        add_theme_chrome(frame, model, "Menu", {88, 132, 142}, {244, 232, 206}, {70, 94, 82}, {244, 232, 206});
        add_rect(frame, {0, 130, kWidth, 190}, {74, 114, 82});
        add_rect(frame, {44, 64, 152, 178}, {116, 132, 116});
        add_theme_menu(frame, model, 68, 96, 104, 30, items, {248, 238, 218}, {144, 164, 146}, {255, 255, 255});
        add_text(frame, 76, 264, 86, "SHAER", {248, 238, 218}, 1);
        return true;
    }
    if (theme_is(model, "indian_raga")) {
        const UiColor navy{2, 1, 38};
        const UiColor ivory{236, 228, 204};
        const UiColor lavender{91, 73, 138};
        add_theme_chrome(frame, model, "HOME", navy, ivory, {8, 18, 62}, lavender);
        add_rect(frame, {26, 30, 188, 250}, {12, 24, 78});
        add_theme_menu(frame, model, 60, 104, 120, 24, items, ivory, ivory, navy);
        add_text(frame, 90, 244, 60, "SHAER", ivory, 1);
        return true;
    }
    return false;
}

bool UiFramework::add_themed_library(UiFrame& frame, const RenderModel& model) const {
    std::vector<std::string> items = {"Liked Songs", "Playlist 1", "Playlist 2", "Blend 1", "Album 1"};
    if (!model.local_library.empty()) {
        items.clear();
        const int offset = std::max(0, model.selected_index - 3);
        for (size_t i = static_cast<size_t>(offset); i < model.local_library.size() && items.size() < 6; ++i) {
            items.push_back(model.local_library[i].title);
        }
    }
    if (theme_is(model, "bombay_ticket")) {
        const UiColor paper{226, 219, 198};
        const UiColor ink{18, 18, 14};
        const UiColor red{178, 60, 50};
        add_theme_chrome(frame, model, "LIBRARY", paper, ink, {205, 199, 182}, red);
        add_rect(frame, {22, 34, 196, 232}, {238, 232, 212});
        add_theme_panels(frame, {144, 140, 126}, 22);
        add_theme_menu(frame, model, 42, 58, 148, 28, items, ink, {214, 200, 178}, red);
        return true;
    }
    if (theme_is(model, "japanese_punk")) {
        const UiColor pink{238, 0, 94};
        const UiColor cream{232, 226, 210};
        add_theme_chrome(frame, model, "MP-002", {7, 7, 10}, cream, {12, 12, 16}, pink);
        add_text(frame, 26, 46, 120, "LIBRARY", cream, 1);
        add_theme_menu(frame, model, 38, 82, 150, 28, items, cream, pink, {0, 0, 0});
        add_rect(frame, {200, 44, 6, 196}, pink);
        return true;
    }
    if (theme_is(model, "windows_xp")) {
        add_themed_home(frame, model);
        add_rect(frame, {52, 58, 136, 178}, {225, 225, 216});
        add_text(frame, 64, 72, 80, "My Music", {0, 0, 0}, 1);
        add_theme_menu(frame, model, 68, 102, 92, 26, items, {0, 0, 0}, {36, 87, 205}, {255, 255, 255});
        return true;
    }
    if (theme_is(model, "ghibli_garden")) {
        add_theme_chrome(frame, model, "Folders", {98, 138, 142}, {244, 232, 206}, {70, 94, 82}, {244, 232, 206});
        add_rect(frame, {36, 50, 168, 220}, {116, 132, 116});
        add_theme_menu(frame, model, 58, 82, 126, 28, items, {248, 238, 218}, {144, 164, 146}, {255, 255, 255});
        return true;
    }
    if (theme_is(model, "indian_raga")) {
        add_theme_chrome(frame, model, "LIBRARY", {82, 0, 0}, {248, 230, 206}, {82, 0, 0}, {248, 230, 206});
        add_rect(frame, {36, 42, 168, 236}, {92, 0, 0});
        add_theme_menu(frame, model, 62, 90, 116, 28, items, {248, 230, 206}, {248, 230, 206}, {82, 0, 0});
        return true;
    }
    return false;
}

bool UiFramework::add_themed_now_playing(UiFrame& frame, const RenderModel& model) const {
    const std::string title = model.playback.track.title.empty() ? "SONG NAME" : model.playback.track.title;
    const std::string artist = model.playback.track.artist.empty() ? "ARTIST" : model.playback.track.artist;
    const int progress = std::max(0, model.playback.progress_seconds);
    const int duration = std::max(1, model.playback.duration_seconds);
    if (theme_is(model, "bombay_ticket")) {
        const UiColor paper{226, 219, 198};
        const UiColor red{190, 70, 60};
        const UiColor ink{18, 18, 14};
        add_theme_chrome(frame, model, "PLAYING", paper, ink, {205, 199, 182}, red);
        add_rect(frame, {36, 42, 168, 226}, {238, 232, 212});
        add_rect(frame, {52, 70, 84, 84}, red);
        add_rect(frame, {58, 76, 72, 72}, paper);
        add_text(frame, 146, 76, 42, "SONG", ink, 1);
        add_text(frame, 146, 92, 42, "ARTIST", ink, 1);
        add_text(frame, 48, 178, 132, title, ink, 1);
        add_progress(frame, {50, 216, 136, 4}, progress, duration, red, {176, 166, 148});
        add_text(frame, 76, 232, 96, "<<  <  >  >>", ink, 1);
        return true;
    }
    if (theme_is(model, "japanese_punk")) {
        const UiColor pink{238, 0, 94};
        const UiColor cream{232, 226, 210};
        add_theme_chrome(frame, model, "TRACK 07", {7, 7, 10}, cream, {12, 12, 16}, pink);
        add_rect(frame, {24, 42, 92, 116}, {22, 20, 24});
        add_text(frame, 36, 72, 68, "TRACK", pink, 1);
        add_text(frame, 128, 60, 74, title, cream, 1);
        add_text(frame, 128, 82, 74, artist, pink, 1);
        add_progress(frame, {126, 190, 84, 8}, progress, duration, pink, {44, 44, 44});
        add_text(frame, 54, 230, 134, "<<  ||  >>", cream, 1);
        return true;
    }
    if (theme_is(model, "windows_xp")) {
        add_rect(frame, {0, 0, kWidth, 212}, {84, 154, 232});
        add_rect(frame, {0, 212, kWidth, 108}, {70, 150, 50});
        add_rect(frame, {50, 46, 140, 202}, {225, 225, 216});
        add_rect(frame, {50, 46, 140, 16}, {36, 87, 205});
        add_text(frame, 58, 50, 80, "SHAeR Player", {255, 255, 255}, 1);
        add_rect(frame, {70, 76, 72, 72}, {200, 174, 118});
        add_text(frame, 66, 158, 112, title, {0, 0, 0}, 1);
        add_text(frame, 66, 174, 112, artist, {0, 0, 0}, 1);
        add_progress(frame, {66, 202, 104, 6}, progress, duration, {36, 87, 205}, {160, 160, 160});
        return true;
    }
    if (theme_is(model, "ghibli_garden")) {
        add_theme_chrome(frame, model, "Playing", {96, 142, 154}, {244, 232, 206}, {70, 94, 82}, {244, 232, 206});
        add_rect(frame, {0, 132, kWidth, 188}, {86, 126, 104});
        add_rect(frame, {42, 54, 156, 214}, {144, 156, 142});
        add_rect(frame, {68, 82, 72, 72}, {224, 90, 64});
        add_text(frame, 74, 174, 98, title, {248, 238, 218}, 1);
        add_text(frame, 82, 194, 82, artist, {248, 238, 218}, 1);
        add_progress(frame, {66, 222, 108, 6}, progress, duration, {248, 238, 218}, {82, 104, 92});
        return true;
    }
    if (theme_is(model, "indian_raga")) {
        add_theme_chrome(frame, model, "PLAYING", {190, 90, 118}, {255, 235, 220}, {190, 90, 118}, {255, 235, 220});
        add_rect(frame, {40, 38, 160, 240}, {206, 112, 136});
        add_rect(frame, {72, 62, 96, 116}, {220, 134, 154});
        add_text(frame, 70, 196, 100, title, {255, 235, 220}, 1);
        add_progress(frame, {66, 226, 108, 6}, progress, duration, {255, 235, 220}, {126, 54, 70});
        return true;
    }
    return false;
}

bool UiFramework::add_themed_voice_archive(UiFrame& frame, const RenderModel& model) const {
    if (theme_is(model, "bombay_ticket")) {
        add_theme_chrome(frame, model, "MEMOS", {226, 219, 198}, {18, 18, 14}, {205, 199, 182}, {30, 67, 104});
        add_rect(frame, {74, 64, 92, 150}, {238, 232, 212});
        add_rect(frame, {94, 84, 28, 88}, {190, 70, 60});
        add_rect(frame, {124, 110, 26, 50}, {18, 18, 14});
        add_theme_menu(frame, model, 54, 240, 132, 20, {"RECORD", "PLAY", "DONE"}, {18, 18, 14}, {30, 67, 104}, {226, 219, 198});
        return true;
    }
    if (theme_is(model, "japanese_punk")) {
        add_theme_chrome(frame, model, "VOICE MEMOS", {7, 7, 10}, {232, 226, 210}, {12, 12, 16}, {238, 0, 94});
        add_rect(frame, {48, 74, 144, 76}, {232, 226, 210});
        add_rect(frame, {64, 94, 112, 28}, {42, 30, 34});
        add_text(frame, 54, 168, 132, "PRESS O TO RECORD", {232, 226, 210}, 1);
        add_theme_menu(frame, model, 72, 230, 90, 22, {"RECORD", "PLAY", "DONE"}, {232, 226, 210}, {238, 0, 94}, {0, 0, 0});
        return true;
    }
    if (theme_is(model, "windows_xp")) {
        add_themed_now_playing(frame, model);
        add_rect(frame, {66, 82, 108, 96}, {255, 255, 255});
        add_text(frame, 80, 116, 78, "VLC MEMO", {0, 0, 0}, 1);
        return true;
    }
    if (theme_is(model, "ghibli_garden")) {
        add_theme_chrome(frame, model, "Memo", {100, 142, 154}, {244, 232, 206}, {70, 94, 82}, {244, 232, 206});
        add_rect(frame, {42, 52, 156, 218}, {144, 156, 142});
        add_text(frame, 76, 96, 92, "Voice Memo", {248, 238, 218}, 1);
        add_rect(frame, {86, 134, 68, 28}, {248, 238, 218});
        add_theme_menu(frame, model, 64, 218, 108, 20, {"RECORD", "PLAY", "DONE"}, {248, 238, 218}, {116, 132, 116}, {255, 255, 255});
        return true;
    }
    if (theme_is(model, "indian_raga")) {
        add_theme_chrome(frame, model, "MEMOS", {42, 68, 72}, {230, 212, 188}, {32, 58, 62}, {230, 212, 188});
        add_rect(frame, {56, 46, 128, 210}, {62, 82, 84});
        add_text(frame, 76, 90, 90, "Voice Memo", {230, 212, 188}, 1);
        add_rect(frame, {90, 130, 60, 60}, {122, 98, 82});
        return true;
    }
    return false;
}

bool UiFramework::add_themed_bluetooth(UiFrame& frame, const RenderModel& model) const {
    UiColor bg = background_for(model);
    UiColor fg = text_for(model);
    UiColor accent = accent_for(model);
    if (theme_is(model, "bombay_ticket")) { bg = {226, 219, 198}; fg = {18, 18, 14}; accent = {30, 67, 104}; }
    if (theme_is(model, "japanese_punk")) { bg = {7, 7, 10}; fg = {232, 226, 210}; accent = {238, 0, 94}; }
    if (theme_is(model, "windows_xp")) { bg = {84, 154, 232}; fg = {0, 0, 0}; accent = {36, 87, 205}; }
    if (theme_is(model, "ghibli_garden")) { bg = {96, 142, 154}; fg = {244, 232, 206}; accent = {144, 156, 142}; }
    if (theme_is(model, "indian_raga")) { bg = {2, 1, 38}; fg = {236, 228, 204}; accent = {91, 73, 138}; }
    add_theme_chrome(frame, model, "BLUETOOTH", bg, fg, bg, accent);
    add_rect(frame, {70, 88, 100, 100}, accent);
    add_rect(frame, {82, 100, 76, 76}, bg);
    add_text(frame, 94, 120, 60, "BT", fg, 3);
    add_text(frame, 42, 218, 158, "SPOTIFY CONNECT", fg, 1);
    add_text(frame, 38, 240, 166, "PRESS OK WHEN PHONE APPEARS", fg, 1);
    return true;
}

bool UiFramework::add_themed_settings(UiFrame& frame, const RenderModel& model) const {
    std::vector<std::string> items;
    if (!model.settings_ui.inside_category) {
        for (const auto& category : model.settings_ui.categories) items.push_back(category.title);
    }
    if (items.empty()) items = {"Appearance", "Audio", "Playback", "Connectivity", "Storage", "Power", "Device", "Privacy", "Advanced", "About"};
    if (theme_is(model, "bombay_ticket")) {
        add_theme_chrome(frame, model, "SETTING", {226, 219, 198}, {18, 18, 14}, {205, 199, 182}, {190, 70, 60});
        add_rect(frame, {28, 34, 184, 236}, {238, 232, 212});
        add_theme_panels(frame, {150, 145, 130}, 22);
        add_theme_menu(frame, model, 48, 56, 140, 20, items, {18, 18, 14}, {190, 70, 60}, {226, 219, 198});
        return true;
    }
    if (theme_is(model, "japanese_punk")) {
        add_theme_chrome(frame, model, "MP-002", {7, 7, 10}, {232, 226, 210}, {12, 12, 16}, {238, 0, 94});
        add_theme_menu(frame, model, 38, 54, 154, 22, items, {232, 226, 210}, {238, 0, 94}, {0, 0, 0});
        return true;
    }
    if (theme_is(model, "windows_xp")) {
        add_themed_home(frame, model);
        add_rect(frame, {60, 52, 126, 188}, {225, 225, 216});
        add_text(frame, 72, 66, 70, "Settings", {0, 0, 0}, 1);
        add_theme_menu(frame, model, 76, 94, 90, 22, items, {0, 0, 0}, {36, 87, 205}, {255, 255, 255});
        return true;
    }
    if (theme_is(model, "ghibli_garden")) {
        add_theme_chrome(frame, model, "Settings", {88, 118, 92}, {244, 232, 206}, {70, 94, 82}, {144, 156, 142});
        add_rect(frame, {48, 52, 150, 218}, {116, 132, 116});
        add_theme_menu(frame, model, 68, 78, 108, 22, items, {248, 238, 218}, {144, 164, 146}, {255, 255, 255});
        return true;
    }
    if (theme_is(model, "indian_raga")) {
        add_theme_chrome(frame, model, "SETTINGS", {14, 10, 10}, {236, 212, 176}, {14, 10, 10}, {92, 32, 4});
        add_rect(frame, {40, 44, 160, 226}, {40, 24, 18});
        add_theme_menu(frame, model, 64, 68, 110, 20, items, {236, 212, 176}, {236, 212, 176}, {40, 24, 18});
        return true;
    }
    return false;
}

bool UiFramework::add_themed_about(UiFrame& frame, const RenderModel& model) const {
    add_themed_settings(frame, model);
    add_text(frame, 62, 238, 120, "FW " + model.device_info.firmware_version, text_for(model), 1);
    return true;
}

bool UiFramework::add_themed_popup(UiFrame& frame, const RenderModel& model) const {
    if (theme_is(model, "archive_dark")) return false;
    const UiColor asset_bg = panel_for(model);
    const UiColor asset_fg = text_for(model);
    const UiColor asset_border = accent_for(model);
    add_rect(frame, {28, 104, 184, 112}, asset_border);
    add_rect(frame, {34, 110, 172, 100}, asset_bg);
    add_text(frame, 52, 132, 140, model.popup.title.empty() ? "NOTICE" : model.popup.title, asset_fg, 1);
    add_text(frame, 52, 158, 140, model.popup.body.empty() ? "PRESS OK" : model.popup.body, muted_for(model), 1);
    add_text(frame, 58, 188, 120, "OK / HOLD CANCEL", asset_border, 1);
    return true;
}

void UiFramework::add_boot(UiFrame& frame, const RenderModel& model) const {
    const auto fg = text_for(model);
    add_rect(frame, {0, 0, kWidth, kHeight}, background_for(model));
    add_rect(frame, {54, 74, 7, 7}, fg);
    add_rect(frame, {78, 62, 4, 4}, fg);
    add_rect(frame, {108, 26, 5, 5}, fg);
    add_rect(frame, {170, 72, 7, 7}, fg);
    add_rect(frame, {154, 162, 5, 5}, fg);
    add_rect(frame, {102, 186, 4, 4}, fg);
    add_rect(frame, {92, 208, 6, 6}, fg);
    add_rect(frame, {62, 79, 18, 2}, muted_for(model));
    add_rect(frame, {82, 64, 25, 2}, muted_for(model));
    add_rect(frame, {112, 31, 56, 2}, muted_for(model));
    add_rect(frame, {166, 80, 2, 80}, muted_for(model));
    add_rect(frame, {106, 188, 46, 2}, muted_for(model));
    add_text(frame, 82, 262, 76, "SHAER", fg, 1);
    add_text(frame, 58, 276, 122, "POWERED BY ADI-VASI", fg, 1);
}

void UiFramework::add_home(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_home(frame, model)) return;
    add_menu_items(frame, model, 92, {"[0] LOCAL FILES", "[1] SPOTIFY CONNECT", "[2] VOICE MEMOS", "[3] SETTINGS"});
}

void UiFramework::add_library(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_library(frame, model)) return;
    add_text(frame, 56, 42, 150, "MY MUSIC", accent_for(model), 1);
    if (model.local_library.empty()) {
        add_menu_items(frame, model, 92, {"[0] LOCAL FILES", "[1] LIKED SONGS", "[2] PLAYLIST 1", "[3] BLEND 1", "[4] PLAYLIST 2", "[5] ALBUM 1"});
        return;
    }
    std::vector<std::string> items;
    const int offset = std::max(0, model.selected_index - 3);
    for (size_t i = static_cast<size_t>(offset); i < model.local_library.size() && items.size() < 6; ++i) {
        items.push_back(model.local_library[i].title);
    }
    RenderModel adjusted = model;
    adjusted.selected_index = model.selected_index - offset;
    add_menu_items(frame, adjusted, 78, items);
}

void UiFramework::add_now_playing(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_now_playing(frame, model)) {
        add_icon(frame, 214, 32, model.theme.definition.icons.marginalia, accent_for(model));
        return;
    }
    const auto fg = text_for(model);
    const auto accent = accent_for(model);
    add_text(frame, 50, 48, 100, "NOW PLAYING", fg, 1);
    add_rect(frame, {20, 78, 92, 102}, color(model.theme.definition.palette.border));
    add_rect(frame, {24, 82, 84, 94}, muted_for(model));
    add_text(frame, 128, 90, 80, "MY QUEUE", accent, 1);
    add_text(frame, 128, 112, 80, "1. TRACK 1", accent, 1);
    add_text(frame, 128, 128, 80, "2. TRACK 2", accent, 1);
    add_text(frame, 128, 144, 80, "3. TRACK 3", accent, 1);
    add_text(frame, 28, 198, 168, model.playback.track.title.empty() ? "SONGNAME-------" : model.playback.track.title, accent, 1);
    add_text(frame, 28, 214, 168, model.playback.track.artist.empty() ? "ARTIST" : model.playback.track.artist, accent, 1);
    add_progress(
        frame,
        {28, 234, 168, 6},
        std::max(0, model.playback.progress_seconds),
        std::max(1, model.playback.duration_seconds),
        color(model.theme.definition.palette.progress_foreground),
        color(model.theme.definition.palette.progress_background));
    add_text(frame, 58, 252, 126, "<<  <  O  >  >>", accent, 1);
    add_icon(frame, 214, 32, model.theme.definition.icons.marginalia, accent);
}

void UiFramework::add_marginalia(UiFrame& frame, const RenderModel& model) const {
    // Marginalia entry can be themed, but the notebook surface is a neutral
    // white drawing workspace shared by all themes.
    const UiColor paper{255, 255, 255};
    const UiColor ink{36, 36, 40};
    const UiColor toolbar{244, 244, 246};
    const UiColor line{174, 178, 184};
    const UiColor accent{42, 92, 160};
    add_rect(frame, {18, 28, 204, 252}, paper);
    add_outline(frame, {18, 28, 204, 252}, line, 1);
    add_rect(frame, {26, 38, 188, 28}, toolbar);
    add_outline(frame, {26, 38, 188, 28}, line, 1);
    add_text(frame, 34, 47, 88, "MARGINALIA", ink, 1);
    add_text(frame, 156, 47, 48, "PAGE " + std::to_string(std::max(1, model.selected_index + 1)), ink, 1);
    add_text(frame, 34, 82, 166, model.playback.track.title.empty() ? "SONG NOTEBOOK" : model.playback.track.title, ink, 1);
    add_text(frame, 34, 98, 166, model.playback.track.artist, line, 1);
    add_outline(frame, {34, 122, 172, 112}, line, 1);
    add_text(frame, 46, 138, 144, "DRAWING SURFACE", line, 1);
    add_rect(frame, {34, 246, 172, 24}, toolbar);
    add_text(frame, 44, 254, 150, "PEN   ERASE   UNDO   SAVE", accent, 1);
}

void UiFramework::add_voice_archive(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_voice_archive(frame, model)) return;
    add_rect(frame, {62, 48, 116, 168}, color(model.theme.definition.palette.popup));
    add_text(frame, 88, 72, 72, "VOICE", muted_for(model), 1);
    add_text(frame, 78, 92, 92, "MEMORY", muted_for(model), 1);
    add_rect(frame, {100, 120, 40, 2}, muted_for(model));
    add_rect(frame, {118, 96, 2, 92}, muted_for(model));
    add_rect(frame, {86, 142, 66, 2}, muted_for(model));
    add_menu_items(frame, model, 244, {"RECORD", "PLAY", "DONE"});
}

void UiFramework::add_bluetooth_connect(UiFrame& frame, const RenderModel&) const {
    // Pairing/connectivity is SHAeR OS chrome, not a theme surface.
    const UiColor bg{16, 16, 24};
    const UiColor fg{243, 240, 238};
    const UiColor blue{74, 143, 239};
    add_rect(frame, {24, 30, 192, 242}, bg);
    add_outline(frame, {24, 30, 192, 242}, {60, 58, 68}, 1);
    add_text(frame, 38, 44, 164, "PHONE CONNECT", fg, 1);
    add_rect(frame, {70, 82, 100, 100}, {24, 32, 48});
    add_outline(frame, {70, 82, 100, 100}, blue, 1);
    add_text(frame, 94, 112, 60, "BT", blue, 3);
    add_text(frame, 42, 214, 158, "SPOTIFY CONNECT", fg, 1);
    add_text(frame, 38, 236, 166, "PRESS OK WHEN PHONE APPEARS", {184, 178, 176}, 1);
}

void UiFramework::add_settings(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_settings(frame, model)) return;
    std::vector<std::string> items;
    if (!model.settings_ui.inside_category) {
        for (const auto& category : model.settings_ui.categories) {
            items.push_back(category.title);
        }
        if (items.empty()) {
            items = {"APPEARANCE", "AUDIO", "POWER", "DEVICE", "ABOUT"};
        }
        add_menu_items(frame, model, 58, items);
        return;
    }

    const int category_index = std::clamp(
        model.settings_ui.category_index,
        0,
        std::max(0, static_cast<int>(model.settings_ui.categories.size()) - 1));
    if (model.settings_ui.categories.empty()) return;
    const auto& category = model.settings_ui.categories[static_cast<size_t>(category_index)];
    add_text(frame, 38, 34, 168, category.title, muted_for(model), 1);
    const int offset = std::max(0, model.settings_ui.item_index - 4);
    for (size_t i = static_cast<size_t>(offset); i < category.items.size() && items.size() < 6; ++i) {
        const auto& row = category.items[i];
        items.push_back(row.label + ": " + row.value);
    }
    RenderModel adjusted = model;
    adjusted.selected_index = model.settings_ui.item_index - offset;
    add_menu_items(frame, adjusted, 58, items);
}

void UiFramework::add_popup(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_popup(frame, model)) return;
    const auto& spacing = model.theme.definition.spacing;
    const int x = (kWidth - spacing.popup_width) / 2;
    const int y = (kHeight - spacing.popup_height) / 2;
    add_rect(frame, {x, y, spacing.popup_width, spacing.popup_height}, color(model.theme.definition.palette.border));
    add_rect(frame, {x + 6, y + 6, spacing.popup_width - 12, spacing.popup_height - 12}, color(model.theme.definition.palette.popup));
    add_text(frame, x + 20, y + 24, spacing.popup_width - 40, model.popup.title.empty() ? "NOTICE" : model.popup.title, text_for(model), 1);
    add_rect(frame, {x + 20, y + 48, spacing.popup_width - 40, 1}, color(model.theme.definition.palette.border));
    add_text(frame, x + 20, y + 66, spacing.popup_width - 40, model.popup.body.empty() ? "PRESS OK" : model.popup.body, muted_for(model), 1);
    add_text(frame, x + 24, y + spacing.popup_height - 32, 144, "OK  HOLD=CANCEL", accent_for(model), 1, true);
}

void UiFramework::add_about(UiFrame& frame, const RenderModel& model) const {
    if (add_themed_about(frame, model)) return;
    add_rect(frame, {72, 34, 96, 234}, color(model.theme.definition.palette.popup));
    add_text(frame, 90, 54, 64, "SHAER", accent_for(model), 1);
    add_text(frame, 82, 80, 84, "001101", text_for(model), 1);
    add_text(frame, 82, 98, 84, "010010", text_for(model), 1);
    add_text(frame, 82, 116, 84, "ARCHIV", text_for(model), 1);
    add_text(frame, 82, 134, 84, "101101", text_for(model), 1);
    add_text(frame, 82, 164, 84, "ADI OS", muted_for(model), 1);
    add_text(frame, 82, 184, 84, "FW " + model.device_info.firmware_version, muted_for(model), 1);
}

void UiFramework::add_power_screen(UiFrame& frame, const RenderModel& model, const std::string& title, const std::string& body) const {
    add_text(frame, 62, 118, 140, title, text_for(model), 2);
    add_text(frame, 54, 152, 154, body, muted_for(model), 1);
}

void UiFramework::add_charging_screen(UiFrame& frame, const RenderModel& model) const {
    const std::string& id = model.theme.id;
    if (id == "archive_dark") return add_archive_charging(frame, model);
    if (id == "bombay_ticket") return add_bombay_charging(frame, model);
    if (id == "japanese_punk") return add_japanese_punk_charging(frame, model);
    if (id == "windows_xp") return add_windows_xp_charging(frame, model);
    if (id == "ghibli_garden") return add_ghibli_charging(frame, model);
    if (id == "indian_raga") return add_indian_raga_charging(frame, model);
    add_power_screen(frame, model, "CHARGING", "BATTERY " + std::to_string(model.battery_percent) + "%");
}

void UiFramework::add_archive_charging(UiFrame& frame, const RenderModel& model) const {
    add_rect(frame, {0, 0, kWidth, kHeight}, background_for(model));
    const int step = (model.tick_count / 10) % 4;
    const int x = 84 + step * 10;
    const int leg = (model.tick_count / 12) % 2;
    add_text(frame, 70, 58, 112, "SHAER", text_for(model), 2);
    add_text(frame, 50, 88, 150, "POWERED BY ADI-VASI", muted_for(model), 1);
    add_rect(frame, {x + 12, 130, 10, 10}, accent_for(model));
    add_rect(frame, {x + 16, 140, 2, 34}, accent_for(model));
    add_rect(frame, {x, 148, 36, 2}, accent_for(model));
    add_rect(frame, {x + 16, 174, leg ? 20 : 10, 2}, accent_for(model));
    add_rect(frame, {x + (leg ? 6 : 16), 184, leg ? 12 : 20, 2}, accent_for(model));
    add_text(frame, 56, 224, 130, "ARCHIVE WALK", muted_for(model), 1);
    add_progress(frame, {50, 252, 140, 6}, model.battery_percent, 100, accent_for(model), panel_for(model));
    add_text(frame, 94, 270, 60, std::to_string(model.battery_percent) + "%", accent_for(model), 1);
}

void UiFramework::add_bombay_charging(UiFrame& frame, const RenderModel& model) const {
    const UiColor paper{225, 218, 197};
    const UiColor ink{28, 30, 24};
    const UiColor red{176, 63, 48};
    const UiColor blue{28, 63, 104};
    add_rect(frame, {0, 0, kWidth, kHeight}, paper);
    for (int i = 0; i < kWidth; i += 14) {
        add_rect(frame, {i, 2, 8, 2}, blue);
        add_rect(frame, {i, kHeight - 4, 8, 2}, blue);
    }
    for (int y = 8; y < kHeight - 8; y += 14) {
        add_rect(frame, {2, y, 2, 8}, blue);
        add_rect(frame, {kWidth - 4, y, 2, 8}, blue);
    }
    add_rect(frame, {76, 126, 76, 88}, {235, 226, 199});
    add_rect(frame, {92, 146, 44, 54}, red);
    add_rect(frame, {102, 158, 24, 28}, paper);
    add_rect(frame, {136, 158, 18, 6}, red);
    add_rect(frame, {148, 164, 6, 22}, red);
    add_rect(frame, {136, 186, 18, 6}, red);
    add_rect(frame, {92, 200, 44, 6}, ink);
    const int steam = (model.tick_count / 15) % 18;
    add_text(frame, 80, 72, 96, model.battery_percent >= 100 ? "HOGYA" : "THAND RAKH", red, 2);
    add_rect(frame, {86, 120 - steam, 6, 18}, red);
    add_rect(frame, {112, 104 + steam / 2, 6, 22}, red);
    add_rect(frame, {140, 118 - steam / 2, 6, 16}, red);
    add_progress(frame, {50, 250, 140, 7}, model.battery_percent, 100, red, {194, 184, 162});
    add_text(frame, 88, 270, 72, std::to_string(model.battery_percent) + "%", ink, 1);
}

void UiFramework::add_japanese_punk_charging(UiFrame& frame, const RenderModel& model) const {
    const UiColor black{8, 8, 12};
    const UiColor red{228, 0, 92};
    const UiColor cream{232, 226, 210};
    add_rect(frame, {0, 0, kWidth, kHeight}, black);
    add_rect(frame, {34, 44, 172, 232}, {14, 13, 20});
    add_text(frame, 44, 54, 90, "MP-002", red, 1);
    add_text(frame, 50, 82, 130, "NIGHT CHASING", cream, 1);
    add_rect(frame, {58, 112, 112, 58}, {42, 30, 34});
    add_rect(frame, {74, 138, 84, 12}, {210, 76, 32});
    add_rect(frame, {62, 156, 24, 8}, cream);
    add_rect(frame, {142, 156, 24, 8}, cream);
    add_text(frame, 58, 206, 124, "NO CHARGE MOTION", red, 1);
    add_text(frame, 66, 228, 106, std::to_string(model.battery_percent) + "% BATTERY", cream, 1);
}

void UiFramework::add_windows_xp_charging(UiFrame& frame, const RenderModel& model) const {
    const UiColor blue{38, 87, 205};
    const UiColor green{74, 200, 54};
    const UiColor gray{211, 211, 203};
    add_rect(frame, {0, 0, kWidth, kHeight}, {0, 0, 0});
    add_text(frame, 66, 106, 90, "SHAeR", {255, 255, 255}, 2);
    add_rect(frame, {146, 100, 36, 24}, {238, 58, 42});
    add_rect(frame, {182, 100, 36, 24}, green);
    add_rect(frame, {146, 124, 36, 24}, blue);
    add_rect(frame, {182, 124, 36, 24}, {246, 210, 55});
    add_text(frame, 58, 152, 130, "powered by ADI-VASI", {255, 255, 255}, 1);
    add_rect(frame, {72, 236, 104, 12}, gray);
    add_progress(frame, {76, 240, 96, 4}, model.battery_percent, 100, green, {36, 36, 36});
}

void UiFramework::add_ghibli_charging(UiFrame& frame, const RenderModel& model) const {
    const UiColor water{42, 86, 88};
    const UiColor moss{48, 90, 55};
    const UiColor koi{235, 92, 42};
    const UiColor cream{236, 222, 192};
    add_rect(frame, {0, 0, kWidth, kHeight}, water);
    add_rect(frame, {0, 0, 42, kHeight}, moss);
    add_rect(frame, {198, 0, 42, kHeight}, moss);
    const int swim = (model.tick_count / 8) % 86;
    const int x1 = 42 + swim;
    const int y1 = 82 + ((model.tick_count / 18) % 26);
    const int x2 = 158 - swim / 2;
    const int y2 = 184 - ((model.tick_count / 20) % 34);
    add_rect(frame, {x1, y1, 32, 12}, koi);
    add_rect(frame, {x1 + 24, y1 + 4, 16, 4}, cream);
    add_rect(frame, {x2, y2, 30, 12}, cream);
    add_rect(frame, {x2 + 22, y2 + 4, 16, 4}, koi);
    add_text(frame, 72, 234, 98, "CHARGING", cream, 1);
    add_progress(frame, {58, 254, 124, 6}, model.battery_percent, 100, cream, {28, 62, 62});
}

void UiFramework::add_indian_raga_charging(UiFrame& frame, const RenderModel& model) const {
    const UiColor parchment{222, 197, 158};
    const UiColor umber{92, 32, 4};
    add_rect(frame, {0, 0, kWidth, kHeight}, parchment);
    add_text(frame, 76, 70, 96, "RAGA", umber, 2);
    add_rect(frame, {118, 112, 4, 88}, umber);
    add_rect(frame, {96, 154, 48, 4}, umber);
    add_rect(frame, {88, 204, 64, 4}, umber);
    add_text(frame, 64, 236, 126, "CHARGING " + std::to_string(model.battery_percent) + "%", umber, 1);
}

}  // namespace shaer
