#include "huynity/adapters/renderer_actor/fake_renderer_adapter.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace runtime = huynity::runtime;
namespace adapter = huynity::adapters::renderer_actor;
namespace render = huynity::render_contracts;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

runtime::RenderFrame makeFrame(std::uint64_t id, const std::shared_ptr<const render::Material>& marker) {
    auto snapshot = std::make_unique<render::RenderSnapshot>();
    snapshot->materials.push_back(marker);
    return {id, std::move(snapshot), nullptr};
}

void acceptedTransfersOwnershipAndCompletionReleasesIt() {
    adapter::FakeRendererAdapter adapter;
    auto marker = std::make_shared<const render::Material>();
    std::weak_ptr<const render::Material> weak = marker;
    auto frame = makeFrame(7, marker);
    CHECK(adapter.trySubmit(frame) == runtime::SubmitResult::Accepted);
    CHECK(frame.snapshot == nullptr);
    CHECK(adapter.isBusy());
    CHECK(adapter.activeFrameId() == 7);
    CHECK(!weak.expired());

    auto completed = adapter.complete();
    CHECK(!adapter.isBusy());
    CHECK(completed.frameId == 7);
    CHECK(completed.snapshot->materials[0] == marker);
    marker.reset();
    completed = {};
    CHECK(weak.expired());
}

void rejectedPreservesActiveAndCallerResourceAndDoesNotStickBusy() {
    adapter::FakeRendererAdapter adapter;
    auto firstMarker = std::make_shared<const render::Material>();
    auto secondMarker = std::make_shared<const render::Material>();
    std::weak_ptr<const render::Material> rejectedResource = secondMarker;
    auto first = makeFrame(1, firstMarker);
    CHECK(adapter.trySubmit(first) == runtime::SubmitResult::Accepted);
    const auto before = rejectedResource.use_count();

    auto rejected = makeFrame(2, secondMarker);
    CHECK(adapter.trySubmit(rejected) == runtime::SubmitResult::Rejected);
    CHECK(rejected.snapshot != nullptr);
    CHECK(rejected.snapshot->materials[0] == secondMarker);
    CHECK(adapter.activeFrameId() == 1);
    CHECK(rejectedResource.use_count() == before + 1);
    CHECK(secondMarker != nullptr);

    auto completed = adapter.complete();
    CHECK(completed.frameId == 1);
    CHECK(!adapter.isBusy());
    auto third = makeFrame(3, secondMarker);
    CHECK(adapter.trySubmit(third) == runtime::SubmitResult::Accepted);
    CHECK(adapter.activeFrameId() == 3);
}

int main() {
    try {
        acceptedTransfersOwnershipAndCompletionReleasesIt();
        rejectedPreservesActiveAndCallerResourceAndDoesNotStickBusy();
        std::cout << "fake_adapter: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
