#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/circle2_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Circle2 {
public:
    [[nodiscard]] static CreateResult<Circle2> create(Circle2Params params);
    [[nodiscard]] double radius() const noexcept { return m_radius; }

private:
    explicit Circle2(double radius) : m_radius{radius} {}
    double m_radius;
};
} // namespace huynity::objectlib
