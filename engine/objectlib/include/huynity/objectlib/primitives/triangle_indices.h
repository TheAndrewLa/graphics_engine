#pragma once

#include <array>
#include <cstdint>

namespace huynity::objectlib {

struct TriangleIndices {
    std::array<std::uint32_t, 3> indices{};
    friend bool operator==(const TriangleIndices&, const TriangleIndices&) = default;
};
} // namespace huynity::objectlib
