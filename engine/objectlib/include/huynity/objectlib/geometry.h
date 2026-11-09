#pragma once

#include "huynity/objectlib/collections/polygon2.h"
#include "huynity/objectlib/collections/polyline.h"
#include "huynity/objectlib/collections/triangle_mesh.h"
#include "huynity/objectlib/math_types.h"
#include "huynity/objectlib/params/point_params.h"
#include "huynity/objectlib/primitives/aabb.h"
#include "huynity/objectlib/primitives/line.h"
#include "huynity/objectlib/primitives/plane3.h"
#include "huynity/objectlib/primitives/ray.h"
#include "huynity/objectlib/primitives/segment.h"
#include "huynity/objectlib/primitives/triangle.h"
#include "huynity/objectlib/shapes/box3.h"
#include "huynity/objectlib/shapes/circle2.h"
#include "huynity/objectlib/shapes/disk2.h"
#include "huynity/objectlib/shapes/rectangle2.h"
#include "huynity/objectlib/shapes/sphere3.h"

#include <variant>

namespace huynity::objectlib {

using Geometry = std::variant<Point2, Point3, Segment2, Segment3, Line2, Line3, Ray2, Ray3, Triangle2, Triangle3,
                              Polyline2, Polyline3, Aabb2, Aabb3, TriangleMesh2, TriangleMesh3, Plane3, Polygon2,
                              Rectangle2, Circle2, Disk2, Sphere3, Box3>;
} // namespace huynity::objectlib
