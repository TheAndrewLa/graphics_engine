#include "huynity/adapters/renderer_actor/fake_renderer_adapter.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace runtime = huynity::runtime;
namespace adapter = huynity::adapters::renderer_actor;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

runtime::RenderFrame frame(std::uint64_t id) {
    return {id, std::make_unique<huynity::render_contracts::RenderSnapshot>(), nullptr};
}

void readyIsReservedCorrelatedAndDeliveredOnce() {
    runtime::CompletionChannel channel;
    adapter::FakeRendererAdapter adapter{channel};
    runtime::FrameTracker tracker;
    auto first = frame(10);
    CHECK(tracker.submit(adapter, first) == runtime::SubmitResult::Accepted);
    CHECK(channel.hasReservedSlot());
    CHECK(adapter.completeReady());
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Ready);
    CHECK(!tracker.hasActiveFrame());
    CHECK(!channel.hasReservedSlot());
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Ignored);
}

void failedCompletionReleasesActiveFrame() {
    runtime::CompletionChannel channel;
    adapter::FakeRendererAdapter adapter{channel};
    runtime::FrameTracker tracker;
    auto submitted = frame(20);
    CHECK(tracker.submit(adapter, submitted) == runtime::SubmitResult::Accepted);
    CHECK(adapter.completeFailed("failed"));
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Failed);
    CHECK(!tracker.hasActiveFrame());
}

void staleAndDuplicateIdsDoNotChangeCurrentFrame() {
    runtime::CompletionChannel channel;
    adapter::FakeRendererAdapter adapter{channel};
    runtime::FrameTracker tracker;
    auto submitted = frame(30);
    CHECK(tracker.submit(adapter, submitted) == runtime::SubmitResult::Accepted);

    CHECK(channel.publish(runtime::FrameReady{999, frame(999)}));
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Ignored);
    CHECK(tracker.activeFrameId() == 30);
    CHECK(tracker.ignoredCompletions() == 1);

    CHECK(adapter.completeReady());
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Ready);
    CHECK(channel.reserve());
    CHECK(channel.publish(runtime::FrameReady{30, frame(30)}));
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Ignored);
    CHECK(!tracker.hasActiveFrame());
    CHECK(tracker.ignoredCompletions() == 2);
}

void pendingCompletionBackpressuresNextSubmission() {
    runtime::CompletionChannel channel;
    adapter::FakeRendererAdapter adapter{channel};
    runtime::FrameTracker tracker;
    auto first = frame(40);
    CHECK(tracker.submit(adapter, first) == runtime::SubmitResult::Accepted);
    CHECK(adapter.completeFailed("failed"));
    auto second = frame(41);
    CHECK(tracker.submit(adapter, second) == runtime::SubmitResult::Rejected);
    CHECK(second.snapshot != nullptr);
    CHECK(tracker.complete(channel) == runtime::CompletionResult::Failed);
    CHECK(tracker.submit(adapter, second) == runtime::SubmitResult::Accepted);
}

int main() {
    try {
        readyIsReservedCorrelatedAndDeliveredOnce();
        failedCompletionReleasesActiveFrame();
        staleAndDuplicateIdsDoNotChangeCurrentFrame();
        pendingCompletionBackpressuresNextSubmission();
        std::cout << "completions: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
