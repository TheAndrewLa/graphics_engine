#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "fake_port.h"

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

runtime::RenderFrame makeFrame(std::uint64_t id) {
    return {id, std::make_unique<huynity::render_contracts::RenderSnapshot>()};
}

void phasesRunInFixedOrderAndResizePrecedesCompletion() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({640, 480});
    runtime::FixedStepUpdate update{0.1, 4};
    auto frame = makeFrame(10);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeReady());

    std::vector<std::string> order;
    runtime::LoopEvents events;
    events.resize = runtime::SurfaceSize{320, 200};
    const auto result = runtime::runIteration(
        port, pipeline, channel, update, events, 0.25, [&] { order.emplace_back("commands"); },
        [&](double) { order.emplace_back("update"); },
        [&] {
            order.emplace_back("prepare");
            return true;
        });

    // resize применён до FrameReady: готовый кадр прежнего поколения отброшен без показа
    CHECK(result.completion.outcome == runtime::FrameOutcome::Stale);
    CHECK(order == std::vector<std::string>{"commands", "update", "update", "prepare"});
    CHECK(result.update.steps == 2);
    CHECK(result.prepared);
    CHECK(result.dispatch == runtime::DispatchResult::Empty);
}

void prepareObservesStateAfterCommandsAndUpdate() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({640, 480});
    runtime::FixedStepUpdate update{0.1, 4};
    int state = 0;
    int observed = -1;

    const auto result = runtime::runIteration(
        port, pipeline, channel, update, runtime::LoopEvents{}, 0.25, [&] { state = 1; }, [&](double) { state += 10; },
        [&] {
            observed = state;
            auto next = makeFrame(11);
            return pipeline.storePrepared(next);
        });

    CHECK(result.update.steps == 2);
    CHECK(observed == 21);
    CHECK(result.prepared);
    CHECK(result.dispatch == runtime::DispatchResult::Submitted);
    CHECK(port.acceptedIds() == std::vector<std::uint64_t>{11});
    CHECK(port.isBusy());
}

void zeroAreaSurfaceSkipsPrepare() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    runtime::FixedStepUpdate update{0.1, 4};
    bool prepareCalled = false;
    runtime::LoopEvents events;
    events.resize = runtime::SurfaceSize{0, 480};

    const auto result = runtime::runIteration(
        port, pipeline, channel, update, events, 0.1, [] {}, [](double) {},
        [&] {
            prepareCalled = true;
            return true;
        });

    CHECK(!prepareCalled);
    CHECK(!result.prepared);
    CHECK(result.completion.outcome == runtime::FrameOutcome::None);
    CHECK(result.dispatch == runtime::DispatchResult::Empty);
}

void prepareOverlapsActiveRenderWithinDepthOnePlusOne() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({640, 480});
    runtime::FixedStepUpdate update{0.1, 4};
    auto frame = makeFrame(12);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    bool rendererBusyAtPrepare = false;

    // Renderer исполняет кадр 12, prepared-слот пуст: подготовка не ждёт растеризацию
    const auto first = runtime::runIteration(
        port, pipeline, channel, update, runtime::LoopEvents{}, 0.1, [] {}, [](double) {},
        [&] {
            rendererBusyAtPrepare = port.isBusy();
            auto next = makeFrame(13);
            return pipeline.storePrepared(next);
        });

    CHECK(rendererBusyAtPrepare);
    CHECK(first.prepared);
    CHECK(first.dispatch == runtime::DispatchResult::Empty);
    CHECK(port.isBusy());
    CHECK(port.activeFrameId() == 12);
    CHECK(pipeline.hasPreparedFrame());

    // Renderer освободился: prepared-снимок актуального поколения уходит сразу
    CHECK(port.completeReady());
    bool prepareCalled = false;
    const auto second = runtime::runIteration(
        port, pipeline, channel, update, runtime::LoopEvents{}, 0.1, [] {}, [](double) {},
        [&] {
            prepareCalled = true;
            return true;
        });

    CHECK(second.completion.outcome == runtime::FrameOutcome::Ready);
    CHECK(!prepareCalled);
    CHECK(second.dispatch == runtime::DispatchResult::Submitted);
    CHECK(port.isBusy());
    CHECK(port.activeFrameId() == 13);
    CHECK(!pipeline.hasPreparedFrame());
    CHECK(port.acceptedIds() == std::vector<std::uint64_t>{12, 13});
}

void occupiedPreparedSlotAccumulatesChangesInCore() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({640, 480});
    runtime::FixedStepUpdate update{0.1, 4};
    auto frame = makeFrame(20);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    int coreState = 0;

    const auto first = runtime::runIteration(
        port, pipeline, channel, update, runtime::LoopEvents{}, 0.1, [&] { ++coreState; }, [](double) {},
        [&] {
            auto next = makeFrame(21);
            return pipeline.storePrepared(next);
        });
    CHECK(first.prepared);
    CHECK(pipeline.hasPreparedFrame());

    // Слот занят, Renderer занят: новые снимки не создаются, изменения остаются в Core
    for (int iteration = 0; iteration < 3; ++iteration) {
        bool prepareCalled = false;
        const auto result = runtime::runIteration(
            port, pipeline, channel, update, runtime::LoopEvents{}, 0.1, [&] { ++coreState; }, [](double) {},
            [&] {
                prepareCalled = true;
                return true;
            });
        CHECK(!prepareCalled);
        CHECK(!result.prepared);
        CHECK(result.dispatch == runtime::DispatchResult::Empty);
        CHECK(port.isBusy());
        CHECK(port.activeFrameId() == 20);
        CHECK(pipeline.hasPreparedFrame());
    }
    CHECK(coreState == 4);
    CHECK(port.acceptedIds() == std::vector<std::uint64_t>{20});

    // После освобождения Renderer отправляется ровно один накопленный снимок
    CHECK(port.completeReady());
    const auto last = runtime::runIteration(
        port, pipeline, channel, update, runtime::LoopEvents{}, 0.1, [&] { ++coreState; }, [](double) {},
        [&] {
            auto next = makeFrame(22);
            return pipeline.storePrepared(next);
        });
    CHECK(last.completion.outcome == runtime::FrameOutcome::Ready);
    CHECK(!last.prepared);
    CHECK(last.dispatch == runtime::DispatchResult::Submitted);
    CHECK(port.activeFrameId() == 21);
    CHECK(port.acceptedIds() == std::vector<std::uint64_t>({20, 21}));
    CHECK(coreState == 5);
}

int main() {
    try {
        phasesRunInFixedOrderAndResizePrecedesCompletion();
        prepareObservesStateAfterCommandsAndUpdate();
        zeroAreaSurfaceSkipsPrepare();
        prepareOverlapsActiveRenderWithinDepthOnePlusOne();
        occupiedPreparedSlotAccumulatesChangesInCore();
        std::cout << "main_loop: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
