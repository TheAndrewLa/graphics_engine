#pragma once

#include "huynity/objectlib/params/aabb_params.h"
#include "huynity/objectlib/params/box3_params.h"
#include "huynity/objectlib/params/circle2_params.h"
#include "huynity/objectlib/params/disk2_params.h"
#include "huynity/objectlib/params/line_params.h"
#include "huynity/objectlib/params/plane3_params.h"
#include "huynity/objectlib/params/point_params.h"
#include "huynity/objectlib/params/polygon2_params.h"
#include "huynity/objectlib/params/polyline_params.h"
#include "huynity/objectlib/params/ray_params.h"
#include "huynity/objectlib/params/rectangle2_params.h"
#include "huynity/objectlib/params/segment_params.h"
#include "huynity/objectlib/params/sphere3_params.h"
#include "huynity/objectlib/params/triangle_mesh_params.h"
#include "huynity/objectlib/params/triangle_params.h"

#include <variant>

namespace huynity::objectlib {

using ObjectSpec =
    std::variant<Point2Params, Point3Params, Segment2Params, Segment3Params, Line2Params, Line3Params, Ray2Params,
                 Ray3Params, Triangle2Params, Triangle3Params, Polyline2Params, Polyline3Params, Aabb2Params,
                 Aabb3Params, TriangleMesh2Params, TriangleMesh3Params, Plane3Params, Polygon2Params, Rectangle2Params,
                 Circle2Params, Disk2Params, Sphere3Params, Box3Params>;
} // namespace huynity::objectlib
