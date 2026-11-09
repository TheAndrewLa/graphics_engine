#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/aabb_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class Aabb {
public:
    [[nodiscard]] static CreateResult<Aabb> create(AabbParams<dimension> params) {
        if (!math::isFinite(params.min) || !math::isFinite(params.max)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        for (std::size_t i = 0; i < dimension; ++i) {
            if (params.min[i] > params.max[i]) {
                return std::unexpected(CreateError{CreateErrorCode::InvalidBounds});
            }
        }
        return Aabb(params.min, params.max);
    }
    [[nodiscard]] Point<dimension> min() const noexcept { return m_min; }
    [[nodiscard]] Point<dimension> max() const noexcept { return m_max; }

private:
    Aabb(Point<dimension> min, Point<dimension> max) : m_min{min}, m_max{max} {}
    Point<dimension> m_min;
    Point<dimension> m_max;
};

using Aabb2 = Aabb<2>;
using Aabb3 = Aabb<3>;
} // namespace huynity::objectlib
