#include "huynity/objectlib/collections/polygon2.h"

#include "huynity/objectlib/detail/validation.h"

#include <cmath>
#include <utility>

namespace huynity::objectlib {

CreateResult<Polygon2> Polygon2::create(Polygon2Params params) {
    auto& points = params.contour;
    if (!detail::finitePoints<2>(points)) {
        return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
    }
    if (points.size() > 1 && points.front() == points.back()) {
        points.pop_back();
    }
    if (points.size() < 3) {
        return std::unexpected(CreateError{CreateErrorCode::TooFewVertices});
    }
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& a = points[i];
        const auto& b = points[(i + 1) % points.size()];
        const auto& c = points[(i + 2) % points.size()];
        if (a == b) {
            return std::unexpected(CreateError{CreateErrorCode::DegenerateGeometry});
        }
        const auto turn = detail::orientation(a, b, c);
        if (!std::isfinite(turn)) {
            return std::unexpected(CreateError{CreateErrorCode::NumericalFailure});
        }
        if (turn == 0) {
            const long double forward =
                (static_cast<long double>(b[0]) - a[0]) * (static_cast<long double>(c[0]) - b[0]) +
                (static_cast<long double>(b[1]) - a[1]) * (static_cast<long double>(c[1]) - b[1]);
            if (forward < 0) {
                return std::unexpected(CreateError{CreateErrorCode::SelfIntersection});
            }
        }
        for (std::size_t j = i + 1; j < points.size(); ++j) {
            if (j == i + 1 || (i == 0 && j + 1 == points.size())) {
                continue;
            }
            if (detail::segmentsIntersect(a, b, points[j], points[(j + 1) % points.size()])) {
                return std::unexpected(CreateError{CreateErrorCode::SelfIntersection});
            }
        }
    }
    const auto area = detail::signedAreaTwice(points);
    if (!std::isfinite(area)) {
        return std::unexpected(CreateError{CreateErrorCode::NumericalFailure});
    }
    if (area == 0) {
        return std::unexpected(CreateError{CreateErrorCode::DegenerateGeometry});
    }
    return Polygon2(std::move(points));
}

} // namespace huynity::objectlib
