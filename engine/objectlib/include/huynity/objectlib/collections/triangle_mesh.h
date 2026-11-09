#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/triangle_mesh_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class TriangleMesh {
public:
    [[nodiscard]] static CreateResult<TriangleMesh> create(TriangleMeshParams<dimension> params) {
        if (!detail::finitePoints<dimension>(params.positions)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        for (const auto& triangle : params.triangles) {
            for (auto index : triangle.indices) {
                if (index >= params.positions.size()) {
                    return std::unexpected(CreateError{CreateErrorCode::InvalidIndex});
                }
            }
        }
        return TriangleMesh(std::move(params.positions), std::move(params.triangles));
    }
    [[nodiscard]] std::span<const Point<dimension>> positions() const noexcept { return m_positions; }
    [[nodiscard]] std::span<const TriangleIndices> triangles() const noexcept { return m_triangles; }

private:
    TriangleMesh(std::vector<Point<dimension>> positions, std::vector<TriangleIndices> triangles)
        : m_positions{std::move(positions)}, m_triangles{std::move(triangles)} {}
    std::vector<Point<dimension>> m_positions;
    std::vector<TriangleIndices> m_triangles;
};

using TriangleMesh2 = TriangleMesh<2>;
using TriangleMesh3 = TriangleMesh<3>;
} // namespace huynity::objectlib
