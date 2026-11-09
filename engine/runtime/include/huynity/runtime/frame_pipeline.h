#pragma once

#include "huynity/runtime/frame_completions.h"
#include "huynity/runtime/framebuffer_pool.h"
#include "huynity/runtime/presentation_port.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace huynity::runtime {

struct SurfaceSize {
    std::size_t width{};
    std::size_t height{};

    [[nodiscard]] bool hasArea() const noexcept { return width != 0 && height != 0; }
};

/// @brief Состояние поверхности показа с монотонным поколением.
///
/// Поколение увеличивается только при фактическом изменении размера; кадр связывается
/// с поколением поверхности в момент отправки, что позволяет отбросить завершившийся
/// кадр прежнего поколения без показа после resize.
class SurfaceState {
public:
    void resize(SurfaceSize size) noexcept {
        if (size.width != m_size.width || size.height != m_size.height) {
            m_size = size;
            ++m_generation;
        }
    }

    [[nodiscard]] SurfaceSize size() const noexcept { return m_size; }
    [[nodiscard]] std::uint64_t generation() const noexcept { return m_generation; }
    [[nodiscard]] bool hasArea() const noexcept { return m_size.hasArea(); }

private:
    SurfaceSize m_size;
    std::uint64_t m_generation{};
};

enum class FrameOutcome {
    None,    // завершений нет
    Ready,   // кадр актуального поколения готов к показу
    Failed,  // кадр актуального поколения завершился ошибкой
    Stale,   // кадр прежнего поколения поверхности отброшен без показа
    Ignored, // чужое или повторное завершение
};

struct PipelineCompletion {
    FrameOutcome outcome{FrameOutcome::None};
    std::optional<FrameCompletion> completion; // владение кадром при Ready/Failed/Stale
};

enum class DispatchResult {
    Empty,        // prepared-слот пуст или Renderer занят активным кадром
    Submitted,    // prepared-снимок отправлен Renderer
    DroppedStale, // снимок прежнего поколения поверхности отброшен без отправки
    Rejected,     // транспорт отклонил задание, снимок сохранён в слоте
};

/// @brief Конвейер кадров главного цикла: поколение поверхности и prepared-слот поверх FrameTracker.
///
/// Конвейер ограничен глубиной 1+1: один исполняемый Renderer кадр и не более одного
/// подготовленного снимка. Подготовка разрешена во время активного рендеринга, пока
/// prepared-слот пуст; пока слот занят, изменения накапливаются в Core, а новые снимки
/// не создаются. Отправка допускается только при ненулевой площади поверхности.
/// Кадры и prepared-снимки прежнего поколения поверхности отбрасываются без показа.
class FramePipeline {
public:
    /// @brief Фаза events/resize главного цикла; выполняется до обработки завершений.
    ///
    /// Prepared-снимок прежнего поколения отбрасывается: он построен для прежнего
    /// размера поверхности и не может быть показан.
    void resize(SurfaceSize size) noexcept {
        m_surface.resize(size);
        m_pool.resize(size.width, size.height);
        if (m_prepared && m_prepared->surfaceGeneration != m_surface.generation()) {
            m_prepared.reset();
        }
    }

    /// @brief Отправляет подготовленный кадр; при нулевой площади задание отклоняется.
    [[nodiscard]] SubmitResult submit(RendererPort& port, RenderFrame& frame) {
        if (!m_surface.hasArea()) {
            return SubmitResult::Rejected;
        }
        const auto result = m_tracker.submit(port, frame);
        if (result == SubmitResult::Accepted) {
            m_activeGeneration = m_surface.generation();
        }
        return result;
    }

    /// @brief Фаза completions: обрабатывает завершение активного кадра.
    [[nodiscard]] PipelineCompletion complete(CompletionChannel& channel) {
        const bool hadCompletion = channel.hasCompletion();
        auto tracked = m_tracker.completeTracked(channel);
        if (tracked.result == CompletionResult::Ignored) {
            return {hadCompletion ? FrameOutcome::Ignored : FrameOutcome::None, std::nullopt};
        }
        const bool isStale = m_activeGeneration != m_surface.generation();
        m_activeGeneration.reset();
        if (isStale) {
            return {FrameOutcome::Stale, std::move(tracked.completion)};
        }
        const auto outcome = tracked.result == CompletionResult::Ready ? FrameOutcome::Ready : FrameOutcome::Failed;
        return {outcome, std::move(tracked.completion)};
    }

    [[nodiscard]] bool rendererDown(CompletionChannel& channel) {
        const bool isDown = m_tracker.rendererDown(channel);
        if (isDown) {
            m_activeGeneration.reset();
        }
        return isDown;
    }

    void requestStop() noexcept {
        m_tracker.requestStop();
        m_prepared.reset(); // после запроса остановки новые кадры не отправляются
    }

    void confirmStopped() noexcept {
        m_tracker.confirmStopped();
        if (m_tracker.isStopped()) {
            m_activeGeneration.reset();
        }
    }

    /// @brief Допускает подготовку следующего снимка: prepared-слот пуст, поверхность
    /// имеет ненулевую площадь, исполнитель жив и не остановлен.
    ///
    /// Активный кадр Renderer не препятствует подготовке: prepare перекрывается с render.
    [[nodiscard]] bool canPrepare() const noexcept {
        return m_surface.hasArea() && !m_prepared.has_value() && !m_tracker.isRendererDown() &&
               !m_tracker.isStopRequested() && !m_tracker.isStopped();
    }

    /// @brief Помещает подготовленный снимок в единственный prepared-слот.
    /// @return false, если слот занят или подготовка недопустима; владение остаётся у вызывающего.
    [[nodiscard]] bool storePrepared(RenderFrame& frame) {
        if (!canPrepare()) {
            return false;
        }
        m_prepared = PreparedFrame{std::move(frame), m_surface.generation()};
        return true;
    }

    /// @brief Отправляет prepared-снимок, если Renderer свободен и поколение актуально.
    ///
    /// При занятом Renderer снимок остаётся в слоте; снимок прежнего поколения
    /// отбрасывается; при отказе транспорта снимок сохраняется для следующего прохода.
    [[nodiscard]] DispatchResult dispatchPrepared(RendererPort& port) {
        if (!m_prepared || m_tracker.hasActiveFrame()) {
            return DispatchResult::Empty;
        }
        if (m_prepared->surfaceGeneration != m_surface.generation()) {
            m_prepared.reset();
            return DispatchResult::DroppedStale;
        }
        auto frame = std::move(m_prepared->frame);
        const auto result = submit(port, frame);
        if (result == SubmitResult::Rejected) {
            m_prepared->frame = std::move(frame);
            return DispatchResult::Rejected;
        }
        m_prepared.reset();
        return DispatchResult::Submitted;
    }

    /// @brief Выдаёт framebuffer текущего размера поверхности из пула или создаёт новый.
    [[nodiscard]] std::unique_ptr<image::ImageBuffer> acquireFramebuffer() { return m_pool.acquire(); }

    /// @brief Маршрутизирует завершённый кадр: Ready — на показ, Stale/Failed — сразу в пул.
    ///
    /// Уникальное владение framebuffer возвращается Runtime вместе с завершением. При
    /// синхронном показе буфер возвращается в пул сразу после present; буфер, удержанный
    /// асинхронным Presentation, возвращается отдельным вызовом releaseFromPresentation.
    /// Кадр прежнего поколения не показывается, его буфер прежнего размера уничтожается
    /// пулом без обращения к недействительной поверхности.
    void handleCompletedFrame(PipelineCompletion completed, PresentationPort& presentation) {
        if (!completed.completion) {
            return;
        }
        std::visit(
            [&](auto& value) {
                auto framebuffer = std::move(value.frame.framebuffer);
                if (framebuffer == nullptr) {
                    return;
                }
                if (completed.outcome == FrameOutcome::Ready) {
                    framebuffer = presentation.present(std::move(framebuffer));
                }
                m_pool.release(std::move(framebuffer));
            },
            *completed.completion);
    }

    /// @brief Возвращает в пул буфер, освобождённый асинхронным Presentation.
    void releaseFromPresentation(std::unique_ptr<image::ImageBuffer> framebuffer) {
        m_pool.release(std::move(framebuffer));
    }

    [[nodiscard]] const SurfaceState& surface() const noexcept { return m_surface; }
    [[nodiscard]] const FrameTracker& tracker() const noexcept { return m_tracker; }
    [[nodiscard]] const FramebufferPool& pool() const noexcept { return m_pool; }
    [[nodiscard]] bool hasActiveFrame() const noexcept { return m_tracker.hasActiveFrame(); }
    [[nodiscard]] bool hasPreparedFrame() const noexcept { return m_prepared.has_value(); }

private:
    struct PreparedFrame {
        RenderFrame frame;
        std::uint64_t surfaceGeneration{};
    };

    SurfaceState m_surface;
    FrameTracker m_tracker;
    FramebufferPool m_pool;
    std::optional<std::uint64_t> m_activeGeneration;
    std::optional<PreparedFrame> m_prepared;
};

} // namespace huynity::runtime
