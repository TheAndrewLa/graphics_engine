#include "huynity/adapters/renderer_actor/fake_renderer_adapter.h"
#include "huynity/adapters/window/headless_window.h"
#include "huynity/adapters/window/input_events.h"
#include "huynity/adapters/window/input_state.h"
#include "huynity/adapters/window/window_adapter.h"
#include "huynity/image/image_buffer.h"
#include "huynity/render_contracts/render_snapshot.h"
#include "huynity/runtime/fixed_step_update.h"
#include "huynity/runtime/frame_completions.h"
#include "huynity/runtime/frame_pipeline.h"
#include "huynity/runtime/main_loop.h"
#include "huynity/runtime/presentation_port.h"
#include "huynity/runtime/renderer_port.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace window = huynity::adapters::window;
namespace actor = huynity::adapters::renderer_actor;
namespace runtime = huynity::runtime;
namespace image = huynity::image;
namespace render = huynity::render_contracts;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

namespace {

runtime::RenderFrame makeTestFrame(std::uint64_t id, std::unique_ptr<image::ImageBuffer> framebuffer = nullptr) {
    return runtime::RenderFrame{
        .frameId = id,
        .snapshot = std::make_unique<render::RenderSnapshot>(),
        .framebuffer = std::move(framebuffer),
    };
}

/// @brief Сценарий 1: Нормальный цикл кадра со связкой WindowAdapter, FramePipeline и InputState.
void testNormalFrameCycle() {
    auto headless = std::make_unique<window::HeadlessWindow>(window::WindowSize{.width = 64, .height = 64});
    window::WindowAdapter& windowAdapter = *headless;

    // 1. Окно размером 64x64 генерирует события ввода (KeyPressed, MouseMoved)
    headless->pushEvent(window::KeyPressedEvent{.key = window::KeyCode::Space});
    headless->pushEvent(window::MouseMovedEvent{.x = 32.0, .y = 32.0, .deltaX = 4.0, .deltaY = -2.0});

    // 2. В фазе событий window->pollEvents(events) события извлекаются и обновляют InputState
    std::vector<window::InputEvent> events;
    windowAdapter.pollEvents(events);
    CHECK(events.size() == 2);

    window::InputState inputState;
    for (const auto& event : events) {
        inputState.processEvent(event);
    }
    CHECK(inputState.isKeyPressed(window::KeyCode::Space));
    CHECK(inputState.mouseX() == 32.0);
    CHECK(inputState.mouseY() == 32.0);
    CHECK(inputState.mouseDeltaX() == 4.0);
    CHECK(inputState.mouseDeltaY() == -2.0);

    // 3. FramePipeline::resize синхронизируется с window->size()
    runtime::CompletionChannel channel;
    actor::FakeRendererAdapter renderer{channel};
    runtime::FramePipeline pipeline;

    const auto winSize = windowAdapter.size();
    pipeline.resize(runtime::SurfaceSize{.width = winSize.width, .height = winSize.height});
    CHECK(pipeline.surface().hasArea());
    CHECK(pipeline.surface().size().width == 64);
    CHECK(pipeline.surface().size().height == 64);
    CHECK(pipeline.surface().generation() == 1);

    // 4. Снимок готовится и отправляется в рендерер через pipeline.submit(), результат возвращается через FrameReady
    auto buffer = pipeline.acquireFramebuffer();
    CHECK(buffer != nullptr);
    CHECK(buffer->width() == 64);
    CHECK(buffer->height() == 64);
    buffer->bytes()[0] = std::byte{0x7F};

    auto frame = makeTestFrame(1, std::move(buffer));
    CHECK(pipeline.submit(renderer, frame) == runtime::SubmitResult::Accepted);
    CHECK(renderer.isBusy());
    CHECK(renderer.activeFrameId() == 1);

    CHECK(renderer.completeReady());
    CHECK(!renderer.isBusy());

    auto completed = pipeline.complete(channel);
    CHECK(completed.outcome == runtime::FrameOutcome::Ready);
    CHECK(completed.completion.has_value());

    // 5. window->present(framebuffer) успешно отображает буфер и возвращает его в пул
    // pipeline.releaseFromPresentation()
    pipeline.handleCompletedFrame(std::move(completed), windowAdapter);
    CHECK(headless->presentedCount() == 1);
    CHECK(headless->rejectedPresents() == 0);
    CHECK(headless->lastPresentedFrame() != nullptr);
    CHECK(headless->lastPresentedFrame()->width() == 64);
    CHECK(headless->lastPresentedFrame()->height() == 64);
    CHECK(headless->lastPresentedFrame()->bytes()[0] == std::byte{0x7F});
    CHECK(pipeline.pool().pooledCount() == 1);

    // Дополнительно: прямой present с ручным возвратом через releaseFromPresentation
    auto directBuffer = pipeline.acquireFramebuffer();
    CHECK(pipeline.pool().pooledCount() == 0);
    directBuffer->bytes()[0] = std::byte{0x55};
    auto returned = windowAdapter.present(std::move(directBuffer));
    CHECK(returned != nullptr);
    CHECK(headless->presentedCount() == 2);
    CHECK(headless->lastPresentedFrame()->bytes()[0] == std::byte{0x55});
    pipeline.releaseFromPresentation(std::move(returned));
    CHECK(pipeline.pool().pooledCount() == 1);
}

/// @brief Сценарий 1 (дополнение): Выполнение шага через runtime::runIteration (MainLoop).
void testNormalCycleWithMainLoop() {
    auto headless = std::make_unique<window::HeadlessWindow>(window::WindowSize{.width = 64, .height = 64});
    window::WindowAdapter& windowAdapter = *headless;

    runtime::CompletionChannel channel;
    actor::FakeRendererAdapter renderer{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize(runtime::SurfaceSize{.width = windowAdapter.size().width, .height = windowAdapter.size().height});
    runtime::FixedStepUpdate update{1.0 / 60.0, 4};
    window::InputState inputState;

    // Итерация 1: генерация ввода, фиксированный шаг обновления, подготовка и dispatch кадра
    headless->pushEvent(window::KeyPressedEvent{.key = window::KeyCode::W});
    headless->pushEvent(window::MouseMovedEvent{.x = 10.0, .y = 20.0, .deltaX = 2.0, .deltaY = 3.0});

    std::vector<window::InputEvent> events;
    windowAdapter.pollEvents(events);
    for (const auto& ev : events) {
        inputState.processEvent(ev);
    }
    CHECK(inputState.isKeyPressed(window::KeyCode::W));

    runtime::LoopEvents loopEvents;
    const auto winSize = windowAdapter.size();
    loopEvents.resize = runtime::SurfaceSize{.width = winSize.width, .height = winSize.height};

    int updateStepsExecuted = 0;
    auto iterResult1 = runtime::runIteration(
        renderer, pipeline, channel, update, loopEvents, 1.0 / 60.0,
        /*commands=*/[] {},
        /*updatePhase=*/[&](double) { ++updateStepsExecuted; },
        /*prepare=*/
        [&] {
            auto buf = pipeline.acquireFramebuffer();
            buf->bytes()[0] = std::byte{0x33};
            auto frame = makeTestFrame(100, std::move(buf));
            return pipeline.storePrepared(frame);
        });

    CHECK(updateStepsExecuted == 1);
    CHECK(iterResult1.prepared);
    CHECK(iterResult1.dispatch == runtime::DispatchResult::Submitted);
    CHECK(renderer.isBusy());
    CHECK(renderer.activeFrameId() == 100);

    // Рендерер завершает работу
    CHECK(renderer.completeReady());

    // Итерация 2: приём FrameReady, показ через окно, возврат буфера в пул
    auto iterResult2 = runtime::runIteration(
        renderer, pipeline, channel, update, runtime::LoopEvents{}, 1.0 / 60.0,
        /*commands=*/[] {},
        /*updatePhase=*/[](double) {},
        /*prepare=*/[&] { return false; });

    CHECK(iterResult2.completion.outcome == runtime::FrameOutcome::Ready);
    pipeline.handleCompletedFrame(std::move(iterResult2.completion), windowAdapter);
    CHECK(headless->presentedCount() == 1);
    CHECK(headless->lastPresentedFrame()->bytes()[0] == std::byte{0x33});
    CHECK(pipeline.pool().pooledCount() == 1);
}

/// @brief Сценарий 2: Реакция на изменение размера окна, сброс stale-снимков и отклонение несовпадающих буферов.
void testWindowResizeAndStaleFrameDropping() {
    auto headless = std::make_unique<window::HeadlessWindow>(window::WindowSize{.width = 64, .height = 64});
    window::WindowAdapter& windowAdapter = *headless;

    runtime::CompletionChannel channel;
    actor::FakeRendererAdapter renderer{channel};
    runtime::FramePipeline pipeline;
    pipeline.resize(runtime::SurfaceSize{.width = 64, .height = 64});
    runtime::FixedStepUpdate update{1.0 / 60.0, 4};

    CHECK(pipeline.surface().generation() == 1);

    // 1. Кадр отправлен в рендерер при поколении 1 (размер 64x64)
    auto frame1 = makeTestFrame(10, pipeline.acquireFramebuffer());
    CHECK(pipeline.submit(renderer, frame1) == runtime::SubmitResult::Accepted);
    CHECK(renderer.isBusy());

    // 2. window->resize(128, 96) генерирует WindowResizedEvent
    headless->resize(128, 96);
    CHECK(headless->size() == window::WindowSize{.width = 128, .height = 96});

    std::vector<window::InputEvent> events;
    windowAdapter.pollEvents(events);
    CHECK(events.size() == 1);
    CHECK(std::holds_alternative<window::WindowResizedEvent>(events[0]));
    const auto& resizeEvent = std::get<window::WindowResizedEvent>(events[0]);
    CHECK(resizeEvent.width == 128);
    CHECK(resizeEvent.height == 96);

    // Рендерер завершает кадр старого поколения
    CHECK(renderer.completeReady());

    // 3. При обработке событий в главном цикле pipeline.resize({128, 96}) обновляет surfaceGeneration.
    // Снимок прежнего поколения отбрасывается как устаревший (FrameOutcome::Stale).
    runtime::LoopEvents loopEvents;
    loopEvents.resize = runtime::SurfaceSize{.width = resizeEvent.width, .height = resizeEvent.height};

    auto iterResult = runtime::runIteration(
        renderer, pipeline, channel, update, loopEvents, 1.0 / 60.0,
        /*commands=*/[] {},
        /*updatePhase=*/[](double) {},
        /*prepare=*/[&] { return false; });

    CHECK(pipeline.surface().generation() == 2);
    CHECK(iterResult.completion.outcome == runtime::FrameOutcome::Stale);

    // handleCompletedFrame отбрасывает stale-кадр без показа в window
    pipeline.handleCompletedFrame(std::move(iterResult.completion), windowAdapter);
    CHECK(headless->presentedCount() == 0);
    CHECK(headless->lastPresentedFrame() == nullptr);

    // 4. Старый буфер 64x64, переданный в window->present(), отклоняется (счётчик rejectedPresents растёт)
    auto staleBufResult = image::ImageBuffer::create(image::ImageBufferDesc{.width = 64, .height = 64});
    CHECK(staleBufResult.has_value());
    auto staleBuf = std::make_unique<image::ImageBuffer>(std::move(*staleBufResult));
    auto* staleBufRaw = staleBuf.get();

    auto rejectedRet = windowAdapter.present(std::move(staleBuf));
    CHECK(rejectedRet.get() == staleBufRaw);
    CHECK(headless->rejectedPresents() == 1);
    CHECK(headless->presentedCount() == 0);

    // 5. Новый буфер 128x96 принимается
    auto newBuf = pipeline.acquireFramebuffer();
    CHECK(newBuf->width() == 128);
    CHECK(newBuf->height() == 96);
    newBuf->bytes()[0] = std::byte{0xAA};

    auto acceptedRet = windowAdapter.present(std::move(newBuf));
    CHECK(acceptedRet != nullptr);
    CHECK(headless->presentedCount() == 1);
    CHECK(headless->rejectedPresents() == 1);
    CHECK(headless->lastPresentedFrame() != nullptr);
    CHECK(headless->lastPresentedFrame()->width() == 128);
    CHECK(headless->lastPresentedFrame()->height() == 96);
    CHECK(headless->lastPresentedFrame()->bytes()[0] == std::byte{0xAA});

    pipeline.releaseFromPresentation(std::move(acceptedRet));
    CHECK(pipeline.pool().pooledCount() == 1);

    // 6. Проверка сброса подготовленного снимка в prepared-слоте при смене поколений поверхности
    auto preparedFrame = makeTestFrame(20, pipeline.acquireFramebuffer());
    CHECK(pipeline.storePrepared(preparedFrame));
    CHECK(pipeline.hasPreparedFrame());

    // Меняем размер окна ещё раз: 200x150
    headless->resize(200, 150);
    events.clear();
    windowAdapter.pollEvents(events);
    CHECK(events.size() == 1);
    const auto& res2 = std::get<window::WindowResizedEvent>(events[0]);
    pipeline.resize(runtime::SurfaceSize{.width = res2.width, .height = res2.height});

    CHECK(pipeline.surface().generation() == 3);
    // Prepared-снимок прежнего поколения сброшен при resize (DroppedStale)
    CHECK(!pipeline.hasPreparedFrame());
    CHECK(pipeline.canPrepare());
}

/// @brief Сценарий 3: Завершение работы по WindowCloseRequestedEvent и прямому вызову window.close().
void testTerminationLifecycle() {
    // 3.1 Завершение работы по WindowCloseRequestedEvent
    {
        auto headless = std::make_unique<window::HeadlessWindow>(window::WindowSize{.width = 64, .height = 64});
        window::WindowAdapter& windowAdapter = *headless;

        runtime::CompletionChannel channel;
        actor::FakeRendererAdapter renderer{channel};
        runtime::FramePipeline pipeline;
        pipeline.resize(runtime::SurfaceSize{.width = 64, .height = 64});
        runtime::FixedStepUpdate update{1.0 / 60.0, 4};

        headless->pushEvent(window::WindowCloseRequestedEvent{});

        bool stopRequested = false;
        int loopIterations = 0;

        while (windowAdapter.isOpen() && !stopRequested) {
            ++loopIterations;
            std::vector<window::InputEvent> events;
            windowAdapter.pollEvents(events);

            for (const auto& ev : events) {
                if (std::holds_alternative<window::WindowCloseRequestedEvent>(ev)) {
                    stopRequested = true;
                    pipeline.requestStop();
                    renderer.requestStop();
                    windowAdapter.close();
                    break;
                }
            }

            if (stopRequested) {
                break;
            }

            runtime::runIteration(
                renderer, pipeline, channel, update, runtime::LoopEvents{}, 1.0 / 60.0, [] {}, [](double) {},
                [] { return false; });
        }

        CHECK(stopRequested);
        CHECK(!windowAdapter.isOpen());
        CHECK(loopIterations == 1);
        CHECK(pipeline.tracker().isStopRequested());

        pipeline.confirmStopped();
        renderer.confirmStopped();
        CHECK(renderer.isStopped());
        CHECK(pipeline.tracker().isStopped());
    }

    // 3.2 Завершение работы по прямому window.close()
    {
        auto headless = std::make_unique<window::HeadlessWindow>(window::WindowSize{.width = 64, .height = 64});
        window::WindowAdapter& windowAdapter = *headless;

        runtime::CompletionChannel channel;
        actor::FakeRendererAdapter renderer{channel};
        runtime::FramePipeline pipeline;
        pipeline.resize(runtime::SurfaceSize{.width = 64, .height = 64});
        runtime::FixedStepUpdate update{1.0 / 60.0, 4};

        CHECK(windowAdapter.isOpen());
        windowAdapter.close();
        CHECK(!windowAdapter.isOpen());

        int loopIterations = 0;
        while (windowAdapter.isOpen()) {
            ++loopIterations;
            runtime::runIteration(
                renderer, pipeline, channel, update, runtime::LoopEvents{}, 1.0 / 60.0, [] {}, [](double) {},
                [] { return false; });
        }
        CHECK(loopIterations == 0);

        // Попытка present в закрытое окно отклоняется
        auto bufResult = image::ImageBuffer::create(image::ImageBufferDesc{.width = 64, .height = 64});
        CHECK(bufResult.has_value());
        auto buf = std::make_unique<image::ImageBuffer>(std::move(*bufResult));
        auto ret = windowAdapter.present(std::move(buf));
        CHECK(ret != nullptr);
        CHECK(headless->rejectedPresents() == 1);
        CHECK(headless->presentedCount() == 0);
    }
}

} // namespace

int main() {
    try {
        testNormalFrameCycle();
        testNormalCycleWithMainLoop();
        testWindowResizeAndStaleFrameDropping();
        testTerminationLifecycle();

        std::cout << "window_integration_tests: OK\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
