#include "huynity/image/image_buffer.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace image = huynity::image;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

static_assert(!std::is_copy_constructible_v<image::ImageBuffer>);
static_assert(!std::is_copy_assignable_v<image::ImageBuffer>);
static_assert(std::is_nothrow_move_constructible_v<image::ImageBuffer>);
static_assert(std::is_nothrow_move_assignable_v<image::ImageBuffer>);

image::ImageBuffer take(image::ImageBuffer::Result result) {
    CHECK(result.has_value());
    return std::move(*result);
}

void checkError(image::ImageBuffer::Result result, image::ImageError expected) {
    CHECK(!result.has_value());
    CHECK(result.error() == expected);
}

void layoutAndBoundaries() {
    auto buffer = take(image::ImageBuffer::create({.width = 2, .height = 3, .stride = 12}));
    CHECK(buffer.width() == 2);
    CHECK(buffer.height() == 3);
    CHECK(buffer.stride() == 12);
    CHECK(buffer.format() == image::PixelFormat::Rgba8Unorm);
    CHECK(buffer.sizeBytes() == 36);
    CHECK(buffer.bytes().size() == 36);

    for (std::size_t y = 0; y < buffer.height(); ++y) {
        auto row = buffer.row(y);
        CHECK(row.size() == 12);
        CHECK(row.data() == buffer.bytes().data() + y * buffer.stride());
        std::fill(row.begin(), row.end(), std::byte{static_cast<unsigned char>(y + 1)});
    }
    CHECK(buffer.bytes().front() == std::byte{1});
    CHECK(buffer.bytes().back() == std::byte{3});

    const auto& constBuffer = buffer;
    CHECK(constBuffer.row(2).data() == constBuffer.bytes().data() + 24);
    auto packed = take(image::ImageBuffer::create({.width = 3, .height = 2}));
    CHECK(packed.stride() == 12);
    CHECK(packed.sizeBytes() == 24);
}

void ownershipTransfer() {
    auto source = take(image::ImageBuffer::create({.width = 1, .height = 1}));
    const auto* data = source.bytes().data();
    auto destination = std::move(source);
    CHECK(destination.bytes().data() == data);
    CHECK(destination.sizeBytes() == 4);

    image::ImageBuffer assigned = take(image::ImageBuffer::create({.width = 2, .height = 1}));
    assigned = std::move(destination);
    CHECK(assigned.bytes().data() == data);
    CHECK(assigned.width() == 1);
}

void invalidBuffers() {
    checkError(image::ImageBuffer::create({.height = 1}), image::ImageError::Empty);
    checkError(image::ImageBuffer::create({.width = 1}), image::ImageError::Empty);
    checkError(image::ImageBuffer::create({.width = 2, .height = 1, .stride = 7}), image::ImageError::InvalidStride);
    checkError(image::ImageBuffer::create({.width = std::numeric_limits<std::size_t>::max(), .height = 1}),
               image::ImageError::SizeOverflow);
    checkError(image::ImageBuffer::create({.width = 1, .height = std::numeric_limits<std::size_t>::max()}),
               image::ImageError::SizeOverflow);
}

int main() {
    try {
        layoutAndBoundaries();
        ownershipTransfer();
        invalidBuffers();
        std::cout << "buffer: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
