#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/plane3_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Plane3 {
public:
    [[nodiscard]] static CreateResult<Plane3> create(Plane3Params params);
    [[nodiscard]] Vector3 normal() const noexcept { return m_normal; }
    [[nodiscard]] double offset() const noexcept { return m_offset; }

private:
    Plane3(Vector3 normal, double offset) : m_normal{normal}, m_offset{offset} {}
    Vector3 m_normal;
    double m_offset;
};
} // namespace huynity::objectlib
