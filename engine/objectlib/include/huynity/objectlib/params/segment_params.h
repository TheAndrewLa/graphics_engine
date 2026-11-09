#pragma once

#include "huynity/objectlib/math_types.h"

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct SegmentParams {
    Point<dimension> a;
    Point<dimension> b;
};

using Segment2Params = SegmentParams<2>;
using Segment3Params = SegmentParams<3>;
} // namespace huynity::objectlib
