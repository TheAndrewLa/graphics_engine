#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/ray_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class Ray {
public:
    [[nodiscard]] static CreateResult<Ray> create(RayParams<dimension> params) {
        if (!math::isFinite(params.origin) || !math::isFinite(params.direction)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        if (!detail::nonzero(params.direction)) {
            return std::unexpected(CreateError{CreateErrorCode::InvalidDirection});
        }
        return Ray(params.origin, params.direction);
    }
    [[nodiscard]] Point<dimension> origin() const noexcept { return m_origin; }
    [[nodiscard]] Vector<dimension> direction() const noexcept { return m_direction; }

private:
    Ray(Point<dimension> origin, Vector<dimension> direction) : m_origin{origin}, m_direction{direction} {}
    Point<dimension> m_origin;
    Vector<dimension> m_direction;
};

using Ray2 = Ray<2>;
using Ray3 = Ray<3>;
} // namespace huynity::objectlib
