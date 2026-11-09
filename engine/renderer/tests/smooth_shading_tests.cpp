#include "huynity/renderer/software_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
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

struct TestMesh {
    std::vector<math::Point3> positions;
    std::vector<render::TriangleIndices> triangles;
    std::shared_ptr<const void> owner = std::make_shared<const int>(0);
};

render::Color color(double red, double green, double blue, double alpha = 1.0) {
    return {red, green, blue, alpha};
}

std::shared_ptr<const render::Material> material(double red, double green, double blue) {
    return std::make_shared<const render::Material>(color(red, green, blue));
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

/// @brief Выпуклая поверхность из двух треугольников, стыкующихся по ребру X = 0.
///
/// V0 = (0, -1.5, 10), V1 = (0, 1.5, 10) образуют вертикальное общее ребро.
/// Левый треугольник отогнут назад (Z = 10.5) в сторону -X.
/// Правый треугольник отогнут назад (Z = 10.5) в сторону +X.
/// Обе грани лицевые (нормали смотрят в сторону камеры, -Z).
TestMesh makeConvexCreaseMesh() {
    TestMesh mesh;
    mesh.positions = {
        {0.0, -1.5, 10.0}, // 0: низ общего ребра
        {0.0, 1.5, 10.0},  // 1: верх общего ребра
        {-1.5, 0.0, 10.5}, // 2: левая вершина
        {1.5, 0.0, 10.5},  // 3: правая вершина
    };
    mesh.triangles = {
        {0, 2, 1}, // Левый треугольник
        {0, 1, 3}, // Правый треугольник
    };
    return mesh;
}

/// @brief Создаёт параметрическую UV-сетку сферы с общими вершинами на стыках граней.
TestMesh makeSphereMesh(double radius, std::size_t latSegments, std::size_t lonSegments) {
    TestMesh mesh;
    // Северный полюс
    mesh.positions.push_back({0.0, radius, 10.0});
    for (std::size_t i = 1; i < latSegments; ++i) {
        const double theta = std::numbers::pi * static_cast<double>(i) / static_cast<double>(latSegments);
        const double sinTheta = std::sin(theta);
        const double cosTheta = std::cos(theta);
        for (std::size_t j = 0; j < lonSegments; ++j) {
            const double phi = 2.0 * std::numbers::pi * static_cast<double>(j) / static_cast<double>(lonSegments);
            const double x = radius * sinTheta * std::cos(phi);
            const double y = radius * cosTheta;
            const double z = 10.0 + radius * sinTheta * std::sin(phi);
            mesh.positions.push_back({x, y, z});
        }
    }
    // Южный полюс
    const auto southPole = static_cast<std::uint32_t>(mesh.positions.size());
    mesh.positions.push_back({0.0, -radius, 10.0});

    // Верхняя шапка (вокруг северного полюса)
    for (std::size_t j = 0; j < lonSegments; ++j) {
        const auto next = static_cast<std::uint32_t>((j + 1) % lonSegments);
        mesh.triangles.push_back({0, 1 + next, 1 + static_cast<std::uint32_t>(j)});
    }

    // Промежуточные пояса
    for (std::size_t i = 0; i < latSegments - 2; ++i) {
        const auto ringStart = static_cast<std::uint32_t>(1 + i * lonSegments);
        const auto nextRingStart = static_cast<std::uint32_t>(1 + (i + 1) * lonSegments);
        for (std::size_t j = 0; j < lonSegments; ++j) {
            const auto next = static_cast<std::uint32_t>((j + 1) % lonSegments);
            const auto current = static_cast<std::uint32_t>(j);
            mesh.triangles.push_back({ringStart + current, nextRingStart + next, nextRingStart + current});
            mesh.triangles.push_back({ringStart + current, ringStart + next, nextRingStart + next});
        }
    }

    // Нижняя шапка (вокруг южного полюса)
    const auto lastRingStart = static_cast<std::uint32_t>(1 + (latSegments - 2) * lonSegments);
    for (std::size_t j = 0; j < lonSegments; ++j) {
        const auto next = static_cast<std::uint32_t>((j + 1) % lonSegments);
        mesh.triangles.push_back({southPole, lastRingStart + static_cast<std::uint32_t>(j), lastRingStart + next});
    }

    return mesh;
}

/// @brief Проверка расчёта и кеширования вершинных нормалей в ShadingDataCache.
void testVertexNormalsCalculationAndCaching() {
    renderer::ShadingDataCache cache;
    auto mesh = makeSphereMesh(1.0, 12, 24);

    render::MeshResource3 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});

    // 1. Первый вызов вычисляет нормали (miss)
    auto normalsResult = cache.vertexNormals(resource);
    CHECK(normalsResult.has_value());
    CHECK(cache.misses() == 1);
    CHECK(cache.hits() == 0);

    const auto* normals = *normalsResult;
    CHECK(normals->size() == mesh.positions.size());

    // Все нормали нормализованы и для сферы направлены радиально наружу от центра (0, 0, 10)
    for (std::size_t i = 0; i < normals->size(); ++i) {
        const auto& n = (*normals)[i];
        const double len = math::length(n);
        CHECK(std::abs(len - 1.0) < 1e-4);

        const auto& p = mesh.positions[i];
        const math::Vector3 radial{p[0], p[1], p[2] - 10.0};
        const double radLen = math::length(radial);
        if (radLen > 1e-4) {
            const auto expectedDir = radial * (1.0 / radLen);
            CHECK(math::dot(n, expectedDir) > 0.9);
        }
    }

    // 2. Повторный вызов возвращает кешированные значения (hit)
    auto cachedResult = cache.vertexNormals(resource);
    CHECK(cachedResult.has_value());
    CHECK(cache.misses() == 1);
    CHECK(cache.hits() == 1);
    CHECK(*cachedResult == normals);

    // 3. Изменение ресурса приводит к повторному расчёту (miss)
    auto newMesh = makeConvexCreaseMesh();
    render::MeshResource3 newResource;
    newResource.owner = newMesh.owner;
    newResource.positions = newMesh.positions;
    newResource.triangles = std::as_bytes(std::span{newMesh.triangles});

    auto creaseNormals = cache.vertexNormals(newResource);
    CHECK(creaseNormals.has_value());
    CHECK(cache.misses() == 2);
    CHECK(cache.hits() == 1);

    // В выпуклом ребре нормали вершин на стыке (0 и 1) строго направлены в -Z
    const auto& vn = **creaseNormals;
    CHECK(std::abs(vn[0][0]) < 1e-4);
    CHECK(std::abs(vn[0][1]) < 1e-4);
    CHECK(vn[0][2] < -0.99);

    CHECK(std::abs(vn[1][0]) < 1e-4);
    CHECK(std::abs(vn[1][1]) < 1e-4);
    CHECK(vn[1][2] < -0.99);
}

/// @brief Проверка скачка Flat vs непрерывности Gouraud на стыке граней.
void testGradientContinuityAcrossSharedEdge() {
    auto mesh = makeConvexCreaseMesh();

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = 64;
    snapshot.imageHeight = 64;
    snapshot.clearColor = color(0, 0, 0);
    snapshot.materials = {material(1, 1, 1)}; // Белый материал

    render::MeshResource3 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});
    snapshot.resources3.push_back(resource);

    render::World3DPass pass;
    pass.camera = render::OrthographicCamera3{math::Transform3{}, 4.0, 4.0, 0.1, 100.0};
    // Свет падает под углом слева-спереди: towardLight = normalized({-1.0, 0, -0.5})
    pass.lighting.ambient = {color(1, 1, 1), 0.0};
    pass.lighting.directional = {{math::Vector3{1.0, 0.0, 0.5}, color(1, 1, 1), 1.0}};

    // 1. Рендерим с ShadingModel::Flat
    {
        render::DrawItem3 item;
        item.resource = 0;
        item.material = 0;
        item.backfaceCulling = true;
        item.shading = render::ShadingModel::Flat;

        auto flatSnapshot = snapshot;
        pass.drawItems = {item};
        flatSnapshot.passes = {pass};

        auto flatFb = makeFramebuffer(64, 64);
        CHECK(renderer::render(flatSnapshot, flatFb).has_value());

        // Пиксель (31, 32) лежит в левом треугольнике, (32, 32) лежит в правом треугольнике.
        // Во Flat shading нормали граней разные, поэтому освещённость соседних пикселей
        // на стыке испытывает резкий скачок.
        const auto leftPixel = pixel(flatFb, 31, 32);
        const auto rightPixel = pixel(flatFb, 32, 32);

        const int flatJump = std::abs(leftPixel.red - rightPixel.red);
        CHECK(flatJump >= 100);

        // Весь левый треугольник вдоль горизонтали x in [16, 31] имеет константную яркость
        for (std::size_t x = 16; x < 31; ++x) {
            CHECK(pixel(flatFb, x, 32).red == leftPixel.red);
        }
        // Весь правый треугольник вдоль горизонтали x in [32, 47] имеет константную яркость
        for (std::size_t x = 32; x < 47; ++x) {
            CHECK(pixel(flatFb, x, 32).red == rightPixel.red);
        }
    }

    // 2. Рендерим с ShadingModel::Gouraud
    {
        render::DrawItem3 item;
        item.resource = 0;
        item.material = 0;
        item.backfaceCulling = true;
        item.shading = render::ShadingModel::Gouraud;

        auto gouraudSnapshot = snapshot;
        pass.drawItems = {item};
        gouraudSnapshot.passes = {pass};

        auto gouraudFb = makeFramebuffer(64, 64);
        CHECK(renderer::render(gouraudSnapshot, gouraudFb).has_value());

        // В Gouraud shading цвета на общем ребре V0-V1 интерполируются из одинаковых
        // усреднённых вершинных нормалей, поэтому скачка на стыке (31 -> 32) нет.
        const auto leftPixel = pixel(gouraudFb, 31, 32);
        const auto rightPixel = pixel(gouraudFb, 32, 32);

        const int edgeDifference = std::abs(leftPixel.red - rightPixel.red);
        CHECK(edgeDifference <= 10); // Разница между соседними пикселями минимальна!

        // Градиент освещённости непрерывен и плавен по всей горизонтали от левой вершины к правой
        for (std::size_t x = 16; x < 47; ++x) {
            const auto current = pixel(gouraudFb, x, 32);
            const auto next = pixel(gouraudFb, x + 1, 32);
            const int step = std::abs(current.red - next.red);
            // Ни на одном шаге нет резкого скачка: каждый шаг плавный (<= 15)
            CHECK(step <= 15);
        }
    }
}

/// @brief Проверка сглаживания на сфере и кеширования между кадрами SoftwareRenderer.
void testSphereSmoothShadingAndRendererCache() {
    auto mesh = makeSphereMesh(1.2, 16, 32);

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = 64;
    snapshot.imageHeight = 64;
    snapshot.clearColor = color(0, 0, 0);
    snapshot.materials = {material(0.8, 0.8, 0.8)};

    render::MeshResource3 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});
    snapshot.resources3.push_back(resource);

    render::World3DPass pass;
    pass.camera = render::OrthographicCamera3{math::Transform3{}, 4.0, 4.0, 0.1, 100.0};
    pass.lighting.ambient = {color(1, 1, 1), 0.1};
    pass.lighting.directional = {{math::Vector3{0.0, 0.0, 1.0}, color(1, 1, 1), 0.9}};

    render::DrawItem3 item;
    item.resource = 0;
    item.material = 0;
    item.backfaceCulling = true;
    item.shading = render::ShadingModel::Gouraud;
    pass.drawItems = {item};
    snapshot.passes = {pass};

    renderer::SoftwareRenderer renderer;

    // Кадр 1: рендеринг сферы с вычислением вершинных нормалей (кеш miss)
    auto fb1 = makeFramebuffer(64, 64);
    CHECK(renderer.render(snapshot, fb1).has_value());
    CHECK(renderer.shadingCacheMisses() == 1);
    CHECK(renderer.shadingCacheHits() == 0);

    // Центр сферы (направлен прямо на источник света) ярко освещён
    const auto center = pixel(fb1, 32, 32);
    CHECK(center.red >= 200);
    CHECK(center.green >= 200);
    CHECK(center.blue >= 200);

    // Кадр 2: повторный рендеринг переиспользует кешированные нормали (кеш hit)
    auto fb2 = makeFramebuffer(64, 64);
    CHECK(renderer.render(snapshot, fb2).has_value());
    CHECK(renderer.shadingCacheMisses() == 1);
    CHECK(renderer.shadingCacheHits() == 1);

    // Содержимое обоих кадров попиксельно идентично
    const auto bytes1 = fb1.bytes();
    const auto bytes2 = fb2.bytes();
    CHECK(bytes1.size() == bytes2.size());
    CHECK(std::equal(bytes1.begin(), bytes1.end(), bytes2.begin()));
}

int main() {
    try {
        testVertexNormalsCalculationAndCaching();
        testGradientContinuityAcrossSharedEdge();
        testSphereSmoothShadingAndRendererCache();
        std::cout << "All smooth shading tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }
}
