#pragma once

#include "huynity/adapters/window/window_adapter.h"

#include <memory>
#include <string>

namespace huynity::adapters::window {

/**
 * @brief Параметры конфигурации создаваемого окна.
 */
struct WindowConfig {
    WindowSize size{800, 600};
    std::string title{"Huynity Window"};
    bool forceHeadless{false};

    bool operator==(const WindowConfig&) const = default;
};

/**
 * @brief Проверяет наличие поддержки нативного окна в текущей сборке.
 * @return true, если нативный бэкенд скомпилирован; иначе false.
 */
[[nodiscard]] bool hasNativeWindowSupport() noexcept;

/**
 * @brief Фабричная функция создания оконного адаптера.
 *
 * Если config.forceHeadless равен true или нативный бэкенд недоступен,
 * функция создаёт экземпляр HeadlessWindow с переданными размерами и заголовком.
 *
 * @param config конфигурация создаваемого окна.
 * @return уникальный указатель на интерфейс WindowAdapter.
 */
[[nodiscard]] std::unique_ptr<WindowAdapter> createWindow(const WindowConfig& config = {});

} // namespace huynity::adapters::window
