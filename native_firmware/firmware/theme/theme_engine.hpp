#pragma once

#include "types.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace shaer {

class ThemeAssets {
public:
    explicit ThemeAssets(std::string fallback_root = "assets/themes/default");
    ThemeResources load_active(const ThemeDefinition& definition) const;
    std::string resolve_asset(const ThemeResources& resources, const std::string& relative_path) const;

private:
    std::string fallback_root_;
};

class ThemeRegistry {
public:
    ThemeRegistry();
    // Registers a package only when its manifest satisfies SDK V1.
    bool register_theme(ThemeDefinition definition);
    ThemeValidationResult validate(const ThemeDefinition& definition) const;
    ThemeDefinition resolve(const std::string& id) const;
    std::vector<std::string> available_ids() const;
    bool contains(const std::string& id) const;

private:
    std::map<std::string, ThemeDefinition> definitions_;
};

class ThemeRenderer {
public:
    ThemeRenderProfile render_profile(const ThemeDefinition& definition) const;
    ScreenBlueprint screen_blueprint(const ThemeDefinition& definition, Screen screen) const;
    TransitionPlan transition_plan(
        const ThemeDefinition& definition,
        Screen from,
        Screen to,
        bool blocks_input,
        std::string reason) const;
    AnimationPolicy animation_policy(const ThemeDefinition& definition) const;
};

class ThemeManager {
public:
    ThemeManager();
    const ThemeDefinition& active_definition() const;
    void set_active(std::string id);
    void cycle_active();
    std::vector<std::string> available_theme_ids() const;
    ThemeRenderProfile render_profile() const;
    ScreenBlueprint screen_blueprint(Screen screen) const;
    TransitionPlan transition_plan(Screen from, Screen to, bool blocks_input, std::string reason) const;
    AnimationPolicy animation_policy() const;
    const ThemeResources& active_resources() const;
    size_t active_asset_count() const;
    ThemeValidationResult validation() const;

private:
    ThemeDefinition load_pack_or_default(const std::string& id) const;
    std::optional<ThemeDefinition> load_profile_from_pack(const std::string& id) const;
    ThemeRegistry registry_;
    ThemeRenderer renderer_;
    ThemeAssets assets_;
    ThemeDefinition active_;
    ThemeResources active_resources_;
};

class ThemeEngine {
public:
    const ThemeDefinition& active_theme() const;
    const ThemeDefinition& active_definition() const;
    ThemeRenderProfile render_profile() const;
    ScreenBlueprint screen_blueprint(Screen screen) const;
    TransitionPlan transition_plan(Screen from, Screen to, bool blocks_input, std::string reason) const;
    AnimationPolicy animation_policy() const;
    void set_theme(std::string id);
    void cycle_theme();
    std::vector<std::string> available_theme_ids() const;
    const ThemeResources& active_resources() const;
    size_t active_asset_count() const;
    ThemeValidationResult validation() const;

private:
    ThemeManager manager_;
};

}  // namespace shaer
