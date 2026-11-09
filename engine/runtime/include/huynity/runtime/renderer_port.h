#pragma once

#include "huynity/image/image_buffer.h"
#include "huynity/render_contracts/render_snapshot.h"

#include <cstdint>
#include <memory>

namespace huynity::runtime {

struct RenderFrame {
    std::uint64_t frameId{};
    std::unique_ptr<render_contracts::RenderSnapshot> snapshot;
    std::unique_ptr<image::ImageBuffer> framebuffer; // уникальное владение; возвращается в FrameReady/FrameFailed
};

enum class SubmitResult {
    Accepted,
    Rejected,
};

class RendererPort {
public:
    virtual ~RendererPort() = default;
    [[nodiscard]] virtual SubmitResult trySubmit(RenderFrame& frame) = 0;
};

} // namespace huynity::runtime
