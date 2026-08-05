#pragma once

#include "event_bus.hpp"

namespace shaer {

class InputManager {
public:
    Event map(InputAction action) const;
};

}  // namespace shaer

