#pragma once

#include "huynity/adapters/window/window_adapter.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace huynity::adapters::window {

class NativeWindow final : public WindowAdapter {
public:
    NativeWindow(WindowSize size, std::string_view title);
    ~NativeWindow() override;

    NativeWindow(const NativeWindow&) = delete;
    NativeWindow& operator=(const NativeWindow&) = delete;
    NativeWindow(NativeWindow&&) = delete;
    NativeWindow& operator=(NativeWindow&&) = delete;

    [[nodiscard]] bool isOpen() const noexcept override;
    void close() noexcept override;
    [[nodiscard]] WindowSize size() const noexcept override;
    void setTitle(std::string_view title) override;
    void pollEvents(std::vector<InputEvent>& outEvents) override;
    [[nodiscard]] std::unique_ptr<image::ImageBuffer> present(std::unique_ptr<image::ImageBuffer> framebuffer) override;

private:
    [[nodiscard]] bool ensureTexture() noexcept;

    SDL_Window* m_window{};
    SDL_Renderer* m_renderer{};
    SDL_Texture* m_texture{};
    WindowSize m_size;
    bool m_isOpen{};
    bool m_videoInitialized{};
};

} // namespace huynity::adapters::window
