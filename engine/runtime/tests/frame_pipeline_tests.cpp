#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
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

void zeroAreaSurfaceBlocksNewFrames() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    CHECK(!pipeline.surface().hasArea());
    CHECK(!pipeline.canPrepare());

    auto frame = makeFrame(1);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Rejected);
    CHECK(frame.snapshot != nullptr);
    CHECK(!port.isBusy());

    pipeline.resize({800, 600});
    CHECK(pipeline.surface().generation() == 1);
    CHECK(pipeline.canPrepare());
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(frame.snapshot == nullptr);
}

void resizeBeforeFrameReadyDropsFrameWithoutPresentation() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto frame = makeFrame(2);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeReady());

    // resize обрабатывается до завершений: кадр прежнего поколения не показывается
    pipeline.resize({1024, 768});
    CHECK(pipeline.surface().generation() == 2);
    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Stale);
    CHECK(completed.completion.has_value());
    const auto* ready = std::get_if<runtime::FrameReady>(&*completed.completion);
    CHECK(ready != nullptr);
    CHECK(ready->frame.snapshot != nullptr);
    CHECK(!pipeline.hasActiveFrame());
    CHECK(pipeline.canPrepare());
}

void sameSizeResizeKeepsGenerationAndDeliversReady() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto frame = makeFrame(3);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeReady());

    pipeline.resize({800, 600});
    CHECK(pipeline.surface().generation() == 1);
    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Ready);
    CHECK(completed.completion.has_value());
    CHECK(std::get_if<runtime::FrameReady>(&*completed.completion) != nullptr);
}

void failedCompletionReturnsFrameAndError() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto frame = makeFrame(4);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeFailed("boom"));

    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Failed);
    const auto* failed = std::get_if<runtime::FrameFailed>(&*completed.completion);
    CHECK(failed != nullptr);
    CHECK(failed->error == "boom");
    CHECK(failed->frame.snapshot != nullptr);
}

void emptyAndMismatchedCompletionsAreDistinguished() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    CHECK(pipeline.complete(channel).outcome == runtime::FrameOutcome::None);

    auto frame = makeFrame(5);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(channel.publish(runtime::FrameReady{999, makeFrame(999)}));
    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Ignored);
    CHECK(!completed.completion.has_value());
    CHECK(pipeline.hasActiveFrame());
    CHECK(pipeline.tracker().ignoredCompletions() == 1);
}

void rendererDownCancelsPendingGeneration() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto frame = makeFrame(6);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    channel.markRendererDown();

    CHECK(pipeline.rendererDown(channel));
    CHECK(!pipeline.canPrepare());
    CHECK(pipeline.complete(channel).outcome == runtime::FrameOutcome::None);
}

void preparedSlotHoldsSingleSnapshot() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    CHECK(pipeline.canPrepare());

    auto first = makeFrame(7);
    CHECK(pipeline.storePrepared(first));
    CHECK(pipeline.hasPreparedFrame());
    CHECK(!pipeline.canPrepare());

    auto second = makeFrame(8);
    CHECK(!pipeline.storePrepared(second));
    CHECK(second.snapshot != nullptr);

    // Renderer свободен: снимок актуального поколения отправляется
    CHECK(pipeline.dispatchPrepared(port) == runtime::DispatchResult::Submitted);
    CHECK(!pipeline.hasPreparedFrame());
    CHECK(port.acceptedIds() == std::vector<std::uint64_t>{7});
}

void stalePreparedSnapshotIsDroppedOnResize() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto frame = makeFrame(9);
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    auto prepared = makeFrame(10);
    CHECK(pipeline.storePrepared(prepared));
    CHECK(pipeline.hasPreparedFrame());

    pipeline.resize({1024, 768});
    CHECK(!pipeline.hasPreparedFrame());
    CHECK(pipeline.canPrepare());
    CHECK(port.completeReady());
    CHECK(pipeline.complete(channel).outcome == runtime::FrameOutcome::Stale);
}

void dispatchRejectedByTransportKeepsSnapshot() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto prepared = makeFrame(11);
    CHECK(pipeline.storePrepared(prepared));

    // Транспорт не может принять: слот завершения занят чужим результатом
    CHECK(channel.reserve());
    CHECK(channel.publish(runtime::FrameReady{999, makeFrame(999)}));
    CHECK(pipeline.dispatchPrepared(port) == runtime::DispatchResult::Rejected);
    CHECK(pipeline.hasPreparedFrame());
    CHECK(!port.isBusy());

    static_cast<void>(channel.take());
    CHECK(pipeline.dispatchPrepared(port) == runtime::DispatchResult::Submitted);
    CHECK(port.acceptedIds() == std::vector<std::uint64_t>{11});
}

void stopRequestDropsPreparedSnapshot() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({800, 600});
    auto prepared = makeFrame(12);
    CHECK(pipeline.storePrepared(prepared));

    pipeline.requestStop();
    CHECK(!pipeline.hasPreparedFrame());
    CHECK(!pipeline.canPrepare());
    CHECK(pipeline.dispatchPrepared(port) == runtime::DispatchResult::Empty);
}

int main() {
    try {
        zeroAreaSurfaceBlocksNewFrames();
        resizeBeforeFrameReadyDropsFrameWithoutPresentation();
        sameSizeResizeKeepsGenerationAndDeliversReady();
        failedCompletionReturnsFrameAndError();
        emptyAndMismatchedCompletionsAreDistinguished();
        rendererDownCancelsPendingGeneration();
        preparedSlotHoldsSingleSnapshot();
        stalePreparedSnapshotIsDroppedOnResize();
        dispatchRejectedByTransportKeepsSnapshot();
        stopRequestDropsPreparedSnapshot();
        std::cout << "frame_pipeline: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
