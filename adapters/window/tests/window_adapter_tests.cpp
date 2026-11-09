#include "huynity/adapters/window/window_adapter.h"

#include <concepts>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace window = huynity::adapters::window;
namespace runtime = huynity::runtime;
namespace image = huynity::image;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

namespace {

// Compile-time checks: WindowAdapter inherits from PresentationPort and satisfies API contract
static_assert(std::is_base_of_v<runtime::PresentationPort, window::WindowAdapter>,
              "WindowAdapter must inherit from runtime::PresentationPort");
static_assert(std::has_virtual_destructor_v<window::WindowAdapter>, "WindowAdapter must have a virtual destructor");
static_assert(std::is_abstract_v<window::WindowAdapter>, "WindowAdapter must be an abstract interface");

// Verify member signatures via concept / invocable checks
template <typename T>
concept WindowAdapterContract =
    requires(T& adapter, const T& constAdapter, std::string_view title, std::vector<window::InputEvent>& events,
             std::unique_ptr<image::ImageBuffer> buffer) {
        { constAdapter.isOpen() } noexcept -> std::same_as<bool>;
        { adapter.close() } noexcept -> std::same_as<void>;
        { constAdapter.size() } noexcept -> std::same_as<window::WindowSize>;
        { adapter.setTitle(title) } -> std::same_as<void>;
        { adapter.pollEvents(events) } -> std::same_as<void>;
        { adapter.present(std::move(buffer)) } -> std::same_as<std::unique_ptr<image::ImageBuffer>>;
    };

static_assert(WindowAdapterContract<window::WindowAdapter>,
              "WindowAdapter interface must satisfy the WindowAdapterContract requirements");

/**
 * @brief Мок-реализация WindowAdapter для верификации контракта интерфейса.
 */
class MockWindow final : public window::WindowAdapter {
public:
    explicit MockWindow(window::WindowSize initialSize = {800, 600}, std::string_view initialTitle = "Test Window")
        : m_size(initialSize), m_title(initialTitle) {}

    [[nodiscard]] bool isOpen() const noexcept override { return m_isOpen; }

    void close() noexcept override { m_isOpen = false; }

    [[nodiscard]] window::WindowSize size() const noexcept override { return m_size; }

    void setTitle(std::string_view title) override { m_title = title; }

    void pollEvents(std::vector<window::InputEvent>& outEvents) override {
        for (const auto& ev : m_queuedEvents) {
            outEvents.push_back(ev);
        }
        m_queuedEvents.clear();
    }

    [[nodiscard]] std::unique_ptr<image::ImageBuffer>
    present(std::unique_ptr<image::ImageBuffer> framebuffer) override {
        ++m_presentCallCount;
        if (!m_isOpen || !framebuffer) {
            return framebuffer;
        }

        if (framebuffer->width() != m_size.width || framebuffer->height() != m_size.height) {
            ++m_mismatchedCount;
            return framebuffer;
        }

        m_lastPresentedBuffer = std::move(framebuffer);
        if (m_isSynchronous) {
            return std::move(m_lastPresentedBuffer);
        }

        return nullptr;
    }

    // Вспомогательные методы для тестирования
    void setSize(window::WindowSize newSize) noexcept { m_size = newSize; }

    [[nodiscard]] const std::string& title() const noexcept { return m_title; }

    void queueEvent(window::InputEvent event) { m_queuedEvents.push_back(std::move(event)); }

    void setSynchronous(bool sync) noexcept { m_isSynchronous = sync; }

    [[nodiscard]] std::size_t presentCallCount() const noexcept { return m_presentCallCount; }

    [[nodiscard]] std::size_t mismatchedCount() const noexcept { return m_mismatchedCount; }

    [[nodiscard]] bool hasRetainedBuffer() const noexcept { return m_lastPresentedBuffer != nullptr; }

private:
    window::WindowSize m_size{800, 600};
    std::string m_title;
    std::vector<window::InputEvent> m_queuedEvents;
    std::unique_ptr<image::ImageBuffer> m_lastPresentedBuffer;
    std::size_t m_presentCallCount{0};
    std::size_t m_mismatchedCount{0};
    bool m_isOpen{true};
    bool m_isSynchronous{true};
};

std::unique_ptr<image::ImageBuffer> createTestBuffer(std::size_t width, std::size_t height) {
    auto result = image::ImageBuffer::create(image::ImageBufferDesc{
        .width = width,
        .height = height,
    });
    if (!result) {
        throw std::runtime_error("Failed to create ImageBuffer for test");
    }
    return std::make_unique<image::ImageBuffer>(std::move(*result));
}

void testWindowSizeStruct() {
    const window::WindowSize defaultSize{};
    CHECK(defaultSize.width == 0);
    CHECK(defaultSize.height == 0);

    const window::WindowSize sizeA{.width = 1280, .height = 720};
    const window::WindowSize sizeB{.width = 1280, .height = 720};
    const window::WindowSize sizeC{.width = 1920, .height = 1080};

    CHECK(sizeA == sizeB);
    CHECK(!(sizeA == sizeC));
    CHECK(sizeA != sizeC);
}

void testWindowAdapterLifecycle() {
    MockWindow window({1024, 768}, "Initial Title");
    window::WindowAdapter& adapter = window;

    CHECK(adapter.isOpen());
    CHECK(adapter.size() == window::WindowSize{.width = 1024, .height = 768});
    CHECK(window.title() == "Initial Title");

    adapter.setTitle("Updated Title");
    CHECK(window.title() == "Updated Title");

    adapter.close();
    CHECK(!adapter.isOpen());
}

void testWindowAdapterEventPolling() {
    MockWindow window({800, 600});
    window::WindowAdapter& adapter = window;

    window.queueEvent(window::KeyPressedEvent{.key = window::KeyCode::Space, .isRepeat = false});
    window.queueEvent(window::WindowResizedEvent{.width = 1024, .height = 768});
    window.queueEvent(window::MouseButtonPressedEvent{.button = window::MouseButton::Left, .x = 50.0, .y = 100.0});

    std::vector<window::InputEvent> events;
    adapter.pollEvents(events);

    CHECK(events.size() == 3);
    CHECK(std::holds_alternative<window::KeyPressedEvent>(events[0]));
    CHECK(std::get<window::KeyPressedEvent>(events[0]).key == window::KeyCode::Space);

    CHECK(std::holds_alternative<window::WindowResizedEvent>(events[1]));
    CHECK(std::get<window::WindowResizedEvent>(events[1]).width == 1024);
    CHECK(std::get<window::WindowResizedEvent>(events[1]).height == 768);

    CHECK(std::holds_alternative<window::MouseButtonPressedEvent>(events[2]));
    CHECK(std::get<window::MouseButtonPressedEvent>(events[2]).button == window::MouseButton::Left);

    // Второе обращение не должно возвращать повторные события
    events.clear();
    adapter.pollEvents(events);
    CHECK(events.empty());
}

void testPresentationPortPolymorphismAndSynchronousPresent() {
    MockWindow window({800, 600});
    // Обращение строго через интерфейс runtime::PresentationPort
    runtime::PresentationPort& presentationPort = window;

    auto buffer = createTestBuffer(800, 600);
    const auto* rawBufferPtr = buffer.get();

    auto returnedBuffer = presentationPort.present(std::move(buffer));
    CHECK(returnedBuffer != nullptr);
    CHECK(returnedBuffer.get() == rawBufferPtr);
    CHECK(returnedBuffer->width() == 800);
    CHECK(returnedBuffer->height() == 600);
    CHECK(window.presentCallCount() == 1);
    CHECK(window.mismatchedCount() == 0);
}

void testPresentationPortMismatchedBufferRejection() {
    MockWindow window({800, 600});
    runtime::PresentationPort& presentationPort = window;

    // Передаём буфер другого разрешения (640x480 вместо 800x600)
    auto mismatchedBuffer = createTestBuffer(640, 480);
    const auto* rawBufferPtr = mismatchedBuffer.get();

    auto returnedBuffer = presentationPort.present(std::move(mismatchedBuffer));
    CHECK(returnedBuffer != nullptr);
    CHECK(returnedBuffer.get() == rawBufferPtr);
    CHECK(returnedBuffer->width() == 640);
    CHECK(returnedBuffer->height() == 480);
    CHECK(window.presentCallCount() == 1);
    CHECK(window.mismatchedCount() == 1);
}

void testPresentationPortAsynchronousPresent() {
    MockWindow window({800, 600});
    window.setSynchronous(false);

    runtime::PresentationPort& presentationPort = window;

    auto buffer = createTestBuffer(800, 600);
    auto returnedBuffer = presentationPort.present(std::move(buffer));

    // При асинхронном показе порт удерживает буфер и возвращает nullptr
    CHECK(returnedBuffer == nullptr);
    CHECK(window.hasRetainedBuffer());
    CHECK(window.presentCallCount() == 1);
}

} // namespace

int main() {
    try {
        testWindowSizeStruct();
        testWindowAdapterLifecycle();
        testWindowAdapterEventPolling();
        testPresentationPortPolymorphismAndSynchronousPresent();
        testPresentationPortMismatchedBufferRejection();
        testPresentationPortAsynchronousPresent();

        std::cout << "All WindowAdapter tests passed successfully.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "WindowAdapter test failure: " << e.what() << "\n";
        return 1;
    }
}
