#pragma once

#include "huynity/objectlib/math_types.h"

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct AabbParams {
    Point<dimension> min;
    Point<dimension> max;
};

using Aabb2Params = AabbParams<2>;
using Aabb3Params = AabbParams<3>;
} // namespace huynity::objectlib
