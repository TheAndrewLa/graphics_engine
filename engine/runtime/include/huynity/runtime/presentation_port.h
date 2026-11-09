#pragma once

#include "huynity/image/image_buffer.h"

#include <memory>

namespace huynity::runtime {

/// @brief Порт показа готового framebuffer главного цикла.
class PresentationPort {
public:
    virtual ~PresentationPort() = default;

    /// @brief Показывает framebuffer.
    /// @return буфер, если показ синхронный: использование полностью завершено к возврату
    ///         из вызова, и Runtime может вернуть буфер в пул; nullptr, если показ
    ///         асинхронный: Presentation удерживает буфер до отдельного подтверждения
    ///         через FramePipeline::releaseFromPresentation.
    [[nodiscard]] virtual std::unique_ptr<image::ImageBuffer>
    present(std::unique_ptr<image::ImageBuffer> framebuffer) = 0;
};

} // namespace huynity::runtime
