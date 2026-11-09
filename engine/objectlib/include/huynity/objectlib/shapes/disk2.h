#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/disk2_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Disk2 {
public:
    [[nodiscard]] static CreateResult<Disk2> create(Disk2Params params);
    [[nodiscard]] double radius() const noexcept { return m_radius; }

private:
    explicit Disk2(double radius) : m_radius{radius} {}
    double m_radius;
};
} // namespace huynity::objectlib
