#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/box3_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Box3 {
public:
    [[nodiscard]] static CreateResult<Box3> create(Box3Params params);
    [[nodiscard]] Vector3 size() const noexcept { return m_size; }

private:
    explicit Box3(Vector3 size) : m_size{size} {}
    Vector3 m_size;
};
} // namespace huynity::objectlib
