#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/triangle_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class Triangle {
public:
    [[nodiscard]] static CreateResult<Triangle> create(TriangleParams<dimension> params) {
        if (!detail::finitePoints<dimension>(params.vertices)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        bool isDegenerate;
        if constexpr (dimension == 2) {
            const auto area = detail::orientation(params.vertices[0], params.vertices[1], params.vertices[2]);
            if (!std::isfinite(area)) {
                return std::unexpected(CreateError{CreateErrorCode::NumericalFailure});
            }
            isDegenerate = area == 0;
        } else {
            std::array<long double, 3> a{};
            std::array<long double, 3> b{};
            std::array<long double, 3> normal{};
            for (std::size_t i = 0; i < 3; ++i) {
                a[i] = static_cast<long double>(params.vertices[1][i]) - params.vertices[0][i];
                b[i] = static_cast<long double>(params.vertices[2][i]) - params.vertices[0][i];
            }
            for (std::size_t i = 0; i < 3; ++i) {
                normal[i] = a[(i + 1) % 3] * b[(i + 2) % 3] - a[(i + 2) % 3] * b[(i + 1) % 3];
                if (!std::isfinite(normal[i])) {
                    return std::unexpected(CreateError{CreateErrorCode::NumericalFailure});
                }
            }
            isDegenerate = normal[0] == 0 && normal[1] == 0 && normal[2] == 0;
        }
        if (isDegenerate) {
            return std::unexpected(CreateError{CreateErrorCode::DegenerateGeometry});
        }
        return Triangle(params.vertices);
    }
    [[nodiscard]] std::span<const Point<dimension>, 3> vertices() const noexcept { return m_vertices; }

private:
    explicit Triangle(std::array<Point<dimension>, 3> vertices) : m_vertices{vertices} {}
    std::array<Point<dimension>, 3> m_vertices;
};

using Triangle2 = Triangle<2>;
using Triangle3 = Triangle<3>;
} // namespace huynity::objectlib
