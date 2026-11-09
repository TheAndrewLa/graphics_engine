#pragma once

#include "huynity/math/dimension.h"

#include <array>
#include <cmath>

namespace huynity::math {

template <std::size_t dimension>
    requires Dimension<dimension>
struct Vector {
    std::array<double, dimension> components{};

    constexpr double& operator[](std::size_t i) noexcept { return components[i]; }
    constexpr double operator[](std::size_t i) const noexcept { return components[i]; }
    friend constexpr bool operator==(const Vector&, const Vector&) = default;
};

using Vector2 = Vector<2>;
using Vector3 = Vector<3>;

template <std::size_t dimension>
[[nodiscard]] constexpr Vector<dimension> operator+(Vector<dimension> a, const Vector<dimension>& b) noexcept {
    for (std::size_t i = 0; i < dimension; ++i) {
        a[i] += b[i];
    }
    return a;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Vector<dimension> operator-(Vector<dimension> a, const Vector<dimension>& b) noexcept {
    for (std::size_t i = 0; i < dimension; ++i) {
        a[i] -= b[i];
    }
    return a;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Vector<dimension> operator*(Vector<dimension> v, double scalar) noexcept {
    for (double& value : v.components) {
        value *= scalar;
    }
    return v;
}

template <std::size_t dimension>
[[nodiscard]] constexpr double dot(const Vector<dimension>& a, const Vector<dimension>& b) noexcept {
    double result = 0;
    for (std::size_t i = 0; i < dimension; ++i) {
        result += a[i] * b[i];
    }
    return result;
}

[[nodiscard]] constexpr Vector3 cross(const Vector3& a, const Vector3& b) noexcept {
    return {{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}};
}

template <std::size_t dimension>
[[nodiscard]] double length(const Vector<dimension>& v) noexcept {
    if constexpr (dimension == 2) {
        return std::hypot(v[0], v[1]);
    } else {
        return std::hypot(v[0], v[1], v[2]);
    }
}

template <std::size_t dimension>
[[nodiscard]] bool isFinite(const Vector<dimension>& v) noexcept {
    for (double value : v.components) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return true;
}

} // namespace huynity::math
