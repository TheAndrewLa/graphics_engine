#pragma once

#include "huynity/objectlib/math_types.h"

#include <vector>

namespace huynity::objectlib {

struct Polygon2Params {
    std::vector<Point2> contour;
};
} // namespace huynity::objectlib
