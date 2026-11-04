#include "huynity/objectlib/operations/bounds.h"
#include "huynity/objectlib/operations/tessellate.h"

#include <concepts>
#include <utility>
#include <variant>

namespace huynity::objectlib {

namespace {

template <typename T>
RuntimeBoundsResult toRuntimeBounds(GeometryResult<T> result) {
    if (!result) {
        return std::unexpected(result.error());
    }
    return BoundsGeometry{std::in_place_type<T>, std::move(*result)};
}

template <typename T>
concept HasTypedBounds = requires(const T& geometry) { bounds(geometry); } &&
                         (!std::same_as<decltype(bounds(std::declval<const T&>())), RuntimeBoundsResult>);

template <typename T>
RuntimeBoundsResult runtimeBounds(const T& geometry) {
    if constexpr (HasTypedBounds<T>) {
        return toRuntimeBounds(bounds(geometry));
    } else {
        return std::unexpected(GeometryError{GeometryErrorCode::UnsupportedOperation});
    }
}

template <typename T>
RuntimeTessellationResult toRuntimeTessellation(GeometryResult<T> result) {
    if (!result) {
        return std::unexpected(result.error());
    }
    return TessellatedGeometry{std::in_place_type<T>, std::move(*result)};
}

template <typename T>
concept HasTypedTessellation =
    requires(const T& geometry, TessellationOptions options) { tessellate(geometry, options); } &&
    (!std::same_as<decltype(tessellate(std::declval<const T&>(), std::declval<TessellationOptions>())),
                   RuntimeTessellationResult>);

template <typename T>
RuntimeTessellationResult runtimeTessellation(const T& geometry, TessellationOptions options) {
    if constexpr (std::same_as<T, Circle2>) {
        return toRuntimeTessellation(sampleCircle(geometry, options));
    } else if constexpr (HasTypedTessellation<T>) {
        return toRuntimeTessellation(tessellate(geometry, options));
    } else {
        return std::unexpected(GeometryError{GeometryErrorCode::UnsupportedOperation});
    }
}

} // namespace

RuntimeBoundsResult bounds(const Geometry& geometry) {
    if (geometry.valueless_by_exception()) {
        return std::unexpected(GeometryError{GeometryErrorCode::InvalidGeometry});
    }
    return std::visit([](const auto& concrete) -> RuntimeBoundsResult { return runtimeBounds(concrete); }, geometry);
}

RuntimeTessellationResult tessellate(const Geometry& geometry, TessellationOptions options) {
    if (geometry.valueless_by_exception()) {
        return std::unexpected(GeometryError{GeometryErrorCode::InvalidGeometry});
    }
    return std::visit(
        [options](const auto& concrete) -> RuntimeTessellationResult { return runtimeTessellation(concrete, options); },
        geometry);
}

} // namespace huynity::objectlib
