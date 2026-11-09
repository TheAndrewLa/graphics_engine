#include "huynity/objectlib/operations/triangulate.h"

#include "huynity/objectlib/detail/validation.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>

namespace huynity::objectlib {

TessellationResult<2> triangulate(const Polygon2& polygon) {
    const auto points = polygon.contour();
    if (points.size() < 3) {
        return std::unexpected(GeometryError{GeometryErrorCode::InvalidGeometry});
    }
    if (points.size() > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(GeometryError{GeometryErrorCode::BudgetExceeded});
    }
    std::vector<std::uint32_t> ring(points.size());
    std::iota(ring.begin(), ring.end(), std::uint32_t{0});
    if (detail::signedAreaTwice(points) < 0) {
        std::ranges::reverse(ring);
    }

    // Straight boundary vertices are not needed as corners of output triangles.
    bool hasRemovedVertex = true;
    while (hasRemovedVertex && ring.size() > 3) {
        hasRemovedVertex = false;
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const auto a = ring[(i + ring.size() - 1) % ring.size()];
            const auto b = ring[i];
            const auto c = ring[(i + 1) % ring.size()];
            if (detail::orientation(points[a], points[b], points[c]) == 0) {
                ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
                hasRemovedVertex = true;
                break;
            }
        }
    }
    std::vector<TriangleIndices> triangles;
    triangles.reserve(ring.size() - 2);
    while (ring.size() > 3) {
        bool hasFoundEar = false;
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const auto a = ring[(i + ring.size() - 1) % ring.size()];
            const auto b = ring[i];
            const auto c = ring[(i + 1) % ring.size()];
            if (detail::orientation(points[a], points[b], points[c]) <= 0) {
                continue;
            }
            bool isOccupied = false;
            for (auto other : ring) {
                if (other == a || other == b || other == c) {
                    continue;
                }
                const auto& p = points[other];
                if (detail::orientation(points[a], points[b], p) >= 0 &&
                    detail::orientation(points[b], points[c], p) >= 0 &&
                    detail::orientation(points[c], points[a], p) >= 0) {
                    isOccupied = true;
                    break;
                }
            }
            if (isOccupied) {
                continue;
            }
            triangles.push_back({{a, b, c}});
            ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
            hasFoundEar = true;
            break;
        }
        if (!hasFoundEar) {
            return std::unexpected(GeometryError{GeometryErrorCode::NumericalFailure});
        }
    }
    if (detail::orientation(points[ring[0]], points[ring[1]], points[ring[2]]) <= 0) {
        return std::unexpected(GeometryError{GeometryErrorCode::NumericalFailure});
    }
    triangles.push_back({{ring[0], ring[1], ring[2]}});
    auto mesh = TriangleMesh2::create({std::vector<Point2>{points.begin(), points.end()}, std::move(triangles)});
    if (!mesh) {
        return std::unexpected(GeometryError{GeometryErrorCode::InvalidGeometry});
    }
    return std::move(*mesh);
}

} // namespace huynity::objectlib
