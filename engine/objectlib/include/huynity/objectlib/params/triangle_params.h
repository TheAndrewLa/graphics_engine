#pragma once

#include "huynity/objectlib/math_types.h"

#include <array>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct TriangleParams {
    std::array<Point<dimension>, 3> vertices;
};

using Triangle2Params = TriangleParams<2>;
using Triangle3Params = TriangleParams<3>;
} // namespace huynity::objectlib
