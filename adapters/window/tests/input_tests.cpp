#include "huynity/adapters/window/input_events.h"
#include "huynity/adapters/window/input_state.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace window = huynity::adapters::window;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

namespace {

bool isClose(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) <= epsilon;
}

void testInitialDefaultState() {
    const window::InputState state{};

    CHECK(!state.isKeyPressed(window::KeyCode::W));
    CHECK(!state.isKeyPressed(window::KeyCode::A));
    CHECK(!state.isKeyPressed(window::KeyCode::S));
    CHECK(!state.isKeyPressed(window::KeyCode::D));
    CHECK(!state.isKeyPressed(window::KeyCode::Space));
    CHECK(!state.isKeyPressed(window::KeyCode::Escape));
    CHECK(!state.isKeyPressed(window::KeyCode::Unknown));

    CHECK(!state.isMouseButtonPressed(window::MouseButton::Left));
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Right));
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Middle));
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Button4));
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Button5));

    CHECK(isClose(state.mouseX(), 0.0));
    CHECK(isClose(state.mouseY(), 0.0));
    CHECK(isClose(state.mouseDeltaX(), 0.0));
    CHECK(isClose(state.mouseDeltaY(), 0.0));
    CHECK(isClose(state.scrollDelta(), 0.0));
    CHECK(isClose(state.scrollDeltaX(), 0.0));
    CHECK(isClose(state.scrollDeltaY(), 0.0));
}

void testKeyPressAndRelease() {
    window::InputState state;

    state.processEvent(window::KeyPressedEvent{.key = window::KeyCode::W, .isRepeat = false});
    CHECK(state.isKeyPressed(window::KeyCode::W));
    CHECK(!state.isKeyPressed(window::KeyCode::S));

    state.processEvent(window::KeyPressedEvent{.key = window::KeyCode::Space, .isRepeat = false});
    CHECK(state.isKeyPressed(window::KeyCode::W));
    CHECK(state.isKeyPressed(window::KeyCode::Space));

    // Repeat press does not clear pressed state
    state.processEvent(window::KeyPressedEvent{.key = window::KeyCode::W, .isRepeat = true});
    CHECK(state.isKeyPressed(window::KeyCode::W));

    // Release W
    state.processEvent(window::KeyReleasedEvent{.key = window::KeyCode::W});
    CHECK(!state.isKeyPressed(window::KeyCode::W));
    CHECK(state.isKeyPressed(window::KeyCode::Space));

    // Release Space
    state.processEvent(window::KeyReleasedEvent{.key = window::KeyCode::Space});
    CHECK(!state.isKeyPressed(window::KeyCode::Space));
}

void testKeyUnknownIgnored() {
    window::InputState state;

    state.processEvent(window::KeyPressedEvent{.key = window::KeyCode::Unknown});
    CHECK(!state.isKeyPressed(window::KeyCode::Unknown));

    state.processEvent(window::KeyReleasedEvent{.key = window::KeyCode::Unknown});
    CHECK(!state.isKeyPressed(window::KeyCode::Unknown));
}

void testMouseButtonPressAndRelease() {
    window::InputState state;

    state.processEvent(window::MouseButtonPressedEvent{
        .button = window::MouseButton::Left,
        .x = 100.0,
        .y = 200.0,
    });
    CHECK(state.isMouseButtonPressed(window::MouseButton::Left));
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Right));
    CHECK(isClose(state.mouseX(), 100.0));
    CHECK(isClose(state.mouseY(), 200.0));

    state.processEvent(window::MouseButtonPressedEvent{
        .button = window::MouseButton::Right,
        .x = 150.0,
        .y = 250.0,
    });
    CHECK(state.isMouseButtonPressed(window::MouseButton::Left));
    CHECK(state.isMouseButtonPressed(window::MouseButton::Right));
    CHECK(isClose(state.mouseX(), 150.0));
    CHECK(isClose(state.mouseY(), 250.0));

    state.processEvent(window::MouseButtonReleasedEvent{
        .button = window::MouseButton::Left,
        .x = 160.0,
        .y = 260.0,
    });
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Left));
    CHECK(state.isMouseButtonPressed(window::MouseButton::Right));
    CHECK(isClose(state.mouseX(), 160.0));
    CHECK(isClose(state.mouseY(), 260.0));

    state.processEvent(window::MouseButtonReleasedEvent{
        .button = window::MouseButton::Right,
        .x = 160.0,
        .y = 260.0,
    });
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Right));
}

void testMouseMovedAccumulation() {
    window::InputState state;

    state.processEvent(window::MouseMovedEvent{
        .x = 10.0,
        .y = 20.0,
        .deltaX = 10.0,
        .deltaY = 20.0,
    });
    CHECK(isClose(state.mouseX(), 10.0));
    CHECK(isClose(state.mouseY(), 20.0));
    CHECK(isClose(state.mouseDeltaX(), 10.0));
    CHECK(isClose(state.mouseDeltaY(), 20.0));

    // Next motion event accumulates delta
    state.processEvent(window::MouseMovedEvent{
        .x = 15.0,
        .y = 18.0,
        .deltaX = 5.0,
        .deltaY = -2.0,
    });
    CHECK(isClose(state.mouseX(), 15.0));
    CHECK(isClose(state.mouseY(), 18.0));
    CHECK(isClose(state.mouseDeltaX(), 15.0));
    CHECK(isClose(state.mouseDeltaY(), 18.0));
}

void testMouseScrolledAccumulation() {
    window::InputState state;

    state.processEvent(window::MouseScrolledEvent{
        .deltaX = 1.0,
        .deltaY = 2.5,
    });
    CHECK(isClose(state.scrollDelta(), 2.5));
    CHECK(isClose(state.scrollDeltaX(), 1.0));
    CHECK(isClose(state.scrollDeltaY(), 2.5));

    state.processEvent(window::MouseScrolledEvent{
        .deltaX = -0.5,
        .deltaY = -1.0,
    });
    CHECK(isClose(state.scrollDelta(), 1.5));
    CHECK(isClose(state.scrollDeltaX(), 0.5));
    CHECK(isClose(state.scrollDeltaY(), 1.5));
}

void testResetDeltasPreservesPositionsAndKeys() {
    window::InputState state;

    state.processEvent(window::KeyPressedEvent{.key = window::KeyCode::LeftShift});
    state.processEvent(window::MouseButtonPressedEvent{
        .button = window::MouseButton::Middle,
        .x = 42.0,
        .y = 84.0,
    });
    state.processEvent(window::MouseMovedEvent{
        .x = 50.0,
        .y = 90.0,
        .deltaX = 8.0,
        .deltaY = 6.0,
    });
    state.processEvent(window::MouseScrolledEvent{
        .deltaX = 0.0,
        .deltaY = -3.0,
    });

    CHECK(state.isKeyPressed(window::KeyCode::LeftShift));
    CHECK(state.isMouseButtonPressed(window::MouseButton::Middle));
    CHECK(isClose(state.mouseX(), 50.0));
    CHECK(isClose(state.mouseY(), 90.0));
    CHECK(isClose(state.mouseDeltaX(), 8.0));
    CHECK(isClose(state.mouseDeltaY(), 6.0));
    CHECK(isClose(state.scrollDelta(), -3.0));

    state.resetDeltas();

    // Deltas are reset
    CHECK(isClose(state.mouseDeltaX(), 0.0));
    CHECK(isClose(state.mouseDeltaY(), 0.0));
    CHECK(isClose(state.scrollDelta(), 0.0));
    CHECK(isClose(state.scrollDeltaX(), 0.0));
    CHECK(isClose(state.scrollDeltaY(), 0.0));

    // Persistent state is preserved
    CHECK(state.isKeyPressed(window::KeyCode::LeftShift));
    CHECK(state.isMouseButtonPressed(window::MouseButton::Middle));
    CHECK(isClose(state.mouseX(), 50.0));
    CHECK(isClose(state.mouseY(), 90.0));
}

void testClearResetsEverything() {
    window::InputState state;

    state.processEvent(window::KeyPressedEvent{.key = window::KeyCode::A});
    state.processEvent(window::MouseButtonPressedEvent{
        .button = window::MouseButton::Button4,
        .x = 10.0,
        .y = 10.0,
    });
    state.processEvent(window::MouseMovedEvent{.x = 20.0, .y = 30.0, .deltaX = 5.0, .deltaY = 5.0});
    state.processEvent(window::MouseScrolledEvent{.deltaX = 1.0, .deltaY = 2.0});

    state.clear();
    CHECK(!state.isKeyPressed(window::KeyCode::A));
    CHECK(!state.isMouseButtonPressed(window::MouseButton::Button4));
    CHECK(isClose(state.mouseX(), 0.0));
    CHECK(isClose(state.mouseY(), 0.0));
    CHECK(isClose(state.mouseDeltaX(), 0.0));
    CHECK(isClose(state.mouseDeltaY(), 0.0));
    CHECK(isClose(state.scrollDelta(), 0.0));
}

void testBatchUpdateWithMixedEvents() {
    window::InputState state;

    std::vector<window::InputEvent> events{
        window::WindowResizedEvent{.width = 1920, .height = 1080},
        window::KeyPressedEvent{.key = window::KeyCode::W},
        window::KeyPressedEvent{.key = window::KeyCode::D},
        window::MouseMovedEvent{.x = 640.0, .y = 480.0, .deltaX = 12.0, .deltaY = -4.0},
        window::MouseButtonPressedEvent{.button = window::MouseButton::Left, .x = 640.0, .y = 480.0},
        window::MouseScrolledEvent{.deltaX = 0.0, .deltaY = 1.0},
        window::WindowCloseRequestedEvent{},
    };

    state.update(events);

    CHECK(state.isKeyPressed(window::KeyCode::W));
    CHECK(state.isKeyPressed(window::KeyCode::D));
    CHECK(!state.isKeyPressed(window::KeyCode::S));
    CHECK(state.isMouseButtonPressed(window::MouseButton::Left));
    CHECK(isClose(state.mouseX(), 640.0));
    CHECK(isClose(state.mouseY(), 480.0));
    CHECK(isClose(state.mouseDeltaX(), 12.0));
    CHECK(isClose(state.mouseDeltaY(), -4.0));
    CHECK(isClose(state.scrollDelta(), 1.0));
}

void testEventVariantTypesAndEquality() {
    window::InputEvent e1 = window::WindowResizedEvent{.width = 800, .height = 600};
    CHECK(std::holds_alternative<window::WindowResizedEvent>(e1));
    CHECK(std::get<window::WindowResizedEvent>(e1).width == 800);
    CHECK(std::get<window::WindowResizedEvent>(e1).height == 600);

    window::InputEvent e2 = window::WindowResizedEvent{.width = 800, .height = 600};
    CHECK(e1 == e2);

    window::InputEvent e3 = window::WindowCloseRequestedEvent{};
    CHECK(std::holds_alternative<window::WindowCloseRequestedEvent>(e3));
    CHECK(e1 != e3);

    window::InputEvent e4 = window::KeyPressedEvent{.key = window::KeyCode::Escape, .isRepeat = false};
    CHECK(std::holds_alternative<window::KeyPressedEvent>(e4));
    CHECK(std::get<window::KeyPressedEvent>(e4).key == window::KeyCode::Escape);

    window::InputEvent e5 = window::KeyReleasedEvent{.key = window::KeyCode::Escape};
    CHECK(std::holds_alternative<window::KeyReleasedEvent>(e5));

    window::InputEvent e6 = window::MouseMovedEvent{.x = 1.0, .y = 2.0, .deltaX = 3.0, .deltaY = 4.0};
    CHECK(std::holds_alternative<window::MouseMovedEvent>(e6));

    window::InputEvent e7 = window::MouseButtonPressedEvent{.button = window::MouseButton::Right, .x = 5.0, .y = 6.0};
    CHECK(std::holds_alternative<window::MouseButtonPressedEvent>(e7));

    window::InputEvent e8 = window::MouseButtonReleasedEvent{.button = window::MouseButton::Right, .x = 5.0, .y = 6.0};
    CHECK(std::holds_alternative<window::MouseButtonReleasedEvent>(e8));

    window::InputEvent e9 = window::MouseScrolledEvent{.deltaX = 0.5, .deltaY = -1.5};
    CHECK(std::holds_alternative<window::MouseScrolledEvent>(e9));
}

} // namespace

int main() {
    try {
        testInitialDefaultState();
        testKeyPressAndRelease();
        testKeyUnknownIgnored();
        testMouseButtonPressAndRelease();
        testMouseMovedAccumulation();
        testMouseScrolledAccumulation();
        testResetDeltasPreservesPositionsAndKeys();
        testClearResetsEverything();
        testBatchUpdateWithMixedEvents();
        testEventVariantTypesAndEquality();
        std::cout << "window_input_tests: OK\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
