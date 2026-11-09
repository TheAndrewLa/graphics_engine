#include "huynity/renderer/software_renderer.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace render = huynity::render_contracts;
namespace image = huynity::image;
namespace math = huynity::math;
namespace renderer = huynity::renderer;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

struct Rgba {
    int red;
    int green;
    int blue;
    int alpha;

    friend bool operator==(const Rgba&, const Rgba&) = default;
};

constexpr Rgba BLACK{0, 0, 0, 255};
constexpr Rgba RED{255, 0, 0, 255};
constexpr Rgba GREEN{0, 255, 0, 255};
constexpr Rgba BLUE{0, 0, 255, 255};

struct TestMesh3 {
    std::vector<math::Point3> positions;
    std::vector<render::TriangleIndices> triangles;
    std::shared_ptr<const void> owner = std::make_shared<const int>(0);
};

struct TestMesh2 {
    std::vector<math::Point2> positions;
    std::vector<render::TriangleIndices> triangles;
    std::shared_ptr<const void> owner = std::make_shared<const int>(0);
};

render::Color color(double red, double green, double blue, double alpha = 1.0) {
    return {red, green, blue, alpha};
}

std::shared_ptr<const render::Material> material(double red, double green, double blue, double alpha = 1.0) {
    return std::make_shared<const render::Material>(color(red, green, blue, alpha));
}

void addResource3(render::RenderSnapshot& snapshot, const TestMesh3& mesh) {
    render::MeshResource3 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});
    snapshot.resources3.push_back(resource);
}

void addResource2(render::RenderSnapshot& snapshot, const TestMesh2& mesh) {
    render::MeshResource2 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});
    snapshot.resources2.push_back(resource);
}

render::RenderSnapshot makeSnapshot(std::size_t width, std::size_t height) {
    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    snapshot.clearColor = color(0, 0, 0);
    snapshot.materials = {material(1, 0, 0), material(0, 1, 0), material(0, 0, 1)};
    return snapshot;
}

render::Lighting3 unlitLighting() {
    return {{render::Color{1, 1, 1, 1}, 1}, {}};
}

image::ImageBuffer makeFramebuffer(std::size_t width, std::size_t height) {
    auto created = image::ImageBuffer::create({width, height});
    if (!created) {
        throw std::runtime_error("failed to create framebuffer");
    }
    return std::move(*created);
}

Rgba pixel(const image::ImageBuffer& framebuffer, std::size_t x, std::size_t y) {
    const auto bytes = framebuffer.bytes();
    const std::size_t offset = y * framebuffer.stride() + x * 4;
    return {std::to_integer<int>(bytes[offset]), std::to_integer<int>(bytes[offset + 1]),
            std::to_integer<int>(bytes[offset + 2]), std::to_integer<int>(bytes[offset + 3])};
}

std::size_t countColor(const image::ImageBuffer& framebuffer, const Rgba& target) {
    std::size_t count = 0;
    for (std::size_t y = 0; y < framebuffer.height(); ++y) {
        for (std::size_t x = 0; x < framebuffer.width(); ++x) {
            if (pixel(framebuffer, x, y) == target) {
                ++count;
            }
        }
    }
    return count;
}

// Лицевой треугольник: нормаль в -z при обходе (0 -> 1 -> 2)
TestMesh3 makeFrontTriangle3(double extent, double z) {
    return {{{-extent, -extent, z}, {0, extent, z}, {extent, -extent, z}}, {{0, 1, 2}}, {}};
}

TestMesh2 makeTriangle2(double extent) {
    return {{{-extent, -extent}, {0, extent}, {extent, -extent}}, {{0, 1, 2}}, {}};
}

void test3DWireframeVsSolid() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    // extent = 16 при OrthoCamera 64x64:
    // V0(-16, -16) -> экран (16, 48)
    // V1(0, 16)    -> экран (32, 16)
    // V2(16, -16)  -> экран (48, 48)
    // Центр (0, 0) -> экран (32, 32)
    const auto mesh = makeFrontTriangle3(16.0, 10.0);

    // 1. Solid rendering (wireframe = false)
    {
        auto snapshot = makeSnapshot(width, height);
        addResource3(snapshot, mesh);

        render::World3DPass pass;
        pass.camera = render::OrthographicCamera3{math::Transform3{}, 64.0, 64.0, 0.1, 100.0};
        pass.lighting = unlitLighting();
        // material 1 = GREEN
        pass.drawItems.push_back(
            render::DrawItem3{.resource = 0, .material = 1, .backfaceCulling = true, .wireframe = false});
        snapshot.passes.push_back(std::move(pass));

        renderer::SoftwareRenderer sr;
        auto framebuffer = makeFramebuffer(width, height);
        CHECK(sr.render(snapshot, framebuffer).has_value());

        // Внутренний центр закрашен зеленым
        CHECK(pixel(framebuffer, 32, 32) == GREEN);
        // Внутренняя область ближе к нижнему ребру закрашена
        CHECK(pixel(framebuffer, 32, 40) == GREEN);

        const auto solidCount = countColor(framebuffer, GREEN);
        CHECK(solidCount > 200); // заполнена площадь треугольника
    }

    // 2. Wireframe rendering (wireframe = true)
    {
        auto snapshot = makeSnapshot(width, height);
        addResource3(snapshot, mesh);

        render::World3DPass pass;
        pass.camera = render::OrthographicCamera3{math::Transform3{}, 64.0, 64.0, 0.1, 100.0};
        pass.lighting = unlitLighting();
        pass.drawItems.push_back(
            render::DrawItem3{.resource = 0, .material = 1, .backfaceCulling = true, .wireframe = true});
        snapshot.passes.push_back(std::move(pass));

        renderer::SoftwareRenderer sr;
        auto framebuffer = makeFramebuffer(width, height);
        CHECK(sr.render(snapshot, framebuffer).has_value());

        // ВАЖНО: Внутренний пиксель треугольника (32, 32) НЕ закрашен и остаётся фоновым!
        CHECK(pixel(framebuffer, 32, 32) == BLACK);

        // Пиксели рёбер окрашены в цвет объекта:
        // Нижнее горизонтальное ребро (16, 48) -> (48, 48):
        CHECK(pixel(framebuffer, 32, 48) == GREEN);
        // Вершины:
        CHECK(pixel(framebuffer, 16, 48) == GREEN);
        CHECK(pixel(framebuffer, 48, 48) == GREEN);
        CHECK(pixel(framebuffer, 32, 16) == GREEN);

        const auto wireCount = countColor(framebuffer, GREEN);
        CHECK(wireCount > 0);
        // В каркасном режиме закрашены только рёбра, поэтому пикселей значительно меньше
        CHECK(wireCount < 150);
    }
}

void test3DWireframeDepthTesting() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    // Ближний треугольник: z = 10, красный (material 0), wireframe
    const auto nearMesh = makeFrontTriangle3(16.0, 10.0);
    // Дальний треугольник: z = 20, синий (material 2), wireframe
    const auto farMesh = makeFrontTriangle3(16.0, 20.0);

    auto snapshot = makeSnapshot(width, height);
    addResource3(snapshot, nearMesh);
    addResource3(snapshot, farMesh);

    render::World3DPass pass;
    pass.camera = render::OrthographicCamera3{math::Transform3{}, 64.0, 64.0, 0.1, 100.0};
    pass.lighting = unlitLighting();

    // Сначала дальний (синий), затем ближний (красный)
    pass.drawItems.push_back(
        render::DrawItem3{.resource = 1, .material = 2, .backfaceCulling = true, .wireframe = true});
    pass.drawItems.push_back(
        render::DrawItem3{.resource = 0, .material = 0, .backfaceCulling = true, .wireframe = true});
    snapshot.passes.push_back(std::move(pass));

    renderer::SoftwareRenderer sr;
    auto framebuffer = makeFramebuffer(width, height);
    CHECK(sr.render(snapshot, framebuffer).has_value());

    // На нижнем ребре (32, 48) оба треугольника имеют ребро, но ближний z = 10 перекрывает z = 20
    CHECK(pixel(framebuffer, 32, 48) == RED);
    // Внутренняя область остаётся фоновой
    CHECK(pixel(framebuffer, 32, 32) == BLACK);
}

void test3DWireframePerspective() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    // В перспективе: fov = pi / 2 (90 deg), tanHalfFov = 1.0
    // При z = 10: halfHeightAtDepth = 10.0
    // extent = 5.0 -> ndc = 0.5 -> screen = 48
    const auto mesh = makeFrontTriangle3(5.0, 10.0);

    auto snapshot = makeSnapshot(width, height);
    addResource3(snapshot, mesh);

    render::World3DPass pass;
    pass.camera = render::PerspectiveCamera3{.worldToView = math::Transform3{},
                                             .verticalFieldOfView = std::numbers::pi / 2,
                                             .nearPlane = 0.1,
                                             .farPlane = 100.0};
    pass.lighting = unlitLighting();
    pass.drawItems.push_back(
        render::DrawItem3{.resource = 0, .material = 1, .backfaceCulling = true, .wireframe = true});
    snapshot.passes.push_back(std::move(pass));

    renderer::SoftwareRenderer sr;
    auto framebuffer = makeFramebuffer(width, height);
    CHECK(sr.render(snapshot, framebuffer).has_value());

    // Внутренний центр (32, 32) не закрашен
    CHECK(pixel(framebuffer, 32, 32) == BLACK);
    // Рёбра закрашены
    CHECK(countColor(framebuffer, GREEN) > 0);
}

void test2DWorldWireframeVsSolid() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    const auto mesh = makeTriangle2(16.0);

    // 1. Solid
    {
        auto snapshot = makeSnapshot(width, height);
        addResource2(snapshot, mesh);

        render::World2DPass pass;
        pass.camera = render::Camera2{.worldToView = math::Transform2{}, .width = 64.0, .height = 64.0};
        pass.drawItems.push_back(
            render::DrawItem2{.resource = 0, .material = 1, .layer = 0, .order = 0, .wireframe = false});
        snapshot.passes.push_back(std::move(pass));

        renderer::SoftwareRenderer sr;
        auto framebuffer = makeFramebuffer(width, height);
        CHECK(sr.render(snapshot, framebuffer).has_value());

        CHECK(pixel(framebuffer, 32, 32) == GREEN);
        CHECK(pixel(framebuffer, 32, 40) == GREEN);
        CHECK(countColor(framebuffer, GREEN) > 200);
    }

    // 2. Wireframe
    {
        auto snapshot = makeSnapshot(width, height);
        addResource2(snapshot, mesh);

        render::World2DPass pass;
        pass.camera = render::Camera2{.worldToView = math::Transform2{}, .width = 64.0, .height = 64.0};
        pass.drawItems.push_back(
            render::DrawItem2{.resource = 0, .material = 1, .layer = 0, .order = 0, .wireframe = true});
        snapshot.passes.push_back(std::move(pass));

        renderer::SoftwareRenderer sr;
        auto framebuffer = makeFramebuffer(width, height);
        CHECK(sr.render(snapshot, framebuffer).has_value());

        // Внутренний пиксель НЕ закрашен!
        CHECK(pixel(framebuffer, 32, 32) == BLACK);

        // Пиксели рёбер окрашены
        CHECK(pixel(framebuffer, 32, 48) == GREEN);
        CHECK(pixel(framebuffer, 16, 48) == GREEN);
        CHECK(pixel(framebuffer, 48, 48) == GREEN);
        CHECK(pixel(framebuffer, 32, 16) == GREEN);

        const auto wireCount = countColor(framebuffer, GREEN);
        CHECK(wireCount > 0);
        CHECK(wireCount < 150);
    }
}

void test2DHudWireframeVsSolid() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    // В HUD координаты вершин задаются прямо в экранных координатах
    TestMesh2 hudMesh;
    hudMesh.positions = {{{16.0, 48.0}, {32.0, 16.0}, {48.0, 48.0}}};
    hudMesh.triangles = {{0, 1, 2}};

    // 1. Solid HUD
    {
        auto snapshot = makeSnapshot(width, height);
        addResource2(snapshot, hudMesh);

        render::Hud2DPass pass;
        pass.drawItems.push_back(
            render::DrawItem2{.resource = 0, .material = 1, .layer = 0, .order = 0, .wireframe = false});
        snapshot.passes.push_back(std::move(pass));

        renderer::SoftwareRenderer sr;
        auto framebuffer = makeFramebuffer(width, height);
        CHECK(sr.render(snapshot, framebuffer).has_value());

        CHECK(pixel(framebuffer, 32, 32) == GREEN);
        CHECK(pixel(framebuffer, 32, 40) == GREEN);
        CHECK(countColor(framebuffer, GREEN) > 200);
    }

    // 2. Wireframe HUD
    {
        auto snapshot = makeSnapshot(width, height);
        addResource2(snapshot, hudMesh);

        render::Hud2DPass pass;
        pass.drawItems.push_back(
            render::DrawItem2{.resource = 0, .material = 1, .layer = 0, .order = 0, .wireframe = true});
        snapshot.passes.push_back(std::move(pass));

        renderer::SoftwareRenderer sr;
        auto framebuffer = makeFramebuffer(width, height);
        CHECK(sr.render(snapshot, framebuffer).has_value());

        // Внутренний центр НЕ закрашен!
        CHECK(pixel(framebuffer, 32, 32) == BLACK);

        // Рёбра закрашены
        CHECK(pixel(framebuffer, 32, 48) == GREEN);
        CHECK(pixel(framebuffer, 16, 48) == GREEN);
        CHECK(pixel(framebuffer, 48, 48) == GREEN);
        CHECK(pixel(framebuffer, 32, 16) == GREEN);

        const auto wireCount = countColor(framebuffer, GREEN);
        CHECK(wireCount > 0);
        CHECK(wireCount < 150);
    }
}

int main() {
    try {
        test3DWireframeVsSolid();
        test3DWireframeDepthTesting();
        test3DWireframePerspective();
        test2DWorldWireframeVsSolid();
        test2DHudWireframeVsSolid();
        std::cout << "All wireframe tests passed successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Wireframe test failed: " << e.what() << std::endl;
        return 1;
    }
}
