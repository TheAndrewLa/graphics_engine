#pragma once

#include "huynity/math/vector.h"

#include <array>
#include <cmath>

namespace huynity::math {

template <std::size_t dimension>
    requires Dimension<dimension>
struct Point {
    std::array<double, dimension> coordinates{};

    constexpr double& operator[](std::size_t i) noexcept { return coordinates[i]; }
    constexpr double operator[](std::size_t i) const noexcept { return coordinates[i]; }
    friend constexpr bool operator==(const Point&, const Point&) = default;
};

using Point2 = Point<2>;
using Point3 = Point<3>;

template <std::size_t dimension>
[[nodiscard]] constexpr Vector<dimension> operator-(const Point<dimension>& a, const Point<dimension>& b) noexcept {
    Vector<dimension> result;
    for (std::size_t i = 0; i < dimension; ++i) {
        result[i] = a[i] - b[i];
    }
    return result;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Point<dimension> operator+(Point<dimension> p, const Vector<dimension>& v) noexcept {
    for (std::size_t i = 0; i < dimension; ++i) {
        p[i] += v[i];
    }
    return p;
}

template <std::size_t dimension>
[[nodiscard]] bool isFinite(const Point<dimension>& p) noexcept {
    for (double value : p.coordinates) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return true;
}

} // namespace huynity::math
