#pragma once

#include "huynity/objectlib/math_types.h"

namespace huynity::objectlib {

struct Plane3Params {
    Vector3 normal;
    double offset{};
};
} // namespace huynity::objectlib
