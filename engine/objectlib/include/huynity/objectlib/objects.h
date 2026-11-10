#pragma once

namespace huynity::objectlib {

enum class ObjectType {
    Aabb,
    Box,
    Circle,
    Disk,
    Point,
    Line,
    Plane,
    Polygon,
    Polyline,
    Ray,
    Rectange,
    Segment,
    Sphere,
    Triangle,
    TriangleMesh,
};

template <typename Object>
concept SupportedObject = requires {
    typename Object::Params;
    Object::Dimension -> std::size_t;
    Object::Type -> ObjectType;
};
} // namespace huynity::objectlib
