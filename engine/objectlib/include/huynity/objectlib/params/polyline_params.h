#pragma once

#include "huynity/objectlib/math_types.h"

#include <vector>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct PolylineParams {
    std::vector<Point<dimension>> vertices;
    bool closed{};
};

using Polyline2Params = PolylineParams<2>;
using Polyline3Params = PolylineParams<3>;
} // namespace huynity::objectlib
