#include "sdl_event_translation.h"

#include <SDL3/SDL_mouse.h>
#include <algorithm>
#include <cstdint>

namespace huynity::adapters::window {
namespace {

[[nodiscard]] std::optional<MouseButton> mapMouseButton(Uint8 button) noexcept {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return MouseButton::Left;
    case SDL_BUTTON_RIGHT:
        return MouseButton::Right;
    case SDL_BUTTON_MIDDLE:
        return MouseButton::Middle;
    case SDL_BUTTON_X1:
        return MouseButton::Button4;
    case SDL_BUTTON_X2:
        return MouseButton::Button5;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::uint32_t nonnegativeSize(Sint32 value) noexcept {
    return static_cast<std::uint32_t>(std::max<Sint32>(value, 0));
}

} // namespace

KeyCode mapScancode(SDL_Scancode scancode) noexcept {
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        const auto offset = static_cast<int>(scancode) - static_cast<int>(SDL_SCANCODE_A);
        return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + offset);
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        const auto offset = static_cast<int>(scancode) - static_cast<int>(SDL_SCANCODE_F1);
        return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + offset);
    }

    switch (scancode) {
    case SDL_SCANCODE_1:
        return KeyCode::Num1;
    case SDL_SCANCODE_2:
        return KeyCode::Num2;
    case SDL_SCANCODE_3:
        return KeyCode::Num3;
    case SDL_SCANCODE_4:
        return KeyCode::Num4;
    case SDL_SCANCODE_5:
        return KeyCode::Num5;
    case SDL_SCANCODE_6:
        return KeyCode::Num6;
    case SDL_SCANCODE_7:
        return KeyCode::Num7;
    case SDL_SCANCODE_8:
        return KeyCode::Num8;
    case SDL_SCANCODE_9:
        return KeyCode::Num9;
    case SDL_SCANCODE_0:
        return KeyCode::Num0;
    case SDL_SCANCODE_SPACE:
        return KeyCode::Space;
    case SDL_SCANCODE_ESCAPE:
        return KeyCode::Escape;
    case SDL_SCANCODE_RETURN:
        return KeyCode::Enter;
    case SDL_SCANCODE_TAB:
        return KeyCode::Tab;
    case SDL_SCANCODE_BACKSPACE:
        return KeyCode::Backspace;
    case SDL_SCANCODE_INSERT:
        return KeyCode::Insert;
    case SDL_SCANCODE_DELETE:
        return KeyCode::Delete;
    case SDL_SCANCODE_LEFT:
        return KeyCode::Left;
    case SDL_SCANCODE_RIGHT:
        return KeyCode::Right;
    case SDL_SCANCODE_UP:
        return KeyCode::Up;
    case SDL_SCANCODE_DOWN:
        return KeyCode::Down;
    case SDL_SCANCODE_PAGEUP:
        return KeyCode::PageUp;
    case SDL_SCANCODE_PAGEDOWN:
        return KeyCode::PageDown;
    case SDL_SCANCODE_HOME:
        return KeyCode::Home;
    case SDL_SCANCODE_END:
        return KeyCode::End;
    case SDL_SCANCODE_CAPSLOCK:
        return KeyCode::CapsLock;
    case SDL_SCANCODE_SCROLLLOCK:
        return KeyCode::ScrollLock;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return KeyCode::NumLock;
    case SDL_SCANCODE_PRINTSCREEN:
        return KeyCode::PrintScreen;
    case SDL_SCANCODE_PAUSE:
        return KeyCode::Pause;
    case SDL_SCANCODE_LSHIFT:
        return KeyCode::LeftShift;
    case SDL_SCANCODE_LCTRL:
        return KeyCode::LeftControl;
    case SDL_SCANCODE_LALT:
        return KeyCode::LeftAlt;
    case SDL_SCANCODE_LGUI:
        return KeyCode::LeftSuper;
    case SDL_SCANCODE_RSHIFT:
        return KeyCode::RightShift;
    case SDL_SCANCODE_RCTRL:
        return KeyCode::RightControl;
    case SDL_SCANCODE_RALT:
        return KeyCode::RightAlt;
    case SDL_SCANCODE_RGUI:
        return KeyCode::RightSuper;
    default:
        return KeyCode::Unknown;
    }
}

std::optional<InputEvent> translateSdlEvent(const SDL_Event& event, PixelScale pixelScale) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return InputEvent{WindowCloseRequestedEvent{}};
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        return InputEvent{WindowResizedEvent{
            .width = nonnegativeSize(event.window.data1),
            .height = nonnegativeSize(event.window.data2),
        }};
    case SDL_EVENT_KEY_DOWN:
        return InputEvent{KeyPressedEvent{
            .key = mapScancode(event.key.scancode),
            .isRepeat = event.key.repeat,
        }};
    case SDL_EVENT_KEY_UP:
        return InputEvent{KeyReleasedEvent{.key = mapScancode(event.key.scancode)}};
    case SDL_EVENT_MOUSE_MOTION:
        return InputEvent{MouseMovedEvent{
            .x = static_cast<double>(event.motion.x) * pixelScale.x,
            .y = static_cast<double>(event.motion.y) * pixelScale.y,
            .deltaX = static_cast<double>(event.motion.xrel) * pixelScale.x,
            .deltaY = static_cast<double>(event.motion.yrel) * pixelScale.y,
        }};
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        const auto button = mapMouseButton(event.button.button);
        if (!button) {
            return std::nullopt;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            return InputEvent{MouseButtonPressedEvent{
                .button = *button,
                .x = static_cast<double>(event.button.x) * pixelScale.x,
                .y = static_cast<double>(event.button.y) * pixelScale.y,
            }};
        }
        return InputEvent{MouseButtonReleasedEvent{
            .button = *button,
            .x = static_cast<double>(event.button.x) * pixelScale.x,
            .y = static_cast<double>(event.button.y) * pixelScale.y,
        }};
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        const double direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0 : 1.0;
        return InputEvent{MouseScrolledEvent{
            .deltaX = static_cast<double>(event.wheel.x) * direction,
            .deltaY = static_cast<double>(event.wheel.y) * direction,
        }};
    }
    default:
        return std::nullopt;
    }
}

} // namespace huynity::adapters::window
