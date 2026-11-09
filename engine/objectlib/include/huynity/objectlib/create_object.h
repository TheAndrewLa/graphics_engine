#pragma once

#include "huynity/objectlib/errors.h"
#include "huynity/objectlib/object_traits.h"

#include <utility>

namespace huynity::objectlib {

template <ObjectType kind>
    requires SupportedObject<kind>
[[nodiscard]] CreateResult<ObjectFor<kind>> createObject(ParamsFor<kind> params) {
    if constexpr (kind == ObjectType::Point2 || kind == ObjectType::Point3) {
        ObjectFor<kind> point{params.coordinates};
        if (!math::isFinite(point)) {
            return std::unexpected(CreateError{CreateErrorCode::NonFiniteParameter});
        }
        return point;
    } else {
        return ObjectFor<kind>::create(std::move(params));
    }
}
[[nodiscard]] CreateResult<Geometry> createObject(ObjectSpec spec);
} // namespace huynity::objectlib
