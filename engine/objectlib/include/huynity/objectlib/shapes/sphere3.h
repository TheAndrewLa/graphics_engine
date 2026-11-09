#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/sphere3_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Sphere3 {
public:
    [[nodiscard]] static CreateResult<Sphere3> create(Sphere3Params params);
    [[nodiscard]] double radius() const noexcept { return m_radius; }

private:
    explicit Sphere3(double radius) : m_radius{radius} {}
    double m_radius;
};
} // namespace huynity::objectlib
