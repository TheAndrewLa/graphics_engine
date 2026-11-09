#include "huynity/objectlib/detail/validation.h"
#include "huynity/objectlib/primitives/plane3.h"
#include "huynity/objectlib/shapes/box3.h"
#include "huynity/objectlib/shapes/circle2.h"
#include "huynity/objectlib/shapes/disk2.h"
#include "huynity/objectlib/shapes/rectangle2.h"
#include "huynity/objectlib/shapes/sphere3.h"

#include <cmath>

namespace huynity::objectlib {

namespace {

CreateResult<void> validateRadius(double radius) {
    if (!std::isfinite(radius)) {
        return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
    }
    if (radius <= 0) {
        return std::unexpected(CreateError{CreateErrorCode::InvalidRadius});
    }
    return {};
}

template <std::size_t dimension>
CreateResult<void> validateSize(const Vector<dimension>& size) {
    if (!math::isFinite(size)) {
        return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
    }
    for (double value : size.components) {
        if (value <= 0) {
            return std::unexpected(CreateError{CreateErrorCode::InvalidDimensions});
        }
    }
    return {};
}

} // namespace

CreateResult<Sphere3> Sphere3::create(Sphere3Params params) {
    if (const auto valid = validateRadius(params.radius); !valid) {
        return std::unexpected(valid.error());
    }
    return Sphere3(params.radius);
}

CreateResult<Circle2> Circle2::create(Circle2Params params) {
    if (const auto valid = validateRadius(params.radius); !valid) {
        return std::unexpected(valid.error());
    }
    return Circle2(params.radius);
}

CreateResult<Disk2> Disk2::create(Disk2Params params) {
    if (const auto valid = validateRadius(params.radius); !valid) {
        return std::unexpected(valid.error());
    }
    return Disk2(params.radius);
}

CreateResult<Rectangle2> Rectangle2::create(Rectangle2Params params) {
    if (const auto valid = validateSize(params.size); !valid) {
        return std::unexpected(valid.error());
    }
    return Rectangle2(params.size);
}

CreateResult<Box3> Box3::create(Box3Params params) {
    if (const auto valid = validateSize(params.size); !valid) {
        return std::unexpected(valid.error());
    }
    return Box3(params.size);
}

CreateResult<Plane3> Plane3::create(Plane3Params params) {
    if (!math::isFinite(params.normal) || !std::isfinite(params.offset)) {
        return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
    }
    if (!detail::nonzero(params.normal)) {
        return std::unexpected(CreateError{CreateErrorCode::InvalidDirection});
    }
    return Plane3(params.normal, params.offset);
}

} // namespace huynity::objectlib
