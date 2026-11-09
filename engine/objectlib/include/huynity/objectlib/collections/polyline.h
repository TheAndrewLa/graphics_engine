#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/polyline_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class Polyline {
public:
    [[nodiscard]] static CreateResult<Polyline> create(PolylineParams<dimension> params) {
        if (!detail::finitePoints<dimension>(params.vertices)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        if (params.vertices.size() < (params.closed ? 3U : 2U)) {
            return std::unexpected(CreateError{CreateErrorCode::TooFewVertices});
        }
        return Polyline(std::move(params.vertices), params.closed);
    }
    [[nodiscard]] std::span<const Point<dimension>> vertices() const noexcept { return m_vertices; }
    [[nodiscard]] bool closed() const noexcept { return m_isClosed; }

private:
    Polyline(std::vector<Point<dimension>> vertices, bool closed)
        : m_vertices{std::move(vertices)}, m_isClosed{closed} {}
    std::vector<Point<dimension>> m_vertices;
    bool m_isClosed;
};

using Polyline2 = Polyline<2>;
using Polyline3 = Polyline<3>;
} // namespace huynity::objectlib
