#pragma once

#include "huynity/objectlib/collections/polygon2.h"
#include "huynity/objectlib/collections/triangle_mesh.h"

namespace huynity::objectlib {

template <std::size_t dimension>
using TessellationResult = GeometryResult<TriangleMesh<dimension>>;

[[nodiscard]] TessellationResult<2> triangulate(const Polygon2& polygon);

} // namespace huynity::objectlib
