#pragma once

#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/segment_params.h"

#include <cmath>
#include <span>
#include <utility>

namespace huynity::objectlib {

template <std::size_t dimension>
    requires math::Dimension<dimension>
class Segment {
public:
    [[nodiscard]] static CreateResult<Segment> create(SegmentParams<dimension> params) {
        if (!math::isFinite(params.a) || !math::isFinite(params.b)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        return Segment(params.a, params.b);
    }
    [[nodiscard]] Point<dimension> a() const noexcept { return m_firstPoint; }
    [[nodiscard]] Point<dimension> b() const noexcept { return m_secondPoint; }

private:
    Segment(Point<dimension> a, Point<dimension> b) : m_firstPoint{a}, m_secondPoint{b} {}
    Point<dimension> m_firstPoint;
    Point<dimension> m_secondPoint;
};

using Segment2 = Segment<2>;
using Segment3 = Segment<3>;
} // namespace huynity::objectlib
