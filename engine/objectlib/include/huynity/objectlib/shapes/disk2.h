#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/object_type.h"

namespace huynity::objectlib {

class Disk2 {
public:
    struct Params {
        double radius;
    };

    static constexpr auto Dimension = 2;

    static constexpr auto Type = ObjectType::Disk;

    [[nodiscard]] static CreateResult<Disk2> create(Params params);
    [[nodiscard]] static CreateResult<Disk2> create(double radius);

    [[nodiscard]] double radius() const noexcept { return m_radius; }

private:
    explicit Disk2(double radius) : m_radius{radius} {}
    double m_radius;
};
} // namespace huynity::objectlib
