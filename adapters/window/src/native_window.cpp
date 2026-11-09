#include "native_window.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "sdl_event_translation.h"

namespace huynity::adapters::window {
namespace {

[[nodiscard]] int sdlDimension(std::uint32_t value) {
    if (value == 0 || value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("Window dimensions must be positive and fit SDL integer range");
    }
    return static_cast<int>(value);
}

[[nodiscard]] std::runtime_error sdlError(std::string_view operation) {
    return std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}

[[nodiscard]] SDL_WindowID eventWindowId(const SDL_Event& event) noexcept {
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        return event.window.windowID;
    }
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return event.key.windowID;
    case SDL_EVENT_MOUSE_MOTION:
        return event.motion.windowID;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return event.button.windowID;
    case SDL_EVENT_MOUSE_WHEEL:
        return event.wheel.windowID;
    default:
        return 0;
    }
}

[[nodiscard]] bool isForWindow(const SDL_Event& event, SDL_Window* window) noexcept {
    if (event.type == SDL_EVENT_QUIT) {
        return true;
    }
    const auto windowId = eventWindowId(event);
    return windowId == 0 || windowId == SDL_GetWindowID(window);
}

[[nodiscard]] PixelScale pixelScale(SDL_Window* window, WindowSize pixelSize) noexcept {
    int logicalWidth = 0;
    int logicalHeight = 0;
    if (!SDL_GetWindowSize(window, &logicalWidth, &logicalHeight) || logicalWidth <= 0 || logicalHeight <= 0 ||
        pixelSize.width == 0 || pixelSize.height == 0) {
        return {};
    }
    return {
        .x = static_cast<double>(pixelSize.width) / static_cast<double>(logicalWidth),
        .y = static_cast<double>(pixelSize.height) / static_cast<double>(logicalHeight),
    };
}

} // namespace

NativeWindow::NativeWindow(WindowSize size, std::string_view title) : m_size{size}, m_isOpen{true} {
    try {
        const int width = sdlDimension(size.width);
        const int height = sdlDimension(size.height);
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            throw sdlError("SDL_InitSubSystem(SDL_INIT_VIDEO) failed");
        }
        m_videoInitialized = true;

        m_window = SDL_CreateWindow(std::string(title).c_str(), width, height,
                                    SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        if (m_window == nullptr) {
            throw sdlError("SDL_CreateWindow failed");
        }
        (void)SDL_SyncWindow(m_window);
        m_renderer = SDL_CreateRenderer(m_window, nullptr);
        if (m_renderer == nullptr) {
            throw sdlError("SDL_CreateRenderer failed");
        }

        int pixelWidth = 0;
        int pixelHeight = 0;
        if (!SDL_GetWindowSizeInPixels(m_window, &pixelWidth, &pixelHeight)) {
            throw sdlError("SDL_GetWindowSizeInPixels failed");
        }
        m_size = WindowSize{
            .width = static_cast<std::uint32_t>(pixelWidth),
            .height = static_cast<std::uint32_t>(pixelHeight),
        };
    } catch (...) {
        close();
        throw;
    }
}

NativeWindow::~NativeWindow() {
    close();
}

bool NativeWindow::isOpen() const noexcept {
    return m_isOpen;
}

void NativeWindow::close() noexcept {
    m_isOpen = false;
    if (m_texture != nullptr) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
    if (m_renderer != nullptr) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_videoInitialized) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        m_videoInitialized = false;
    }
}

WindowSize NativeWindow::size() const noexcept {
    return m_size;
}

void NativeWindow::setTitle(std::string_view title) {
    if (!m_isOpen || m_window == nullptr) {
        return;
    }
    const std::string terminatedTitle{title};
    if (!SDL_SetWindowTitle(m_window, terminatedTitle.c_str())) {
        std::cerr << "SDL_SetWindowTitle failed: " << SDL_GetError() << '\n';
    }
}

void NativeWindow::pollEvents(std::vector<InputEvent>& outEvents) {
    if (!m_isOpen || m_window == nullptr) {
        return;
    }

    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (!isForWindow(event, m_window)) {
            continue;
        }
        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            m_size = WindowSize{
                .width = static_cast<std::uint32_t>(std::max<Sint32>(event.window.data1, 0)),
                .height = static_cast<std::uint32_t>(std::max<Sint32>(event.window.data2, 0)),
            };
        }

        auto translated = translateSdlEvent(event, pixelScale(m_window, m_size));
        if (translated) {
            outEvents.push_back(*translated);
        }
    }
}

bool NativeWindow::ensureTexture() noexcept {
    if (m_texture != nullptr) {
        float textureWidth = 0.0F;
        float textureHeight = 0.0F;
        if (SDL_GetTextureSize(m_texture, &textureWidth, &textureHeight) &&
            static_cast<std::uint32_t>(textureWidth) == m_size.width &&
            static_cast<std::uint32_t>(textureHeight) == m_size.height) {
            return true;
        }
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_size.width == 0 || m_size.height == 0 ||
        m_size.width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        m_size.height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                  static_cast<int>(m_size.width), static_cast<int>(m_size.height));
    if (m_texture == nullptr) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << '\n';
        return false;
    }
    return true;
}

std::unique_ptr<image::ImageBuffer> NativeWindow::present(std::unique_ptr<image::ImageBuffer> framebuffer) {
    if (!framebuffer) {
        return nullptr;
    }
    if (!m_isOpen || m_renderer == nullptr || framebuffer->format() != image::PixelFormat::Rgba8Unorm ||
        framebuffer->width() != m_size.width || framebuffer->height() != m_size.height) {
        return framebuffer;
    }
    if (framebuffer->stride() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "ImageBuffer stride exceeds SDL pitch range\n";
        return framebuffer;
    }
    if (!ensureTexture()) {
        return framebuffer;
    }
    if (!SDL_UpdateTexture(m_texture, nullptr, framebuffer->bytes().data(), static_cast<int>(framebuffer->stride()))) {
        std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << '\n';
        return framebuffer;
    }
    if (!SDL_RenderClear(m_renderer)) {
        std::cerr << "SDL_RenderClear failed: " << SDL_GetError() << '\n';
        return framebuffer;
    }
    if (!SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr)) {
        std::cerr << "SDL_RenderTexture failed: " << SDL_GetError() << '\n';
        return framebuffer;
    }
    if (!SDL_RenderPresent(m_renderer)) {
        std::cerr << "SDL_RenderPresent failed: " << SDL_GetError() << '\n';
    }
    return framebuffer;
}

} // namespace huynity::adapters::window
