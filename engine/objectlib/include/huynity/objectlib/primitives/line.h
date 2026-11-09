#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/line_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class Line {
public:
    [[nodiscard]] static CreateResult<Line> create(LineParams<dimension> params) {
        if (!math::isFinite(params.origin) || !math::isFinite(params.direction)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        if (!detail::nonzero(params.direction)) {
            return std::unexpected(CreateError{CreateErrorCode::InvalidDirection});
        }
        return Line(params.origin, params.direction);
    }
    [[nodiscard]] Point<dimension> origin() const noexcept { return m_origin; }
    [[nodiscard]] Vector<dimension> direction() const noexcept { return m_direction; }

private:
    Line(Point<dimension> origin, Vector<dimension> direction) : m_origin{origin}, m_direction{direction} {}
    Point<dimension> m_origin;
    Vector<dimension> m_direction;
};

using Line2 = Line<2>;
using Line3 = Line<3>;
} // namespace huynity::objectlib
