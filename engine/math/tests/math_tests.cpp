#include "huynity/math/transform.h"

#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

namespace math = huynity::math;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

bool near(double a, double b, double tolerance = 1e-12) {
    return std::abs(a - b) <= tolerance;
}

template <std::size_t dimension>
void checkNear(const math::Point<dimension>& actual, const math::Point<dimension>& expected) {
    for (std::size_t i = 0; i < dimension; ++i) {
        CHECK(near(actual[i], expected[i]));
    }
}

void transforms2d() {
    constexpr math::Point2 point{{2, 3}};
    CHECK(math::transformPoint(math::Transform2{}, point) == point);
    CHECK(math::transformVector(math::translation(math::Vector2{{5, -2}}), {{1, 1}}) == math::Vector2{{1, 1}});

    const auto local = math::compose(math::rotation(std::numbers::pi / 2), math::scaling(math::Vector2{{2, 3}}));
    const auto world = math::compose(math::translation(math::Vector2{{5, -2}}), local);
    checkNear(math::transformPoint(local, point), {{-9, 4}});
    checkNear(math::transformPoint(world, point), {{-4, 2}});

    const auto inverse = math::inverse(world);
    CHECK(inverse.has_value());
    checkNear(math::transformPoint(*inverse, math::transformPoint(world, point)), point);
    checkNear(math::transformPoint(math::compose(world, *inverse), point), point);
    CHECK(!math::inverse(math::scaling(math::Vector2{{0, 1}})).has_value());
}

void transforms3d() {
    constexpr math::Point3 point{{2, 3, 4}};
    CHECK(math::transformPoint(math::Transform3{}, point) == point);
    CHECK(math::transformVector(math::translation(math::Vector3{{5, -2, 7}}), {{1, 1, 1}}) == math::Vector3{{1, 1, 1}});

    const auto local = math::compose(math::rotation(math::RotationAxis::Y, std::numbers::pi / 2),
                                     math::scaling(math::Vector3{{2, 3, 4}}));
    const auto world = math::compose(math::translation(math::Vector3{{5, -2, 7}}), local);
    checkNear(math::transformPoint(local, point), {{16, 9, -4}});
    checkNear(math::transformPoint(world, point), {{21, 7, 3}});

    const auto inverse = math::inverse(world);
    CHECK(inverse.has_value());
    checkNear(math::transformPoint(*inverse, math::transformPoint(world, point)), point);
    checkNear(math::transformPoint(math::compose(*inverse, world), point), point);
    CHECK(!math::inverse(math::scaling(math::Vector3{{1, 0, 1}})).has_value());
}

int main() {
    try {
        transforms2d();
        transforms3d();
        std::cout << "transforms: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
