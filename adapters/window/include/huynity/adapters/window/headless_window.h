#pragma once

#include "huynity/adapters/window/input_events.h"
#include "huynity/adapters/window/window_adapter.h"
#include "huynity/image/image_buffer.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace huynity::adapters::window {

/**
 * @brief Детерминированная реализация WindowAdapter в оперативной памяти.
 *
 * Хранит состояние окна, очередь событий и копию последнего представленного кадра.
 * Поддерживает программную эмуляцию событий ввода и детерминированный синхронный present().
 * Используется для автоматизированного тестирования без графического окружения.
 */
class HeadlessWindow final : public WindowAdapter {
public:
    /**
     * @brief Создаёт окно с заданными размерами и заголовком.
     * @param size размеры клиентской области окна (по умолчанию 800x600).
     * @param title заголовок окна (по умолчанию "Huynity Headless").
     */
    explicit HeadlessWindow(WindowSize size = {800, 600}, std::string_view title = "Huynity Headless");

    ~HeadlessWindow() override = default;

    // WindowAdapter
    [[nodiscard]] bool isOpen() const noexcept override;
    void close() noexcept override;
    [[nodiscard]] WindowSize size() const noexcept override;
    void setTitle(std::string_view title) override;
    void pollEvents(std::vector<InputEvent>& outEvents) override;

    // runtime::PresentationPort
    [[nodiscard]] std::unique_ptr<image::ImageBuffer> present(std::unique_ptr<image::ImageBuffer> framebuffer) override;

    /**
     * @brief Изменяет размер клиентской области окна и помещает WindowResizedEvent в очередь событий.
     * @param newSize новые размеры окна.
     */
    void resize(WindowSize newSize);

    /**
     * @brief Изменяет размер клиентской области окна и помещает WindowResizedEvent в очередь событий.
     * @param width новая ширина окна в пикселях.
     * @param height новая высота окна в пикселях.
     */
    void resize(std::uint32_t width, std::uint32_t height);

    /**
     * @brief Добавляет событие в очередь событий окна.
     * @param event добавляемое событие ввода или окна.
     */
    void pushEvent(InputEvent event);

    /**
     * @brief Добавляет последовательность событий в очередь событий окна.
     * @param events диапазон добавляемых событий.
     */
    void pushEvents(std::span<const InputEvent> events);

    /**
     * @brief Возвращает текущий заголовок окна.
     * @return константная ссылка на строку заголовка.
     */
    [[nodiscard]] const std::string& title() const noexcept;

    /**
     * @brief Возвращает количество успешно выполненных вызовов present().
     * @return число успешно отображённых кадров.
     */
    [[nodiscard]] std::size_t presentedCount() const noexcept;

    /**
     * @brief Возвращает количество отклонённых вызовов present().
     * @return число отклонённых кадров.
     */
    [[nodiscard]] std::size_t rejectedPresents() const noexcept;

    /**
     * @brief Возвращает указатель на последний успешно отображённый кадровый буфер.
     * @return указатель на сохранённую копию буфера или nullptr, если кадры ещё не отображались.
     */
    [[nodiscard]] const image::ImageBuffer* lastPresentedFrame() const noexcept;

    /**
     * @brief Псевдоним для lastPresentedFrame().
     * @return указатель на сохранённую копию буфера или nullptr, если кадры ещё не отображались.
     */
    [[nodiscard]] const image::ImageBuffer* lastPresented() const noexcept;

    /**
     * @brief Возвращает количество событий, находящихся в очереди ожидания.
     * @return размер текущей очереди событий.
     */
    [[nodiscard]] std::size_t queuedEventCount() const noexcept;

private:
    WindowSize m_size{800, 600};
    std::string m_title;
    std::vector<InputEvent> m_eventQueue;
    std::unique_ptr<image::ImageBuffer> m_lastPresentedFrame;
    std::size_t m_presentedCount{0};
    std::size_t m_rejectedPresents{0};
    bool m_isOpen{true};
};

} // namespace huynity::adapters::window
