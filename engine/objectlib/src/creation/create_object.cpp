#include "huynity/objectlib/create_object.h"

#include <type_traits>
#include <utility>

namespace huynity::objectlib {

namespace {
template <typename P>
struct ObjectKind;
template <>
struct ObjectKind<Point2Params> {
    static constexpr ObjectType value = ObjectType::Point2;
};

template <>
struct ObjectKind<Point3Params> {
    static constexpr ObjectType value = ObjectType::Point3;
};

template <>
struct ObjectKind<Segment2Params> {
    static constexpr ObjectType value = ObjectType::Segment2;
};

template <>
struct ObjectKind<Segment3Params> {
    static constexpr ObjectType value = ObjectType::Segment3;
};

template <>
struct ObjectKind<Line2Params> {
    static constexpr ObjectType value = ObjectType::Line2;
};

template <>
struct ObjectKind<Line3Params> {
    static constexpr ObjectType value = ObjectType::Line3;
};

template <>
struct ObjectKind<Ray2Params> {
    static constexpr ObjectType value = ObjectType::Ray2;
};

template <>
struct ObjectKind<Ray3Params> {
    static constexpr ObjectType value = ObjectType::Ray3;
};

template <>
struct ObjectKind<Triangle2Params> {
    static constexpr ObjectType value = ObjectType::Triangle2;
};

template <>
struct ObjectKind<Triangle3Params> {
    static constexpr ObjectType value = ObjectType::Triangle3;
};

template <>
struct ObjectKind<Polyline2Params> {
    static constexpr ObjectType value = ObjectType::Polyline2;
};

template <>
struct ObjectKind<Polyline3Params> {
    static constexpr ObjectType value = ObjectType::Polyline3;
};

template <>
struct ObjectKind<Aabb2Params> {
    static constexpr ObjectType value = ObjectType::Aabb2;
};

template <>
struct ObjectKind<Aabb3Params> {
    static constexpr ObjectType value = ObjectType::Aabb3;
};

template <>
struct ObjectKind<TriangleMesh2Params> {
    static constexpr ObjectType value = ObjectType::TriangleMesh2;
};

template <>
struct ObjectKind<TriangleMesh3Params> {
    static constexpr ObjectType value = ObjectType::TriangleMesh3;
};

template <>
struct ObjectKind<Plane3Params> {
    static constexpr ObjectType value = ObjectType::Plane3;
};

template <>
struct ObjectKind<Polygon2Params> {
    static constexpr ObjectType value = ObjectType::Polygon2;
};

template <>
struct ObjectKind<Rectangle2Params> {
    static constexpr ObjectType value = ObjectType::Rectangle2;
};

template <>
struct ObjectKind<Circle2Params> {
    static constexpr ObjectType value = ObjectType::Circle2;
};

template <>
struct ObjectKind<Disk2Params> {
    static constexpr ObjectType value = ObjectType::Disk2;
};

template <>
struct ObjectKind<Sphere3Params> {
    static constexpr ObjectType value = ObjectType::Sphere3;
};

template <>
struct ObjectKind<Box3Params> {
    static constexpr ObjectType value = ObjectType::Box3;
};
} // namespace

CreateResult<Geometry> createObject(ObjectSpec spec) {
    if (spec.valueless_by_exception()) {
        return std::unexpected(CreateError{CreateErrorCode::InvalidDescription});
    }
    return std::visit(
        [](auto params) -> CreateResult<Geometry> {
            using Params = std::remove_cvref_t<decltype(params)>;
            constexpr auto kind = ObjectKind<Params>::value;
            auto object = createObject<kind>(std::move(params));
            if (!object) {
                return std::unexpected(object.error());
            }
            return Geometry{std::in_place_type<ObjectFor<kind>>, std::move(*object)};
        },
        std::move(spec));
}

} // namespace huynity::objectlib
