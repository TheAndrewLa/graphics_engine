#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <vector>

namespace huynity::image {

enum class PixelFormat {
    Rgba8Unorm,
};

enum class ImageError {
    Empty,
    InvalidStride,
    SizeOverflow,
};

struct ImageBufferDesc {
    std::size_t width{};
    std::size_t height{};
    PixelFormat format{PixelFormat::Rgba8Unorm};
    std::size_t stride{};
};

class ImageBuffer {
public:
    using Result = std::expected<ImageBuffer, ImageError>;

    [[nodiscard]] static Result create(const ImageBufferDesc& desc) {
        constexpr std::size_t bytesPerPixel = 4;
        if (desc.width == 0 || desc.height == 0) {
            return std::unexpected(ImageError::Empty);
        }
        if (desc.width > std::numeric_limits<std::size_t>::max() / bytesPerPixel) {
            return std::unexpected(ImageError::SizeOverflow);
        }
        const std::size_t packedStride = desc.width * bytesPerPixel;
        const std::size_t stride = desc.stride == 0 ? packedStride : desc.stride;
        if (stride < packedStride) {
            return std::unexpected(ImageError::InvalidStride);
        }
        if (desc.height > std::numeric_limits<std::size_t>::max() / stride) {
            return std::unexpected(ImageError::SizeOverflow);
        }
        return ImageBuffer{desc.width, desc.height, desc.format, stride};
    }

    ImageBuffer(const ImageBuffer&) = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;
    ImageBuffer(ImageBuffer&&) noexcept = default;
    ImageBuffer& operator=(ImageBuffer&&) noexcept = default;

    [[nodiscard]] std::size_t width() const noexcept { return m_width; }
    [[nodiscard]] std::size_t height() const noexcept { return m_height; }
    [[nodiscard]] std::size_t stride() const noexcept { return m_stride; }
    [[nodiscard]] PixelFormat format() const noexcept { return m_format; }
    [[nodiscard]] std::size_t sizeBytes() const noexcept { return m_bytes.size(); }
    [[nodiscard]] std::span<std::byte> bytes() noexcept { return m_bytes; }
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return m_bytes; }

    [[nodiscard]] std::span<std::byte> row(std::size_t y) noexcept {
        return std::span{m_bytes}.subspan(y * m_stride, m_stride);
    }
    [[nodiscard]] std::span<const std::byte> row(std::size_t y) const noexcept {
        return std::span{m_bytes}.subspan(y * m_stride, m_stride);
    }

private:
    ImageBuffer(std::size_t width, std::size_t height, PixelFormat format, std::size_t stride)
        : m_width{width}, m_height{height}, m_format{format}, m_stride{stride}, m_bytes(height * stride) {}

    std::size_t m_width{};
    std::size_t m_height{};
    PixelFormat m_format{PixelFormat::Rgba8Unorm};
    std::size_t m_stride{};
    std::vector<std::byte> m_bytes;
};

} // namespace huynity::image
