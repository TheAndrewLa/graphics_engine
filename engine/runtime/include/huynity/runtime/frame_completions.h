#pragma once

#include "huynity/runtime/renderer_port.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace huynity::runtime {

struct FrameReady {
    std::uint64_t frameId;
    RenderFrame frame;
};

struct FrameFailed {
    std::uint64_t frameId;
    RenderFrame frame;
    std::string error;
};

using FrameCompletion = std::variant<FrameReady, FrameFailed>;

class CompletionChannel {
public:
    [[nodiscard]] bool reserve() noexcept {
        if (m_reserved) {
            return false;
        }
        m_reserved = true;
        return true;
    }

    [[nodiscard]] bool publish(FrameCompletion completion) {
        if (!m_reserved || m_completion) {
            return false;
        }
        m_completion.emplace(std::move(completion));
        return true;
    }

    [[nodiscard]] std::optional<FrameCompletion> take() {
        if (!m_completion) {
            return std::nullopt;
        }
        auto result = std::move(m_completion);
        m_completion.reset();
        m_reserved = false;
        return result;
    }

    void markRendererDown() noexcept { m_rendererDown = true; }

    [[nodiscard]] bool takeRendererDown() noexcept {
        const bool result = m_rendererDown;
        m_rendererDown = false;
        return result;
    }

    [[nodiscard]] bool hasReservedSlot() const noexcept { return m_reserved; }

    [[nodiscard]] bool hasCompletion() const noexcept { return m_completion.has_value(); }

private:
    std::optional<FrameCompletion> m_completion;
    bool m_reserved{};
    bool m_rendererDown{};
};

enum class CompletionResult {
    Ready,
    Failed,
    Ignored,
};

struct TrackedCompletion {
    CompletionResult result{CompletionResult::Ignored};
    std::optional<FrameCompletion> completion; // заполнено при совпавшем frameId, иначе завершение уничтожено
};

class FrameTracker {
public:
    using Clock = std::chrono::steady_clock;

    [[nodiscard]] SubmitResult submit(RendererPort& port, RenderFrame& frame) {
        return submit(port, frame, Clock::now());
    }

    /// @brief Отправляет задание и запоминает момент отправки для диагностики превышения времени кадра.
    [[nodiscard]] SubmitResult submit(RendererPort& port, RenderFrame& frame, Clock::time_point submittedAt) {
        if (m_rendererDown || m_stopRequested || m_isStopped) {
            return SubmitResult::Rejected;
        }
        const auto result = port.trySubmit(frame);
        if (result == SubmitResult::Accepted) {
            m_activeFrame = frame.frameId;
            m_submittedAt = submittedAt;
        }
        return result;
    }

    [[nodiscard]] CompletionResult complete(CompletionChannel& channel) { return completeTracked(channel).result; }

    /// @brief Обрабатывает завершение и возвращает владение кадром при совпадении frameId.
    ///
    /// Чужое или повторное завершение уничтожается без передачи владения, как и в complete().
    [[nodiscard]] TrackedCompletion completeTracked(CompletionChannel& channel) {
        auto completion = channel.take();
        if (!completion) {
            return {};
        }
        const auto id = std::visit([](const auto& value) { return value.frameId; }, *completion);
        if (!m_activeFrame || id != *m_activeFrame) {
            ++m_ignoredCompletions;
            if (m_activeFrame) {
                static_cast<void>(channel.reserve());
            }
            return {};
        }
        m_activeFrame.reset();
        m_submittedAt.reset();
        const auto result =
            std::holds_alternative<FrameReady>(*completion) ? CompletionResult::Ready : CompletionResult::Failed;
        return {result, std::move(*completion)};
    }

    [[nodiscard]] bool rendererDown(CompletionChannel& channel) {
        if (!channel.takeRendererDown()) {
            return false;
        }
        m_rendererDown = true;
        m_activeFrame.reset();
        m_submittedAt.reset();
        return true;
    }

    void requestStop() noexcept { m_stopRequested = true; }

    void confirmStopped() noexcept {
        if (m_stopRequested || m_rendererDown) {
            m_isStopped = true;
            m_activeFrame.reset();
            m_submittedAt.reset();
        }
    }

    /// @brief Диагностирует превышение ожидаемого времени активного кадра.
    ///
    /// Таймаут — только диагностика: он не освобождает ресурсы задания и не снимает его с учёта.
    /// Активный слот освобождает только совпавший FrameReady/FrameFailed или подтверждённый
    /// RendererDown, поэтому позднее штатное завершение после таймаута остаётся допустимым.
    [[nodiscard]] bool isFrameOverdue(Clock::duration limit, Clock::time_point now) const noexcept {
        return m_activeFrame.has_value() && m_submittedAt.has_value() && now - *m_submittedAt > limit;
    }

    [[nodiscard]] bool hasActiveFrame() const noexcept { return m_activeFrame.has_value(); }
    [[nodiscard]] std::optional<std::uint64_t> activeFrameId() const noexcept { return m_activeFrame; }
    [[nodiscard]] std::size_t ignoredCompletions() const noexcept { return m_ignoredCompletions; }
    [[nodiscard]] bool isRendererDown() const noexcept { return m_rendererDown; }
    [[nodiscard]] bool isStopRequested() const noexcept { return m_stopRequested; }
    [[nodiscard]] bool isStopped() const noexcept { return m_isStopped; }

private:
    std::optional<std::uint64_t> m_activeFrame;
    std::optional<Clock::time_point> m_submittedAt;
    std::size_t m_ignoredCompletions{};
    bool m_rendererDown{};
    bool m_stopRequested{};
    bool m_isStopped{};
};

} // namespace huynity::runtime
