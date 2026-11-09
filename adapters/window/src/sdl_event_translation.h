#pragma once

#include "huynity/adapters/window/input_events.h"

#include <SDL3/SDL_events.h>
#include <optional>

namespace huynity::adapters::window {

struct PixelScale {
    double x{1.0};
    double y{1.0};
};

[[nodiscard]] KeyCode mapScancode(SDL_Scancode scancode) noexcept;
[[nodiscard]] std::optional<InputEvent> translateSdlEvent(const SDL_Event& event, PixelScale pixelScale = {});

} // namespace huynity::adapters::window
