#pragma once

#include "huynity/objectlib/math_types.h"
#include "huynity/objectlib/primitives/triangle_indices.h"

#include <vector>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
struct TriangleMeshParams {
    std::vector<Point<dimension>> positions;
    std::vector<TriangleIndices> triangles;
};

using TriangleMesh2Params = TriangleMeshParams<2>;
using TriangleMesh3Params = TriangleMeshParams<3>;
} // namespace huynity::objectlib
