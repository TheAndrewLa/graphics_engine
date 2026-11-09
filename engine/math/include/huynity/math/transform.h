#pragma once

#include "huynity/math/point.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace huynity::math {

template <std::size_t dimension>
    requires Dimension<dimension>
struct Transform {
    static constexpr std::size_t rowSize = dimension + 1;
    static constexpr std::size_t valueCount = dimension * rowSize;

    [[nodiscard]] static constexpr std::array<double, valueCount> identityValues() noexcept {
        std::array<double, valueCount> result{};
        for (std::size_t i = 0; i < dimension; ++i) {
            result[i * rowSize + i] = 1;
        }
        return result;
    }

    std::array<double, valueCount> values = identityValues();

    [[nodiscard]] constexpr double linear(std::size_t row, std::size_t column) const noexcept {
        return values[row * rowSize + column];
    }
    [[nodiscard]] constexpr double translation(std::size_t axis) const noexcept {
        return values[axis * rowSize + dimension];
    }
    friend constexpr bool operator==(const Transform&, const Transform&) = default;
};

using Transform2 = Transform<2>;
using Transform3 = Transform<3>;

template <std::size_t dimension>
[[nodiscard]] constexpr Transform<dimension> translation(const Vector<dimension>& offset) noexcept {
    Transform<dimension> result;
    for (std::size_t i = 0; i < dimension; ++i) {
        result.values[i * Transform<dimension>::rowSize + dimension] = offset[i];
    }
    return result;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Transform<dimension> scaling(const Vector<dimension>& factors) noexcept {
    Transform<dimension> result;
    for (std::size_t i = 0; i < dimension; ++i) {
        result.values[i * Transform<dimension>::rowSize + i] = factors[i];
    }
    return result;
}

[[nodiscard]] inline Transform2 rotation(double angle) noexcept {
    Transform2 result;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    result.values[0] = cosine;
    result.values[1] = -sine;
    result.values[3] = sine;
    result.values[4] = cosine;
    return result;
}

enum class RotationAxis {
    X,
    Y,
    Z,
};

[[nodiscard]] inline Transform3 rotation(RotationAxis axis, double angle) noexcept {
    Transform3 result;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    switch (axis) {
    case RotationAxis::X: {
        result.values[5] = cosine;
        result.values[6] = -sine;
        result.values[9] = sine;
        result.values[10] = cosine;
        break;
    }
    case RotationAxis::Y: {
        result.values[0] = cosine;
        result.values[2] = sine;
        result.values[8] = -sine;
        result.values[10] = cosine;
        break;
    }
    case RotationAxis::Z: {
        result.values[0] = cosine;
        result.values[1] = -sine;
        result.values[4] = sine;
        result.values[5] = cosine;
        break;
    }
    }
    return result;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Point<dimension> transformPoint(const Transform<dimension>& transform,
                                                        const Point<dimension>& point) noexcept {
    Point<dimension> result;
    for (std::size_t row = 0; row < dimension; ++row) {
        for (std::size_t column = 0; column < dimension; ++column) {
            result[row] += transform.linear(row, column) * point[column];
        }
        result[row] += transform.translation(row);
    }
    return result;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Vector<dimension> transformVector(const Transform<dimension>& transform,
                                                          const Vector<dimension>& vector) noexcept {
    Vector<dimension> result;
    for (std::size_t row = 0; row < dimension; ++row) {
        for (std::size_t column = 0; column < dimension; ++column) {
            result[row] += transform.linear(row, column) * vector[column];
        }
    }
    return result;
}

template <std::size_t dimension>
[[nodiscard]] constexpr Transform<dimension> compose(const Transform<dimension>& parent,
                                                     const Transform<dimension>& local) noexcept {
    Transform<dimension> result;
    constexpr std::size_t rowSize = Transform<dimension>::rowSize;
    for (std::size_t row = 0; row < dimension; ++row) {
        for (std::size_t column = 0; column < rowSize; ++column) {
            double value = 0;
            for (std::size_t i = 0; i < dimension; ++i) {
                value += parent.linear(row, i) * local.values[i * rowSize + column];
            }
            if (column == dimension) {
                value += parent.translation(row);
            }
            result.values[row * rowSize + column] = value;
        }
    }
    return result;
}

[[nodiscard]] constexpr std::optional<Transform2> inverse(const Transform2& transform) noexcept {
    const double a = transform.linear(0, 0);
    const double b = transform.linear(0, 1);
    const double c = transform.linear(1, 0);
    const double d = transform.linear(1, 1);
    const double determinant = a * d - b * c;
    if (determinant == 0) {
        return std::nullopt;
    }

    Transform2 result;
    result.values[0] = d / determinant;
    result.values[1] = -b / determinant;
    result.values[3] = -c / determinant;
    result.values[4] = a / determinant;
    result.values[2] = -(result.values[0] * transform.translation(0) + result.values[1] * transform.translation(1));
    result.values[5] = -(result.values[3] * transform.translation(0) + result.values[4] * transform.translation(1));
    return result;
}

[[nodiscard]] constexpr std::optional<Transform3> inverse(const Transform3& transform) noexcept {
    const double a = transform.linear(0, 0);
    const double b = transform.linear(0, 1);
    const double c = transform.linear(0, 2);
    const double d = transform.linear(1, 0);
    const double e = transform.linear(1, 1);
    const double f = transform.linear(1, 2);
    const double g = transform.linear(2, 0);
    const double h = transform.linear(2, 1);
    const double i = transform.linear(2, 2);
    const double determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (determinant == 0) {
        return std::nullopt;
    }

    Transform3 result;
    result.values[0] = (e * i - f * h) / determinant;
    result.values[1] = (c * h - b * i) / determinant;
    result.values[2] = (b * f - c * e) / determinant;
    result.values[4] = (f * g - d * i) / determinant;
    result.values[5] = (a * i - c * g) / determinant;
    result.values[6] = (c * d - a * f) / determinant;
    result.values[8] = (d * h - e * g) / determinant;
    result.values[9] = (b * g - a * h) / determinant;
    result.values[10] = (a * e - b * d) / determinant;
    for (std::size_t row = 0; row < 3; ++row) {
        double value = 0;
        for (std::size_t column = 0; column < 3; ++column) {
            value += result.linear(row, column) * transform.translation(column);
        }
        result.values[row * Transform3::rowSize + 3] = -value;
    }
    return result;
}

} // namespace huynity::math
