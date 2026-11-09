#pragma once

#include "huynity/runtime/main_loop.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace runtime = huynity::runtime;

// Минимальный инструментированный исполнитель для тестов главного цикла; принимает не
// более одного кадра, публикует завершение в зарезервированный слот канала и записывает
// принятые frameId для проверки глубины конвейера и корреляции.
class FakeRendererPort final : public runtime::RendererPort {
public:
    explicit FakeRendererPort(runtime::CompletionChannel& completions) : m_completions{&completions} {}

    [[nodiscard]] runtime::SubmitResult trySubmit(runtime::RenderFrame& frame) override {
        if (m_active || !m_completions->reserve()) {
            return runtime::SubmitResult::Rejected;
        }
        m_acceptedIds.push_back(frame.frameId);
        m_active.emplace(std::move(frame));
        return runtime::SubmitResult::Accepted;
    }

    [[nodiscard]] bool completeReady() {
        if (!m_active) {
            return false;
        }
        const auto id = m_active->frameId;
        auto frame = std::move(*m_active);
        if (!m_completions->publish(runtime::FrameReady{id, std::move(frame)})) {
            return false;
        }
        m_active.reset();
        return true;
    }

    [[nodiscard]] bool completeFailed(std::string error) {
        if (!m_active) {
            return false;
        }
        const auto id = m_active->frameId;
        auto frame = std::move(*m_active);
        if (!m_completions->publish(runtime::FrameFailed{id, std::move(frame), std::move(error)})) {
            return false;
        }
        m_active.reset();
        return true;
    }

    [[nodiscard]] bool isBusy() const noexcept { return m_active.has_value(); }
    [[nodiscard]] std::uint64_t activeFrameId() const { return m_active->frameId; }
    [[nodiscard]] const std::vector<std::uint64_t>& acceptedIds() const noexcept { return m_acceptedIds; }

private:
    std::optional<runtime::RenderFrame> m_active;
    runtime::CompletionChannel* m_completions;
    std::vector<std::uint64_t> m_acceptedIds;
};
