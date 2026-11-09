#include "huynity/render_contracts/render_snapshot.h"

#include <array>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace contracts = huynity::render_contracts;
namespace math = huynity::math;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

static_assert(std::same_as<contracts::ResourceIndex, std::size_t>);
static_assert(std::same_as<contracts::MaterialIndex, std::size_t>);
static_assert(std::variant_size_v<contracts::Camera3> == 2);
static_assert(std::variant_size_v<contracts::RenderPass> == 3);
static_assert(std::is_same_v<decltype(contracts::DrawItem3::localToWorld), math::Transform3>);
static_assert(std::is_same_v<decltype(contracts::DrawItem2::localToWorld), math::Transform2>);
static_assert(std::is_same_v<decltype(contracts::MeshResource3::positions), std::span<const math::Point3>>);

contracts::RenderSnapshot makeSnapshot() {
    auto positions = std::make_shared<const std::vector<math::Point3>>(std::vector<math::Point3>{{{0, 0, 0}}});
    auto triangles = std::make_shared<const std::vector<std::array<std::uint32_t, 3>>>();
    auto material = std::make_shared<const contracts::Material>();

    contracts::RenderSnapshot snapshot;
    snapshot.sceneRevision = 4;
    snapshot.renderRevision = 7;
    snapshot.imageWidth = 640;
    snapshot.imageHeight = 480;
    snapshot.materials.push_back(material);
    snapshot.resources3.push_back({positions, *positions, {}});
    snapshot.passes.emplace_back(contracts::World2DPass{});
    snapshot.passes.emplace_back(contracts::World3DPass{});
    snapshot.passes.emplace_back(contracts::Hud2DPass{});
    auto& world = std::get<contracts::World3DPass>(snapshot.passes[1]);
    world.camera = contracts::PerspectiveCamera3{.verticalFieldOfView = 1, .nearPlane = 0.1, .farPlane = 100};
    world.drawItems.push_back({.resource = 0, .material = 0});
    world.lighting.directional.push_back({.direction = {{0, 0, -1}}, .intensity = 1});
    return snapshot;
}

void snapshotContract() {
    auto snapshot = makeSnapshot();
    CHECK(snapshot.passes.size() == 3);
    CHECK(std::holds_alternative<contracts::World2DPass>(snapshot.passes[0]));
    CHECK(std::holds_alternative<contracts::World3DPass>(snapshot.passes[1]));
    CHECK(std::holds_alternative<contracts::Hud2DPass>(snapshot.passes[2]));
    CHECK(snapshot.resources3[0].owner != nullptr);
    CHECK(snapshot.materials[0].use_count() == 1);
    CHECK(std::get<contracts::World3DPass>(snapshot.passes[1]).drawItems[0].resource == 0);
    CHECK(std::get<contracts::World3DPass>(snapshot.passes[1]).lighting.directional.size() == 1);

    auto second = makeSnapshot();
    second.resources3[0] = snapshot.resources3[0];
    CHECK(second.resources3[0].positions.data() == snapshot.resources3[0].positions.data());
    CHECK(snapshot.resources3[0].owner.use_count() == 2);
}

int main() {
    try {
        snapshotContract();
        std::cout << "api: OK\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
