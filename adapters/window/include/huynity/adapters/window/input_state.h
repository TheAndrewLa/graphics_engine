#pragma once

#include "huynity/adapters/window/input_events.h"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <variant>

namespace huynity::adapters::window {

/**
 * @brief Синхронный снимок состояния клавиатуры и мыши для текущего шага кадра.
 */
struct InputState {
public:
    static constexpr std::size_t MAX_KEYS = 512;
    static constexpr std::size_t MAX_MOUSE_BUTTONS = 8;

    /**
     * @brief Проверяет, зажата ли клавиша на клавиатуре.
     */
    [[nodiscard]] bool isKeyPressed(KeyCode key) const noexcept {
        if (key == KeyCode::Unknown) {
            return false;
        }
        const auto index = static_cast<std::size_t>(key);
        return index < m_keys.size() && m_keys[index];
    }

    /**
     * @brief Проверяет, зажата ли кнопка мыши.
     */
    [[nodiscard]] bool isMouseButtonPressed(MouseButton button) const noexcept {
        const auto index = static_cast<std::size_t>(button);
        return index < m_mouseButtons.size() && m_mouseButtons[index];
    }

    /**
     * @brief Текущая X-координата курсора мыши в пикселях окна.
     */
    [[nodiscard]] double mouseX() const noexcept { return m_mouseX; }

    /**
     * @brief Текущая Y-координата курсора мыши в пикселях окна.
     */
    [[nodiscard]] double mouseY() const noexcept { return m_mouseY; }

    /**
     * @brief Накопленное за кадр горизонтальное смещение мыши.
     */
    [[nodiscard]] double mouseDeltaX() const noexcept { return m_mouseDeltaX; }

    /**
     * @brief Накопленное за кадр вертикальное смещение мыши.
     */
    [[nodiscard]] double mouseDeltaY() const noexcept { return m_mouseDeltaY; }

    /**
     * @brief Накопленная за кадр дельта прокрутки колеса мыши (вертикальная).
     */
    [[nodiscard]] double scrollDelta() const noexcept { return m_scrollDeltaY; }

    /**
     * @brief Накопленная за кадр горизонтальная дельта прокрутки колеса мыши.
     */
    [[nodiscard]] double scrollDeltaX() const noexcept { return m_scrollDeltaX; }

    /**
     * @brief Накопленная за кадр вертикальная дельта прокрутки колеса мыши.
     */
    [[nodiscard]] double scrollDeltaY() const noexcept { return m_scrollDeltaY; }

    /**
     * @brief Сбрасывает накопленные за кадр дельты мыши и колеса прокрутки.
     */
    void resetDeltas() noexcept {
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
        m_scrollDeltaX = 0.0;
        m_scrollDeltaY = 0.0;
    }

    /**
     * @brief Полный сброс состояния ввода к начальным значениям.
     */
    void clear() noexcept {
        m_keys.fill(false);
        m_mouseButtons.fill(false);
        m_mouseX = 0.0;
        m_mouseY = 0.0;
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
        m_scrollDeltaX = 0.0;
        m_scrollDeltaY = 0.0;
    }

    /**
     * @brief Обрабатывает одно событие ввода и обновляет агрегированное состояние.
     */
    void processEvent(const InputEvent& event) noexcept {
        std::visit(
            [this](const auto& e) noexcept {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, KeyPressedEvent>) {
                    if (e.key != KeyCode::Unknown) {
                        const auto index = static_cast<std::size_t>(e.key);
                        if (index < m_keys.size()) {
                            m_keys[index] = true;
                        }
                    }
                } else if constexpr (std::is_same_v<T, KeyReleasedEvent>) {
                    if (e.key != KeyCode::Unknown) {
                        const auto index = static_cast<std::size_t>(e.key);
                        if (index < m_keys.size()) {
                            m_keys[index] = false;
                        }
                    }
                } else if constexpr (std::is_same_v<T, MouseMovedEvent>) {
                    m_mouseX = e.x;
                    m_mouseY = e.y;
                    m_mouseDeltaX += e.deltaX;
                    m_mouseDeltaY += e.deltaY;
                } else if constexpr (std::is_same_v<T, MouseButtonPressedEvent>) {
                    m_mouseX = e.x;
                    m_mouseY = e.y;
                    const auto index = static_cast<std::size_t>(e.button);
                    if (index < m_mouseButtons.size()) {
                        m_mouseButtons[index] = true;
                    }
                } else if constexpr (std::is_same_v<T, MouseButtonReleasedEvent>) {
                    m_mouseX = e.x;
                    m_mouseY = e.y;
                    const auto index = static_cast<std::size_t>(e.button);
                    if (index < m_mouseButtons.size()) {
                        m_mouseButtons[index] = false;
                    }
                } else if constexpr (std::is_same_v<T, MouseScrolledEvent>) {
                    m_scrollDeltaX += e.deltaX;
                    m_scrollDeltaY += e.deltaY;
                }
            },
            event);
    }

    /**
     * @brief Обновляет состояние ввода на основе последовательности событий.
     */
    void update(std::span<const InputEvent> events) noexcept {
        for (const auto& event : events) {
            processEvent(event);
        }
    }

    /**
     * @brief Перегрузка для удобного обновления через initializer_list.
     */
    void update(std::initializer_list<InputEvent> events) noexcept {
        for (const auto& event : events) {
            processEvent(event);
        }
    }

    bool operator==(const InputState&) const = default;

private:
    std::array<bool, MAX_KEYS> m_keys{};
    std::array<bool, MAX_MOUSE_BUTTONS> m_mouseButtons{};
    double m_mouseX{0.0};
    double m_mouseY{0.0};
    double m_mouseDeltaX{0.0};
    double m_mouseDeltaY{0.0};
    double m_scrollDeltaX{0.0};
    double m_scrollDeltaY{0.0};
};

} // namespace huynity::adapters::window
