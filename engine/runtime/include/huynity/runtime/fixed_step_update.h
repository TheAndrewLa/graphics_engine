#pragma once

#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace huynity::runtime {

struct UpdateResult {
    std::size_t steps{};
    std::size_t discardedSteps{};
    double discardedTime{};
    double remainder{};
};

class FixedStepUpdate {
public:
    FixedStepUpdate(double fixedStep, std::size_t maxSteps) : m_fixedStep{fixedStep}, m_maxSteps{maxSteps} {
        if (!std::isfinite(fixedStep) || fixedStep <= 0) {
            throw std::invalid_argument("Fixed step must be finite and positive");
        }
        if (maxSteps == 0) {
            throw std::invalid_argument("Max steps must be positive");
        }
    }

    template <typename Update>
    UpdateResult advance(double elapsed, Update&& update) {
        if (!std::isfinite(elapsed) || elapsed < 0) {
            throw std::invalid_argument("Elapsed time must be finite and non-negative");
        }
        const double accumulated = m_accumulator + elapsed;
        if (!std::isfinite(accumulated)) {
            throw std::overflow_error("Fixed-step accumulator overflow");
        }
        m_accumulator = accumulated;
        UpdateResult result;
        while (m_accumulator >= m_fixedStep && result.steps < m_maxSteps) {
            std::forward<Update>(update)(m_fixedStep);
            m_accumulator -= m_fixedStep;
            ++result.steps;
        }
        if (m_accumulator >= m_fixedStep) {
            const double discardedCount = std::floor(m_accumulator / m_fixedStep);
            if (discardedCount >= static_cast<double>(std::numeric_limits<std::size_t>::max())) {
                result.discardedSteps = std::numeric_limits<std::size_t>::max();
            } else {
                result.discardedSteps = static_cast<std::size_t>(discardedCount);
            }
            const double remainder = std::fmod(m_accumulator, m_fixedStep);
            result.discardedTime = m_accumulator - remainder;
            m_accumulator = remainder;
        }
        result.remainder = m_accumulator;
        return result;
    }

    [[nodiscard]] double fixedStep() const noexcept { return m_fixedStep; }
    [[nodiscard]] double remainder() const noexcept { return m_accumulator; }

private:
    double m_fixedStep;
    std::size_t m_maxSteps;
    double m_accumulator{};
};

} // namespace huynity::runtime
