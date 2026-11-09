#pragma once

#include <cstdint>
#include <variant>

namespace huynity::adapters::window {

/**
 * @brief Платформонезависимые коды клавиш клавиатуры.
 */
enum class KeyCode : std::uint16_t {
    Unknown = 0,

    // Буквы
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // Цифры верхнего ряда
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,

    // Функциональные клавиши
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // Управление и навигация
    Space,
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Left,
    Right,
    Up,
    Down,
    PageUp,
    PageDown,
    Home,
    End,

    // Переключатели
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,

    // Модификаторы
    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,

    Count
};

/**
 * @brief Кнопки мыши.
 */
enum class MouseButton : std::uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Count
};

/**
 * @brief Событие изменения размера окна.
 */
struct WindowResizedEvent {
    std::uint32_t width{0};
    std::uint32_t height{0};

    bool operator==(const WindowResizedEvent&) const = default;
};

/**
 * @brief Событие запроса на закрытие окна.
 */
struct WindowCloseRequestedEvent {
    bool operator==(const WindowCloseRequestedEvent&) const = default;
};

/**
 * @brief Событие нажатия клавиши.
 */
struct KeyPressedEvent {
    KeyCode key{KeyCode::Unknown};
    bool isRepeat{false};

    bool operator==(const KeyPressedEvent&) const = default;
};

/**
 * @brief Событие отпускания клавиши.
 */
struct KeyReleasedEvent {
    KeyCode key{KeyCode::Unknown};

    bool operator==(const KeyReleasedEvent&) const = default;
};

/**
 * @brief Событие перемещения курсора мыши.
 */
struct MouseMovedEvent {
    double x{0.0};
    double y{0.0};
    double deltaX{0.0};
    double deltaY{0.0};

    bool operator==(const MouseMovedEvent&) const = default;
};

/**
 * @brief Событие нажатия кнопки мыши.
 */
struct MouseButtonPressedEvent {
    MouseButton button{MouseButton::Left};
    double x{0.0};
    double y{0.0};

    bool operator==(const MouseButtonPressedEvent&) const = default;
};

/**
 * @brief Событие отпускания кнопки мыши.
 */
struct MouseButtonReleasedEvent {
    MouseButton button{MouseButton::Left};
    double x{0.0};
    double y{0.0};

    bool operator==(const MouseButtonReleasedEvent&) const = default;
};

/**
 * @brief Событие прокрутки колеса мыши.
 */
struct MouseScrolledEvent {
    double deltaX{0.0};
    double deltaY{0.0};

    bool operator==(const MouseScrolledEvent&) const = default;
};

/**
 * @brief Обобщённое типизированное событие ввода или окна.
 */
using InputEvent = std::variant<WindowResizedEvent, WindowCloseRequestedEvent, KeyPressedEvent, KeyReleasedEvent,
                                MouseMovedEvent, MouseButtonPressedEvent, MouseButtonReleasedEvent, MouseScrolledEvent>;

} // namespace huynity::adapters::window
