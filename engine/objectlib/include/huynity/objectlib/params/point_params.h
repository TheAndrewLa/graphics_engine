#pragma once

#include "huynity/objectlib/math_types.h"

#include <array>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct PointParams {
    std::array<double, dimension> coordinates{};
};

using Point2Params = PointParams<2>;
using Point3Params = PointParams<3>;
} // namespace huynity::objectlib
