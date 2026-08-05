#include "animation_manager.hpp"

#include <algorithm>

namespace shaer {

void AnimationManager::register_animation(AnimationSpec spec) {
    auto existing = std::find_if(animations_.begin(), animations_.end(), [&](const AnimationSpec& item) {
        return item.id == spec.id;
    });
    if (existing != animations_.end()) {
        *existing = std::move(spec);
    } else {
        animations_.push_back(std::move(spec));
    }
}

AnimationBudget AnimationManager::budget_for(PowerMode mode) const {
    if (mode == PowerMode::Critical) {
        return {8, 2, false};
    }
    if (mode == PowerMode::BatterySaver) {
        return {12, 3, false};
    }
    return {30, 8, true};
}

std::vector<AnimationSpec> AnimationManager::active_for(PowerMode mode) const {
    const auto budget = budget_for(mode);
    std::vector<AnimationSpec> active;
    for (auto animation : animations_) {
        if (animation.essential || animation.power_cost <= budget.max_power_cost) {
            animation.fps = std::min(animation.fps, budget.fps);
            active.push_back(animation);
        }
    }
    std::sort(active.begin(), active.end(), [](const AnimationSpec& a, const AnimationSpec& b) {
        return a.priority > b.priority;
    });
    return active;
}

size_t AnimationManager::registered_count() const {
    return animations_.size();
}

}  // namespace shaer

