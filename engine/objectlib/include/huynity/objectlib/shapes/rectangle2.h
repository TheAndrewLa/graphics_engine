#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/params/rectangle2_params.h"

#include <span>
#include <utility>

namespace huynity::objectlib {

class Rectangle2 {
public:
    [[nodiscard]] static CreateResult<Rectangle2> create(Rectangle2Params params);
    [[nodiscard]] Vector2 size() const noexcept { return m_size; }

private:
    explicit Rectangle2(Vector2 size) : m_size{size} {}
    Vector2 m_size;
};
} // namespace huynity::objectlib
