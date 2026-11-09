#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "sdl_event_translation.h"

namespace window = huynity::adapters::window;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

namespace {

void testScancodeSequences() {
    for (int offset = 0; offset < 26; ++offset) {
        const auto scancode = static_cast<SDL_Scancode>(static_cast<int>(SDL_SCANCODE_A) + offset);
        CHECK(window::mapScancode(scancode) ==
              static_cast<window::KeyCode>(static_cast<int>(window::KeyCode::A) + offset));
    }
    for (int offset = 0; offset < 12; ++offset) {
        const auto scancode = static_cast<SDL_Scancode>(static_cast<int>(SDL_SCANCODE_F1) + offset);
        CHECK(window::mapScancode(scancode) ==
              static_cast<window::KeyCode>(static_cast<int>(window::KeyCode::F1) + offset));
    }

    constexpr std::array digits{
        std::pair{SDL_SCANCODE_0, window::KeyCode::Num0}, std::pair{SDL_SCANCODE_1, window::KeyCode::Num1},
        std::pair{SDL_SCANCODE_2, window::KeyCode::Num2}, std::pair{SDL_SCANCODE_3, window::KeyCode::Num3},
        std::pair{SDL_SCANCODE_4, window::KeyCode::Num4}, std::pair{SDL_SCANCODE_5, window::KeyCode::Num5},
        std::pair{SDL_SCANCODE_6, window::KeyCode::Num6}, std::pair{SDL_SCANCODE_7, window::KeyCode::Num7},
        std::pair{SDL_SCANCODE_8, window::KeyCode::Num8}, std::pair{SDL_SCANCODE_9, window::KeyCode::Num9},
    };
    for (const auto& [scancode, expected] : digits) {
        CHECK(window::mapScancode(scancode) == expected);
    }
}

void testNamedScancodes() {
    constexpr std::array mappings{
        std::pair{SDL_SCANCODE_SPACE, window::KeyCode::Space},
        std::pair{SDL_SCANCODE_ESCAPE, window::KeyCode::Escape},
        std::pair{SDL_SCANCODE_RETURN, window::KeyCode::Enter},
        std::pair{SDL_SCANCODE_TAB, window::KeyCode::Tab},
        std::pair{SDL_SCANCODE_BACKSPACE, window::KeyCode::Backspace},
        std::pair{SDL_SCANCODE_INSERT, window::KeyCode::Insert},
        std::pair{SDL_SCANCODE_DELETE, window::KeyCode::Delete},
        std::pair{SDL_SCANCODE_LEFT, window::KeyCode::Left},
        std::pair{SDL_SCANCODE_RIGHT, window::KeyCode::Right},
        std::pair{SDL_SCANCODE_UP, window::KeyCode::Up},
        std::pair{SDL_SCANCODE_DOWN, window::KeyCode::Down},
        std::pair{SDL_SCANCODE_PAGEUP, window::KeyCode::PageUp},
        std::pair{SDL_SCANCODE_PAGEDOWN, window::KeyCode::PageDown},
        std::pair{SDL_SCANCODE_HOME, window::KeyCode::Home},
        std::pair{SDL_SCANCODE_END, window::KeyCode::End},
        std::pair{SDL_SCANCODE_CAPSLOCK, window::KeyCode::CapsLock},
        std::pair{SDL_SCANCODE_SCROLLLOCK, window::KeyCode::ScrollLock},
        std::pair{SDL_SCANCODE_NUMLOCKCLEAR, window::KeyCode::NumLock},
        std::pair{SDL_SCANCODE_PRINTSCREEN, window::KeyCode::PrintScreen},
        std::pair{SDL_SCANCODE_PAUSE, window::KeyCode::Pause},
        std::pair{SDL_SCANCODE_LSHIFT, window::KeyCode::LeftShift},
        std::pair{SDL_SCANCODE_LCTRL, window::KeyCode::LeftControl},
        std::pair{SDL_SCANCODE_LALT, window::KeyCode::LeftAlt},
        std::pair{SDL_SCANCODE_LGUI, window::KeyCode::LeftSuper},
        std::pair{SDL_SCANCODE_RSHIFT, window::KeyCode::RightShift},
        std::pair{SDL_SCANCODE_RCTRL, window::KeyCode::RightControl},
        std::pair{SDL_SCANCODE_RALT, window::KeyCode::RightAlt},
        std::pair{SDL_SCANCODE_RGUI, window::KeyCode::RightSuper},
    };
    for (const auto& [scancode, expected] : mappings) {
        CHECK(window::mapScancode(scancode) == expected);
    }
    CHECK(window::mapScancode(SDL_SCANCODE_UNKNOWN) == window::KeyCode::Unknown);
    CHECK(window::mapScancode(SDL_SCANCODE_MEDIA_NEXT_TRACK) == window::KeyCode::Unknown);
}

void testKeyboardEvents() {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_D;
    event.key.repeat = true;
    auto translated = window::translateSdlEvent(event);
    CHECK(translated.has_value());
    CHECK(std::holds_alternative<window::KeyPressedEvent>(*translated));
    const auto pressed = std::get<window::KeyPressedEvent>(*translated);
    CHECK(pressed.key == window::KeyCode::D);
    CHECK(pressed.isRepeat);

    event.type = SDL_EVENT_KEY_UP;
    event.key.scancode = SDL_SCANCODE_UNKNOWN;
    translated = window::translateSdlEvent(event);
    CHECK(translated.has_value());
    CHECK(std::holds_alternative<window::KeyReleasedEvent>(*translated));
    CHECK(std::get<window::KeyReleasedEvent>(*translated).key == window::KeyCode::Unknown);
}

void testMouseEvents() {
    constexpr window::PixelScale scale{2.0, 1.5};
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 10.0F;
    event.motion.y = 20.0F;
    event.motion.xrel = -3.0F;
    event.motion.yrel = 4.0F;
    auto translated = window::translateSdlEvent(event, scale);
    CHECK(translated.has_value());
    const auto moved = std::get<window::MouseMovedEvent>(*translated);
    CHECK(moved.x == 20.0);
    CHECK(moved.y == 30.0);
    CHECK(moved.deltaX == -6.0);
    CHECK(moved.deltaY == 6.0);

    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_X1;
    event.button.x = 8.0F;
    event.button.y = 12.0F;
    translated = window::translateSdlEvent(event, scale);
    CHECK(translated.has_value());
    const auto pressed = std::get<window::MouseButtonPressedEvent>(*translated);
    CHECK(pressed.button == window::MouseButton::Button4);
    CHECK(pressed.x == 16.0);
    CHECK(pressed.y == 18.0);

    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = SDL_BUTTON_RIGHT;
    translated = window::translateSdlEvent(event, scale);
    CHECK(translated.has_value());
    CHECK(std::get<window::MouseButtonReleasedEvent>(*translated).button == window::MouseButton::Right);

    event.button.button = 99;
    CHECK(!window::translateSdlEvent(event, scale).has_value());

    event = SDL_Event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.x = 2.0F;
    event.wheel.y = -3.0F;
    event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    translated = window::translateSdlEvent(event);
    CHECK(translated.has_value());
    const auto scrolled = std::get<window::MouseScrolledEvent>(*translated);
    CHECK(scrolled.deltaX == -2.0);
    CHECK(scrolled.deltaY == 3.0);
}

void testWindowAndUnknownEvents() {
    SDL_Event event{};
    event.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
    event.window.data1 = 1280;
    event.window.data2 = 720;
    auto translated = window::translateSdlEvent(event);
    CHECK(translated.has_value());
    const auto resized = std::get<window::WindowResizedEvent>(*translated);
    CHECK(resized.width == 1280);
    CHECK(resized.height == 720);

    event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    translated = window::translateSdlEvent(event);
    CHECK(translated.has_value());
    CHECK(std::holds_alternative<window::WindowCloseRequestedEvent>(*translated));

    event.type = SDL_EVENT_QUIT;
    translated = window::translateSdlEvent(event);
    CHECK(translated.has_value());
    CHECK(std::holds_alternative<window::WindowCloseRequestedEvent>(*translated));

    event.type = SDL_EVENT_USER;
    CHECK(!window::translateSdlEvent(event).has_value());
}

} // namespace

int main() {
    try {
        testScancodeSequences();
        testNamedScancodes();
        testKeyboardEvents();
        testMouseEvents();
        testWindowAndUnknownEvents();
        std::cout << "All SDL event translation tests passed successfully!\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
