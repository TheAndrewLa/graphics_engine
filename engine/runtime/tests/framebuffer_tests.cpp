#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

#include "fake_port.h"

namespace image = huynity::image;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

// Записывает факт показа; в синхронном режиме возвращает буфер сразу, в асинхронном
// удерживает его до явного подтверждения окончания использования.
class FakePresentationPort final : public runtime::PresentationPort {
public:
    explicit FakePresentationPort(bool retain) : m_retain{retain} {}

    [[nodiscard]] std::unique_ptr<image::ImageBuffer>
    present(std::unique_ptr<image::ImageBuffer> framebuffer) override {
        ++m_presentCount;
        m_lastWidth = framebuffer->width();
        m_lastHeight = framebuffer->height();
        if (m_retain) {
            m_retained = std::move(framebuffer);
            return nullptr;
        }
        return framebuffer;
    }

    [[nodiscard]] std::unique_ptr<image::ImageBuffer> takeRetained() { return std::move(m_retained); }
    [[nodiscard]] std::size_t presentCount() const noexcept { return m_presentCount; }
    [[nodiscard]] std::size_t lastWidth() const noexcept { return m_lastWidth; }
    [[nodiscard]] std::size_t lastHeight() const noexcept { return m_lastHeight; }

private:
    std::unique_ptr<image::ImageBuffer> m_retained;
    std::size_t m_presentCount{};
    std::size_t m_lastWidth{};
    std::size_t m_lastHeight{};
    bool m_retain;
};

runtime::RenderFrame makeFrame(std::uint64_t id) {
    return {id, std::make_unique<huynity::render_contracts::RenderSnapshot>(), nullptr};
}

void acquireReusesReturnedBufferOfCurrentSize() {
    runtime::FramebufferPool pool;
    pool.resize(64, 48);
    auto first = pool.acquire();
    CHECK(first != nullptr);
    CHECK(first->width() == 64);
    CHECK(first->height() == 48);
    const auto* identity = first.get();
    CHECK(pool.createdCount() == 1);

    pool.release(std::move(first));
    CHECK(pool.pooledCount() == 1);
    auto second = pool.acquire();
    CHECK(second.get() == identity);
    CHECK(pool.createdCount() == 1);
}

void poolIsCappedAndDropsExtras() {
    runtime::FramebufferPool pool{2};
    pool.resize(64, 48);
    auto first = pool.acquire();
    auto second = pool.acquire();
    auto third = pool.acquire();
    CHECK(pool.createdCount() == 3);

    pool.release(std::move(first));
    pool.release(std::move(second));
    pool.release(std::move(third));
    CHECK(pool.pooledCount() == 2);
}

void resizeDropsStaleSizeBuffers() {
    runtime::FramebufferPool pool;
    pool.resize(64, 48);
    auto pooled = pool.acquire();
    pool.release(std::move(pooled));
    CHECK(pool.pooledCount() == 1);

    pool.resize(128, 96);
    CHECK(pool.pooledCount() == 0);

    // буфер прежнего размера, возвращённый после resize, уничтожается, а не копится
    runtime::FramebufferPool second;
    second.resize(64, 48);
    auto stale = second.acquire();
    second.resize(128, 96);
    second.release(std::move(stale));
    CHECK(second.pooledCount() == 0);
    auto fresh = second.acquire();
    CHECK(fresh->width() == 128);
    CHECK(fresh->height() == 96);
    CHECK(second.createdCount() == 2);
}

void zeroAreaAcquireReturnsNull() {
    runtime::FramebufferPool pool;
    CHECK(pool.acquire() == nullptr);
}

void syncPresentationReturnsBufferToPool() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({64, 48});
    FakePresentationPort presentation{false};

    auto frame = makeFrame(1);
    frame.framebuffer = pipeline.acquireFramebuffer();
    const auto* identity = frame.framebuffer.get();
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(frame.framebuffer == nullptr); // уникальное владение передано исполнителю
    CHECK(port.completeReady());

    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Ready);
    const auto* ready = std::get_if<runtime::FrameReady>(&*completed.completion);
    CHECK(ready != nullptr);
    CHECK(ready->frame.framebuffer.get() == identity);

    pipeline.handleCompletedFrame(std::move(completed), presentation);
    CHECK(presentation.presentCount() == 1);
    CHECK(presentation.lastWidth() == 64);
    CHECK(pipeline.pool().pooledCount() == 1);

    auto reused = pipeline.acquireFramebuffer();
    CHECK(reused.get() == identity);
    CHECK(pipeline.pool().createdCount() == 1);
}

void asyncPresentationRetainsBufferUntilRelease() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({64, 48});
    FakePresentationPort presentation{true};

    auto frame = makeFrame(2);
    frame.framebuffer = pipeline.acquireFramebuffer();
    const auto* identity = frame.framebuffer.get();
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeReady());

    auto completed = pipeline.complete(channel);
    pipeline.handleCompletedFrame(std::move(completed), presentation);
    CHECK(presentation.presentCount() == 1);
    CHECK(pipeline.pool().pooledCount() == 0);

    // пока Presentation удерживает буфер, пул выдаёт новый
    auto parallel = pipeline.acquireFramebuffer();
    CHECK(parallel.get() != identity);
    CHECK(pipeline.pool().createdCount() == 2);

    auto retained = presentation.takeRetained();
    CHECK(retained.get() == identity);
    pipeline.releaseFromPresentation(std::move(retained));
    CHECK(pipeline.pool().pooledCount() == 1);
}

void failedFrameReturnsBufferWithoutPresentation() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({64, 48});
    FakePresentationPort presentation{false};

    auto frame = makeFrame(3);
    frame.framebuffer = pipeline.acquireFramebuffer();
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeFailed("boom"));

    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Failed);
    pipeline.handleCompletedFrame(std::move(completed), presentation);
    CHECK(presentation.presentCount() == 0);
    CHECK(pipeline.pool().pooledCount() == 1);
}

void staleFrameIsNotPresentedAndOldBufferIsDestroyed() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({64, 48});
    FakePresentationPort presentation{false};

    auto frame = makeFrame(4);
    frame.framebuffer = pipeline.acquireFramebuffer();
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeReady());

    pipeline.resize({128, 96});
    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Stale);
    pipeline.handleCompletedFrame(std::move(completed), presentation);
    CHECK(presentation.presentCount() == 0);
    CHECK(pipeline.pool().pooledCount() == 0); // буфер прежнего размера не копится

    auto fresh = pipeline.acquireFramebuffer();
    CHECK(fresh->width() == 128);
    CHECK(pipeline.pool().createdCount() == 2);
}

void closeWithRetainedBufferReleasesWithoutDoubleFree() {
    runtime::CompletionChannel channel;
    FakeRendererPort port{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize({64, 48});
    FakePresentationPort presentation{true};

    auto frame = makeFrame(5);
    frame.framebuffer = pipeline.acquireFramebuffer();
    CHECK(pipeline.submit(port, frame) == runtime::SubmitResult::Accepted);
    CHECK(port.completeReady());
    auto completed = pipeline.complete(channel);
    pipeline.handleCompletedFrame(std::move(completed), presentation);
    CHECK(pipeline.pool().pooledCount() == 0);

    pipeline.requestStop();
    pipeline.confirmStopped();
    CHECK(pipeline.tracker().isStopped());

    // подтверждение окончания использования после остановки: единственный владелец, один возврат
    pipeline.releaseFromPresentation(presentation.takeRetained());
    CHECK(pipeline.pool().pooledCount() == 1);
    CHECK(presentation.takeRetained() == nullptr);
}

int main() {
    try {
        acquireReusesReturnedBufferOfCurrentSize();
        poolIsCappedAndDropsExtras();
        resizeDropsStaleSizeBuffers();
        zeroAreaAcquireReturnsNull();
        syncPresentationReturnsBufferToPool();
        asyncPresentationRetainsBufferUntilRelease();
        failedFrameReturnsBufferWithoutPresentation();
        staleFrameIsNotPresentedAndOldBufferIsDestroyed();
        closeWithRetainedBufferReleasesWithoutDoubleFree();
        std::cout << "framebuffer: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
