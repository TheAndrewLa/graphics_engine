#pragma once

#include "huynity/objectlib/math_types.h"

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct RayParams {
    Point<dimension> origin;
    Vector<dimension> direction;
};

using Ray2Params = RayParams<2>;
using Ray3Params = RayParams<3>;
} // namespace huynity::objectlib
