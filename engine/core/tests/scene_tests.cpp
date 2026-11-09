#include "huynity/core/scene.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace core = huynity::core;
namespace objects = huynity::objectlib;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

void typedCreationAndAtomicErrors() {
    core::Scene scene;
    auto created = scene.createObject<objects::ObjectType::Circle2>({.radius = 2});
    CHECK(created.has_value());
    CHECK(scene.size() == 1);
    const auto* instance = scene.find(*created);
    CHECK(instance != nullptr);
    CHECK(instance->geometry != nullptr);
    CHECK(std::get<objects::Circle2>(*instance->geometry).radius() == 2);

    auto rejected = scene.createObject<objects::ObjectType::Circle2>({.radius = 0});
    CHECK(!rejected.has_value());
    CHECK(rejected.error().code == objects::CreateErrorCode::InvalidRadius);
    CHECK(scene.size() == 1);
    CHECK(scene.find(*created) == instance);
    CHECK(std::get<objects::Circle2>(*instance->geometry).radius() == 2);
}

void runtimeCreationAndAtomicErrors() {
    core::Scene scene;
    auto created = scene.createObject(objects::ObjectSpec{objects::Sphere3Params{.radius = 3}});
    CHECK(created.has_value());
    CHECK(scene.size() == 1);
    CHECK(std::get<objects::Sphere3>(*scene.find(*created)->geometry).radius() == 3);

    auto rejected = scene.createObject(objects::ObjectSpec{objects::Sphere3Params{.radius = -1}});
    CHECK(!rejected.has_value());
    CHECK(rejected.error().code == objects::CreateErrorCode::InvalidRadius);
    CHECK(scene.size() == 1);
    CHECK(std::get<objects::Sphere3>(*scene.find(*created)->geometry).radius() == 3);
}

void sharedGeometry() {
    core::Scene scene;
    const auto source = scene.createObject<objects::ObjectType::Box3>({.size = {{1, 2, 3}}});
    CHECK(source.has_value());
    const auto* original = scene.find(*source);
    const auto resource = original->geometry;
    const auto geometryVersion = original->geometryVersion;
    auto second = scene.createInstance(*source);
    CHECK(second.has_value());
    CHECK(*second != *source);
    CHECK(scene.size() == 2);
    CHECK(scene.find(*second)->geometry == resource);
    CHECK(scene.find(*second)->geometryVersion == geometryVersion);
    CHECK(resource.use_count() == 3);

    auto rejected = scene.createInstance(core::ObjectId{1000, 1});
    CHECK(!rejected.has_value());
    CHECK(scene.size() == 2);
}

int main() {
    try {
        typedCreationAndAtomicErrors();
        runtimeCreationAndAtomicErrors();
        sharedGeometry();
        std::cout << "scene: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
