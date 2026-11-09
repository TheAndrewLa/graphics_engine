#pragma once

#include "huynity/image/image_buffer.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace huynity::runtime {

/// @brief Небольшой пул framebuffer главного цикла с уникальным владением.
///
/// Пул хранит буферы только текущего размера поверхности; буфер прежнего размера при
/// возврате уничтожается. Число хранимых буферов ограничено максимумом одновременно
/// живых (активный render и presentation); сверх лимита буферы уничтожаются при
/// возврате, а не копятся в очередь.
class FramebufferPool {
public:
    explicit FramebufferPool(std::size_t maxPooled = 2) : m_maxPooled{maxPooled} {}

    /// @brief Задаёт размер поверхности; хранимые буферы прежнего размера уничтожаются.
    void resize(std::size_t width, std::size_t height) noexcept {
        if (width != m_width || height != m_height) {
            m_width = width;
            m_height = height;
            m_pooled.clear();
        }
    }

    /// @return буфер текущего размера из пула или новый; nullptr при нулевой площади или переполнении размера.
    [[nodiscard]] std::unique_ptr<image::ImageBuffer> acquire() {
        if (!m_pooled.empty()) {
            auto framebuffer = std::move(m_pooled.back());
            m_pooled.pop_back();
            return framebuffer;
        }
        auto created = image::ImageBuffer::create({m_width, m_height});
        if (!created) {
            return nullptr;
        }
        ++m_created;
        return std::make_unique<image::ImageBuffer>(std::move(*created));
    }

    /// @brief Возвращает буфер в пул до лимита; буфер иного размера уничтожается.
    void release(std::unique_ptr<image::ImageBuffer> framebuffer) {
        if (!framebuffer) {
            return;
        }
        const bool matchesSize = framebuffer->width() == m_width && framebuffer->height() == m_height;
        if (matchesSize && m_pooled.size() < m_maxPooled) {
            m_pooled.push_back(std::move(framebuffer));
        }
    }

    [[nodiscard]] std::size_t pooledCount() const noexcept { return m_pooled.size(); }
    [[nodiscard]] std::size_t createdCount() const noexcept { return m_created; }
    [[nodiscard]] std::size_t maxPooled() const noexcept { return m_maxPooled; }

private:
    std::vector<std::unique_ptr<image::ImageBuffer>> m_pooled;
    std::size_t m_width{};
    std::size_t m_height{};
    std::size_t m_maxPooled;
    std::size_t m_created{};
};

} // namespace huynity::runtime
