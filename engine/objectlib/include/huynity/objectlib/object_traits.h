#pragma once

#include "huynity/objectlib/geometry.h"
#include "huynity/objectlib/object_spec.h"
#include "huynity/objectlib/object_type.h"

namespace huynity::objectlib {

template <ObjectType kind>
struct ObjectTraits;
template <>
struct ObjectTraits<ObjectType::Point2> {
    using Params = Point2Params;
    using Object = Point2;
};

template <>
struct ObjectTraits<ObjectType::Point3> {
    using Params = Point3Params;
    using Object = Point3;
};

template <>
struct ObjectTraits<ObjectType::Segment2> {
    using Params = Segment2Params;
    using Object = Segment2;
};

template <>
struct ObjectTraits<ObjectType::Segment3> {
    using Params = Segment3Params;
    using Object = Segment3;
};

template <>
struct ObjectTraits<ObjectType::Line2> {
    using Params = Line2Params;
    using Object = Line2;
};

template <>
struct ObjectTraits<ObjectType::Line3> {
    using Params = Line3Params;
    using Object = Line3;
};

template <>
struct ObjectTraits<ObjectType::Ray2> {
    using Params = Ray2Params;
    using Object = Ray2;
};

template <>
struct ObjectTraits<ObjectType::Ray3> {
    using Params = Ray3Params;
    using Object = Ray3;
};

template <>
struct ObjectTraits<ObjectType::Triangle2> {
    using Params = Triangle2Params;
    using Object = Triangle2;
};

template <>
struct ObjectTraits<ObjectType::Triangle3> {
    using Params = Triangle3Params;
    using Object = Triangle3;
};

template <>
struct ObjectTraits<ObjectType::Polyline2> {
    using Params = Polyline2Params;
    using Object = Polyline2;
};

template <>
struct ObjectTraits<ObjectType::Polyline3> {
    using Params = Polyline3Params;
    using Object = Polyline3;
};

template <>
struct ObjectTraits<ObjectType::Aabb2> {
    using Params = Aabb2Params;
    using Object = Aabb2;
};

template <>
struct ObjectTraits<ObjectType::Aabb3> {
    using Params = Aabb3Params;
    using Object = Aabb3;
};

template <>
struct ObjectTraits<ObjectType::TriangleMesh2> {
    using Params = TriangleMesh2Params;
    using Object = TriangleMesh2;
};

template <>
struct ObjectTraits<ObjectType::TriangleMesh3> {
    using Params = TriangleMesh3Params;
    using Object = TriangleMesh3;
};

template <>
struct ObjectTraits<ObjectType::Plane3> {
    using Params = Plane3Params;
    using Object = Plane3;
};

template <>
struct ObjectTraits<ObjectType::Polygon2> {
    using Params = Polygon2Params;
    using Object = Polygon2;
};

template <>
struct ObjectTraits<ObjectType::Rectangle2> {
    using Params = Rectangle2Params;
    using Object = Rectangle2;
};

template <>
struct ObjectTraits<ObjectType::Circle2> {
    using Params = Circle2Params;
    using Object = Circle2;
};

template <>
struct ObjectTraits<ObjectType::Disk2> {
    using Params = Disk2Params;
    using Object = Disk2;
};

template <>
struct ObjectTraits<ObjectType::Sphere3> {
    using Params = Sphere3Params;
    using Object = Sphere3;
};

template <>
struct ObjectTraits<ObjectType::Box3> {
    using Params = Box3Params;
    using Object = Box3;
};

template <ObjectType kind>
concept SupportedObject = requires {
    typename ObjectTraits<kind>::Params;
    typename ObjectTraits<kind>::Object;
};

template <ObjectType kind>
using ParamsFor = typename ObjectTraits<kind>::Params;
template <ObjectType kind>
using ObjectFor = typename ObjectTraits<kind>::Object;
} // namespace huynity::objectlib
