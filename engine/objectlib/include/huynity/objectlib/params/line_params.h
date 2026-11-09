#pragma once

#include "huynity/objectlib/math_types.h"

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct LineParams {
    Point<dimension> origin;
    Vector<dimension> direction;
};

using Line2Params = LineParams<2>;
using Line3Params = LineParams<3>;
} // namespace huynity::objectlib
