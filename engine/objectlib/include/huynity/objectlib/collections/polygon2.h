#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/polygon2_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Polygon2 {
public:
    [[nodiscard]] static CreateResult<Polygon2> create(Polygon2Params params);
    [[nodiscard]] std::span<const Point2> contour() const noexcept { return m_contour; }

private:
    explicit Polygon2(std::vector<Point2> contour) : m_contour{std::move(contour)} {}
    std::vector<Point2> m_contour;
};
} // namespace huynity::objectlib
