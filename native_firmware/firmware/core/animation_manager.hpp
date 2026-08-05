#pragma once

#include "types.hpp"

#include <string>
#include <vector>

namespace shaer {

struct AnimationSpec {
    std::string id;
    int duration_ms = 0;
    std::string ease = "linear";
    bool loop = false;
    int power_cost = 1;
    int fps = 30;
    int priority = 0;
    bool essential = false;
};

struct AnimationBudget {
    int fps = 30;
    int max_power_cost = 8;
    bool background_enabled = true;
};

class AnimationManager {
public:
    void register_animation(AnimationSpec spec);
    AnimationBudget budget_for(PowerMode mode) const;
    std::vector<AnimationSpec> active_for(PowerMode mode) const;
    size_t registered_count() const;

private:
    std::vector<AnimationSpec> animations_;
};

}  // namespace shaer

