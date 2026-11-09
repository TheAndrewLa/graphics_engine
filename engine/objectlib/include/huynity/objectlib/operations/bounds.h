#pragma once

#include "huynity/objectlib/geometry.h"

#include <algorithm>
#include <array>
#include <span>
#include <variant>

namespace huynity::objectlib {

template <std::size_t dimension>
using BoundsResult = GeometryResult<Aabb<dimension>>;

using BoundsGeometry = std::variant<Aabb2, Aabb3>;
using RuntimeBoundsResult = GeometryResult<BoundsGeometry>;

[[nodiscard]] RuntimeBoundsResult bounds(const Geometry& geometry);

namespace detail {

template <std::size_t dimension>
BoundsResult<dimension> pointBounds(std::span<const Point<dimension>> points) {
    if (points.empty()) {
        return std::unexpected(GeometryError{GeometryErrorCode::EmptyGeometry});
    }
    if (!finitePoints<dimension>(points)) {
        return std::unexpected(GeometryError{GeometryErrorCode::InvalidGeometry});
    }
    Point<dimension> min = points.front();
    Point<dimension> max = min;
    for (const auto& point : points) {
        for (std::size_t i = 0; i < dimension; ++i) {
            min[i] = std::min(min[i], point[i]);
            max[i] = std::max(max[i], point[i]);
        }
    }
    auto result = Aabb<dimension>::create({min, max});
    if (!result) {
        return std::unexpected(GeometryError{GeometryErrorCode::NumericalFailure});
    }
    return std::move(*result);
}

template <std::size_t dimension>
BoundsResult<dimension> symmetricBounds(const Vector<dimension>& halfSize) {
    std::array<Point<dimension>, 2> corners{};
    for (std::size_t i = 0; i < dimension; ++i) {
        corners[0][i] = -halfSize[i];
        corners[1][i] = halfSize[i];
    }
    return pointBounds<dimension>(corners);
}

template <std::size_t dimension>
BoundsResult<dimension> sizeBounds(const Vector<dimension>& size) {
    const auto halfSize = size * 0.5;
    for (double value : halfSize.components) {
        if (value == 0) {
            return std::unexpected(GeometryError{GeometryErrorCode::NumericalFailure});
        }
    }
    return symmetricBounds(halfSize);
}
} // namespace detail

template <std::size_t dimension>
[[nodiscard]] BoundsResult<dimension> bounds(const Point<dimension>& point) {
    return detail::pointBounds<dimension>(std::span<const Point<dimension>>{&point, 1});
}

template <std::size_t dimension>
[[nodiscard]] BoundsResult<dimension> bounds(const Segment<dimension>& segment) {
    const std::array points{segment.a(), segment.b()};
    return detail::pointBounds<dimension>(points);
}

template <std::size_t dimension>
[[nodiscard]] BoundsResult<dimension> bounds(const Triangle<dimension>& triangle) {
    return detail::pointBounds<dimension>(triangle.vertices());
}

template <std::size_t dimension>
[[nodiscard]] BoundsResult<dimension> bounds(const Polyline<dimension>& polyline) {
    return detail::pointBounds<dimension>(polyline.vertices());
}

template <std::size_t dimension>
[[nodiscard]] BoundsResult<dimension> bounds(const TriangleMesh<dimension>& mesh) {
    return detail::pointBounds<dimension>(mesh.positions());
}

template <std::size_t dimension>
[[nodiscard]] BoundsResult<dimension> bounds(const Aabb<dimension>& box) {
    return box;
}

[[nodiscard]] inline BoundsResult<2> bounds(const Polygon2& polygon) {
    return detail::pointBounds<2>(polygon.contour());
}
[[nodiscard]] inline BoundsResult<2> bounds(const Rectangle2& rectangle) {
    return detail::sizeBounds(rectangle.size());
}
[[nodiscard]] inline BoundsResult<3> bounds(const Box3& box) {
    return detail::sizeBounds(box.size());
}
[[nodiscard]] inline BoundsResult<2> bounds(const Circle2& circle) {
    return detail::symmetricBounds(Vector2{{circle.radius(), circle.radius()}});
}
[[nodiscard]] inline BoundsResult<2> bounds(const Disk2& disk) {
    return detail::symmetricBounds(Vector2{{disk.radius(), disk.radius()}});
}
[[nodiscard]] inline BoundsResult<3> bounds(const Sphere3& sphere) {
    return detail::symmetricBounds(Vector3{{sphere.radius(), sphere.radius(), sphere.radius()}});
}

} // namespace huynity::objectlib
