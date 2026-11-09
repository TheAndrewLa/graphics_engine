#include "huynity/runtime/fixed_step_update.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace runtime = huynity::runtime;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

#define CHECK_THROWS_AS(expression, ExceptionType)                                                                     \
    do {                                                                                                               \
        bool wasCaught = false;                                                                                        \
        try {                                                                                                          \
            expression;                                                                                                \
        } catch (const ExceptionType&) {                                                                               \
            wasCaught = true;                                                                                          \
        }                                                                                                              \
        CHECK(wasCaught);                                                                                              \
    } while (false)

void ordinaryTickUsesConfiguredStep() {
    runtime::FixedStepUpdate scheduler{0.02, 4};
    std::vector<double> updates;
    const auto result = scheduler.advance(0.025, [&](double step) { updates.push_back(step); });
    CHECK(result.steps == 1);
    CHECK(updates == std::vector<double>{0.02});
    CHECK(result.discardedSteps == 0);
    CHECK(result.discardedTime == 0);
    CHECK(std::abs(result.remainder - 0.005) < 1e-12);
    CHECK(std::abs(scheduler.remainder() - 0.005) < 1e-12);
}

void longPauseDropsOnlyWholeExcessSteps() {
    runtime::FixedStepUpdate scheduler{0.1, 3};
    int updateCount = 0;
    const auto result = scheduler.advance(0.575, [&](double step) {
        CHECK(step == 0.1);
        ++updateCount;
    });
    CHECK(result.steps == 3);
    CHECK(updateCount == 3);
    CHECK(result.discardedSteps == 2);
    CHECK(std::abs(result.discardedTime - 0.2) < 1e-12);
    CHECK(std::abs(result.remainder - 0.075) < 1e-12);

    const auto next = scheduler.advance(0.025, [&](double) { ++updateCount; });
    CHECK(next.steps == 1);
    CHECK(updateCount == 4);
    CHECK(next.discardedSteps == 0);
    CHECK(std::abs(next.remainder) < 1e-12);
}

void remainderAccumulatesAcrossFrames() {
    runtime::FixedStepUpdate scheduler{0.25, 2};
    int updateCount = 0;
    auto update = [&](double) { ++updateCount; };
    CHECK(scheduler.advance(0.1, update).steps == 0);
    CHECK(scheduler.advance(0.1, update).steps == 0);
    const auto result = scheduler.advance(0.1, update);
    CHECK(result.steps == 1);
    CHECK(updateCount == 1);
    CHECK(std::abs(result.remainder - 0.05) < 1e-12);
}

void extremeDiscardCountHasFiniteSafeResult() {
    runtime::FixedStepUpdate scheduler{1e-300, 1};
    const auto result = scheduler.advance(1.0, [](double) {});
    CHECK(result.steps == 1);
    CHECK(result.discardedSteps == std::numeric_limits<std::size_t>::max());
    CHECK(std::isfinite(result.discardedTime));
    CHECK(result.discardedTime >= 0);
    CHECK(result.remainder >= 0);
    CHECK(result.remainder < 1e-300);
}

void invalidConfigurationAndElapsedAreRejected() {
    CHECK_THROWS_AS(runtime::FixedStepUpdate(0, 1), std::invalid_argument);
    CHECK_THROWS_AS(runtime::FixedStepUpdate(0.1, 0), std::invalid_argument);
    runtime::FixedStepUpdate scheduler{0.1, 1};
    CHECK_THROWS_AS(scheduler.advance(-0.1, [](double) {}), std::invalid_argument);
    CHECK(scheduler.remainder() == 0);
}

int main() {
    try {
        ordinaryTickUsesConfiguredStep();
        longPauseDropsOnlyWholeExcessSteps();
        remainderAccumulatesAcrossFrames();
        extremeDiscardCountHasFiniteSafeResult();
        invalidConfigurationAndElapsedAreRejected();
        std::cout << "fixed_step: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
