#include "huynity/renderer/software_renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
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

inline std::ostream& operator<<(std::ostream& stream, const Rgba& color) {
    return stream << "Rgba(" << color.red << ", " << color.green << ", " << color.blue << ", " << color.alpha << ")";
}

constexpr Rgba BLACK{0, 0, 0, 255};
constexpr Rgba WHITE{255, 255, 255, 255};
constexpr Rgba RED{255, 0, 0, 255};
constexpr Rgba GREEN{0, 255, 0, 255};
constexpr Rgba BLUE{0, 0, 255, 255};
constexpr Rgba YELLOW{255, 255, 0, 255};

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

render::Color color(double red, double green, double blue, double alpha = 1.0) {
    return {red, green, blue, alpha};
}

bool colorsClose(const Rgba& first, const Rgba& second, int tolerance = 2) {
    return std::abs(first.red - second.red) <= tolerance && std::abs(first.green - second.green) <= tolerance &&
           std::abs(first.blue - second.blue) <= tolerance && std::abs(first.alpha - second.alpha) <= tolerance;
}

/// @brief Создаёт 2x2 текстуру с четырьмя контрастными цветами в квадрантах:
/// (0, 0) - красный, (1, 0) - зелёный, (0, 1) - синий, (1, 1) - жёлтый.
std::shared_ptr<const image::ImageBuffer> makeQuadrantTexture() {
    auto buffer = makeFramebuffer(2, 2);
    renderer::detail::writePixel(buffer, 0, 0, color(1, 0, 0, 1));
    renderer::detail::writePixel(buffer, 1, 0, color(0, 1, 0, 1));
    renderer::detail::writePixel(buffer, 0, 1, color(0, 0, 1, 1));
    renderer::detail::writePixel(buffer, 1, 1, color(1, 1, 0, 1));
    return std::make_shared<const image::ImageBuffer>(std::move(buffer));
}

/// @brief Создаёт 2x2 текстуру спрайта с прозрачными и полупрозрачными пикселями:
/// (0, 0) - прозрачный (alpha = 0), (1, 0) - непрозрачный красный,
/// (0, 1) - полупрозрачный зелёный (alpha = 0.5), (1, 1) - полупрозрачный белый (alpha = 0.5).
std::shared_ptr<const image::ImageBuffer> makeTransparentSpriteTexture() {
    auto buffer = makeFramebuffer(2, 2);
    renderer::detail::writePixel(buffer, 0, 0, color(0, 0, 0, 0));
    renderer::detail::writePixel(buffer, 1, 0, color(1, 0, 0, 1));
    renderer::detail::writePixel(buffer, 0, 1, color(0, 1, 0, 0.5));
    renderer::detail::writePixel(buffer, 1, 1, color(1, 1, 1, 0.5));
    return std::make_shared<const image::ImageBuffer>(std::move(buffer));
}

struct TestMesh2 {
    std::vector<math::Point2> positions;
    std::vector<render::TriangleIndices> triangles;
    std::vector<math::Point2> texCoords;
    std::shared_ptr<const void> owner;
};

struct TestMesh3 {
    std::vector<math::Point3> positions;
    std::vector<render::TriangleIndices> triangles;
    std::vector<math::Point2> texCoords;
    std::shared_ptr<const void> owner;
};

/// @brief Создаёт 2D-прямоугольник из 2 треугольников в координатах [minX, minY] .. [maxX, maxY]
/// с UV-развёрткой от (0, 0) до (1, 1).
TestMesh2 makeQuad2D(double minX, double minY, double maxX, double maxY) {
    TestMesh2 mesh;
    mesh.positions = {{minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}};
    mesh.triangles = {{0, 1, 2}, {0, 2, 3}};
    mesh.texCoords = {{{0, 0}}, {{1, 0}}, {{1, 1}}, {{0, 1}}};
    return mesh;
}

void addResource2(render::RenderSnapshot& snapshot, const TestMesh2& mesh) {
    render::MeshResource2 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});
    resource.texCoords = mesh.texCoords;
    snapshot.resources2.push_back(resource);
}

void addResource3(render::RenderSnapshot& snapshot, const TestMesh3& mesh) {
    render::MeshResource3 resource;
    resource.owner = mesh.owner;
    resource.positions = mesh.positions;
    resource.triangles = std::as_bytes(std::span{mesh.triangles});
    resource.texCoords = mesh.texCoords;
    snapshot.resources3.push_back(resource);
}

/// @brief Проверяет наложение текстуры на 2D-прямоугольник через Hud2DPass и SoftwareRenderer.
void testTextured2DQuadMapping() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;
    auto framebuffer = makeFramebuffer(width, height);

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    snapshot.clearColor = color(0, 0, 0, 1);

    auto texture = makeQuadrantTexture();
    auto mat = std::make_shared<render::Material>();
    mat->color = color(1, 1, 1, 1);
    mat->texture = texture;
    mat->filter = render::TextureFilter::Nearest;
    mat->wrap = render::TextureWrap::Clamp;
    snapshot.materials.push_back(mat);

    const auto quad = makeQuad2D(0, 0, 64, 64);
    addResource2(snapshot, quad);

    render::Hud2DPass hud;
    hud.drawItems.push_back({.resource = 0, .material = 0});
    snapshot.passes.emplace_back(hud);

    renderer::SoftwareRenderer sr;
    const auto result = sr.render(snapshot, framebuffer);
    CHECK(result.has_value());

    // Проверяем квадранты
    CHECK(pixel(framebuffer, 16, 16) == RED);
    CHECK(pixel(framebuffer, 48, 16) == GREEN);
    CHECK(pixel(framebuffer, 16, 48) == BLUE);
    CHECK(pixel(framebuffer, 48, 48) == YELLOW);
}

/// @brief Проверяет альфа-прозрачность 2D-спрайта: пиксели с alpha = 0 сохраняют фоновый цвет,
/// непрозрачные заменяют фон, а полупрозрачные корректно смешиваются (source-over).
void testSpriteTransparencyAndAlphaBlending() {
    constexpr std::size_t width = 40;
    constexpr std::size_t height = 40;
    auto framebuffer = makeFramebuffer(width, height);

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    // Фоновый цвет — синий
    snapshot.clearColor = color(0, 0, 1, 1);

    auto spriteTexture = makeTransparentSpriteTexture();
    auto mat = std::make_shared<render::Material>();
    mat->color = color(1, 1, 1, 1);
    mat->texture = spriteTexture;
    mat->filter = render::TextureFilter::Nearest;
    mat->wrap = render::TextureWrap::Clamp;
    snapshot.materials.push_back(mat);

    const auto quad = makeQuad2D(0, 0, 40, 40);
    addResource2(snapshot, quad);

    render::Hud2DPass hud;
    hud.drawItems.push_back({.resource = 0, .material = 0});
    snapshot.passes.emplace_back(hud);

    renderer::SoftwareRenderer sr;
    const auto result = sr.render(snapshot, framebuffer);
    CHECK(result.has_value());

    // (10, 10) соответствует пикселю текстуры (0, 0) с alpha = 0: фоновый синий должен полностью сохраниться!
    const auto transparentArea = pixel(framebuffer, 10, 10);
    CHECK(transparentArea == BLUE);

    // (30, 10) соответствует пикселю текстуры (1, 0) с непрозрачным красным: фон полностью заменяется на красный!
    const auto opaqueArea = pixel(framebuffer, 30, 10);
    CHECK(opaqueArea == RED);

    // (10, 30) соответствует пикселю текстуры (0, 1) с полупрозрачным зелёным (alpha = 0.5) поверх синего:
    // source: (0, 1, 0, 0.5), destination: (0, 0, 1, 1)
    // Red: 0 * 0.5 + 0 * 0.5 = 0
    // Green: 255 * 0.5 + 0 * 0.5 = 128 (допуск ±1)
    // Blue: 0 * 0.5 + 255 * 0.5 = 128 (допуск ±1)
    // Alpha: 255
    const auto blendedGreen = pixel(framebuffer, 10, 30);
    CHECK(colorsClose(blendedGreen, Rgba{0, 128, 128, 255}, 2));

    // (30, 30) соответствует пикселю текстуры (1, 1) с полупрозрачным белым (alpha = 0.5) поверх синего:
    // Red: 255 * 0.5 + 0 * 0.5 = 128
    // Green: 255 * 0.5 + 0 * 0.5 = 128
    // Blue: 255 * 0.5 + 255 * 0.5 = 255
    // (35, 25) соответствует пикселю текстуры (1, 1) внутри первого треугольника (вдали от диагонали):
    // с полупрозрачным белым (alpha = 0.5) поверх синего:
    // Red: 255 * 0.5 + 0 * 0.5 = 128
    // Green: 255 * 0.5 + 0 * 0.5 = 128
    // Blue: 255 * 0.5 + 255 * 0.5 = 255
    const auto blendedWhite = pixel(framebuffer, 35, 25);
    CHECK(colorsClose(blendedWhite, Rgba{128, 128, 255, 255}, 2));
}

/// @brief Проверяет модуляцию цвета материала при 2D-растеризации спрайта (tint color).
void testSpriteMaterialTintModulation() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;
    auto framebuffer = makeFramebuffer(width, height);

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    snapshot.clearColor = color(0, 0, 0, 1);

    // Белая текстура
    auto whiteTexBuffer = makeFramebuffer(2, 2);
    renderer::detail::clear(whiteTexBuffer, color(1, 1, 1, 1));
    auto whiteTexture = std::make_shared<const image::ImageBuffer>(std::move(whiteTexBuffer));

    // Материал с полупрозрачным красным оттенком: {1.0, 0.0, 0.0, 0.5}
    auto mat = std::make_shared<render::Material>();
    mat->color = color(1, 0, 0, 0.5);
    mat->texture = whiteTexture;
    snapshot.materials.push_back(mat);

    const auto quad = makeQuad2D(0, 0, 32, 32);
    addResource2(snapshot, quad);

    render::Hud2DPass hud;
    hud.drawItems.push_back({.resource = 0, .material = 0});
    snapshot.passes.emplace_back(hud);

    renderer::SoftwareRenderer sr;
    const auto result = sr.render(snapshot, framebuffer);
    CHECK(result.has_value());

    // Белая текстура промодулирована цветом {1, 0, 0, 0.5} поверх чёрного фона {0, 0, 0, 1}:
    // Red: 255 * 0.5 + 0 * 0.5 = 128
    const auto nonDiagonalPixel = pixel(framebuffer, 20, 12);
    CHECK(colorsClose(nonDiagonalPixel, Rgba{128, 0, 0, 255}, 2));
}

/// @brief Проверяет перспективно-корректную выборку текстуры в 3D.
///
/// В перспективе текстурные координаты масштабируются с глубиной 1/z.
/// Для треугольника с вершинами на глубинах z = 1.0 и z = 3.0 значение UV на экранной середине
/// существенно отличается от аффинного (0.25 против 0.5).
void testTextured3DPerspectiveCorrectness() {
    constexpr std::size_t width = 100;
    constexpr std::size_t height = 100;
    auto framebuffer = makeFramebuffer(width, height);

    // Создаём 2-пиксельную по горизонтали текстуру:
    // u in [0, 0.5) -> КРАСНЫЙ, u in [0.5, 1.0] -> ЗЕЛЁНЫЙ
    auto texBuffer = makeFramebuffer(2, 1);
    renderer::detail::writePixel(texBuffer, 0, 0, color(1, 0, 0, 1));
    renderer::detail::writePixel(texBuffer, 1, 0, color(0, 1, 0, 1));
    auto texture = std::make_shared<const image::ImageBuffer>(std::move(texBuffer));

    // Треугольник в плоскости view:
    // v0: z = 1.0, ndcX = -0.5, uv = (0, 0)
    // v1: z = 3.0, ndcX = 0.5, uv = (1, 0)
    // v2: z = 2.0, ndcX = 0.0, uv = (0.5, 1)
    // Непосредственно проверяем helper rasterizeTriangleTextured3D
    renderer::detail::clear(framebuffer, color(0, 0, 0, 1));
    std::vector<double> depth(width * height, std::numeric_limits<double>::infinity());

    // Вершина 0: z0 = 1.0 => invZ = 1.0, screenX ~ 25
    const renderer::detail::ProjectedVertex v0{-0.5, 0.5, 1.0, 1.0};
    // Вершина 1: z1 = 3.0 => invZ = 1.0 / 3.0, screenX ~ 75
    const renderer::detail::ProjectedVertex v1{0.5, 0.5, 1.0 / 3.0, 1.0};
    // Вершина 2: z2 = 2.0 => invZ = 0.5, screenX ~ 50, screenY ~ 75
    const renderer::detail::ProjectedVertex v2{0.0, -0.5, 0.5, 1.0};

    const math::Point2 uv0{0.0, 0.0};
    const math::Point2 uv1{1.0, 0.0};
    const math::Point2 uv2{0.5, 1.0};

    renderer::rasterizeTriangleTextured3D(v0, v1, v2, uv0, uv1, uv2, *texture, color(1, 1, 1, 1), framebuffer, depth,
                                          renderer::TextureFilter::Nearest, renderer::TextureWrap::Clamp);

    // На экранной горизонтальной середине между v0 и v1 (x = 50, y = 26):
    // При аффинной интерполяции u было бы 0.5 (зелёный).
    // При перспективно-корректной интерполяции:
    // w0 = 0.5, w1 = 0.5
    // invZ = 0.5 * 1.0 + 0.5 * (1/3) = 2/3
    // u/z = 0.5 * 0 + 0.5 * (1 / 3) = 1/6
    // u = (1/6) / (2/3) = 0.25!
    // Так как u = 0.25 < 0.5, выборка Nearest ДОЛЖНА дать КРАСНЫЙ пиксель!
    const auto midPixel = pixel(framebuffer, 50, 26);
    CHECK(midPixel == RED);
}

/// @brief Проверяет модуляцию текстуры освещением в World3DPass.
void testTextured3DLightingModulation() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;
    auto framebuffer = makeFramebuffer(width, height);

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    snapshot.clearColor = color(0, 0, 0, 1);

    // Белая текстура
    auto whiteTexBuffer = makeFramebuffer(2, 2);
    renderer::detail::clear(whiteTexBuffer, color(1, 1, 1, 1));
    auto whiteTexture = std::make_shared<const image::ImageBuffer>(std::move(whiteTexBuffer));

    // Белый материал с текстурой
    auto mat = std::make_shared<render::Material>();
    mat->color = color(1, 1, 1, 1);
    mat->texture = whiteTexture;
    snapshot.materials.push_back(mat);

    // Лицевой треугольник на z = 2.0 (нормаль в -z, по часовой стрелке 0 -> 1 -> 2)
    TestMesh3 mesh;
    mesh.positions = {{-1.0, -1.0, 2.0}, {0.0, 1.0, 2.0}, {1.0, -1.0, 2.0}};
    mesh.triangles = {{0, 1, 2}};
    mesh.texCoords = {{{0.0, 0.0}}, {{0.5, 1.0}}, {{1.0, 0.0}}};
    addResource3(snapshot, mesh);

    render::World3DPass pass;
    pass.camera = render::OrthographicCamera3{
        .worldToView = {},
        .width = 4.0,
        .height = 4.0,
        .nearPlane = 0.1,
        .farPlane = 10.0,
    };
    // Ambient light интенсивностью 0.6 с цветом {0.5, 0.8, 1.0}
    pass.lighting.ambient = {color(0.5, 0.8, 1.0, 1.0), 0.6};
    pass.drawItems.push_back({.resource = 0, .material = 0, .backfaceCulling = false});
    snapshot.passes.emplace_back(pass);

    renderer::SoftwareRenderer sr;
    const auto result = sr.render(snapshot, framebuffer);
    CHECK(result.has_value());

    // Центр треугольника должен быть освещён ambient светом:
    // Red: 0.5 * 0.6 * 255 = 76.5 (~77)
    // Green: 0.8 * 0.6 * 255 = 122.4 (~122)
    // Blue: 1.0 * 0.6 * 255 = 153.0 (~153)
    const auto center = pixel(framebuffer, 32, 34);
    CHECK(colorsClose(center, Rgba{77, 122, 153, 255}, 2));
}

/// @brief Проверяет перекрытие по Z-буферу для текстурированных 3D-объектов.
void testTextured3DDepthOcclusion() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    // Текстура 1: полностью красная
    auto redTexBuffer = makeFramebuffer(2, 2);
    renderer::detail::clear(redTexBuffer, color(1, 0, 0, 1));
    auto redTexture = std::make_shared<const image::ImageBuffer>(std::move(redTexBuffer));

    // Текстура 2: полностью зелёная
    auto greenTexBuffer = makeFramebuffer(2, 2);
    renderer::detail::clear(greenTexBuffer, color(0, 1, 0, 1));
    auto greenTexture = std::make_shared<const image::ImageBuffer>(std::move(greenTexBuffer));

    // Дальний треугольник (красный, z = 3.0)
    TestMesh3 farMesh;
    farMesh.positions = {{-2.0, -2.0, 3.0}, {0.0, 2.0, 3.0}, {2.0, -2.0, 3.0}};
    farMesh.triangles = {{0, 1, 2}};
    farMesh.texCoords = {{{0.0, 0.0}}, {{0.5, 1.0}}, {{1.0, 0.0}}};

    // Ближний треугольник (зелёный, z = 1.5), закрывающий правую часть
    TestMesh3 nearMesh;
    nearMesh.positions = {{0.0, -2.0, 1.5}, {0.0, 2.0, 1.5}, {2.0, -2.0, 1.5}};
    nearMesh.triangles = {{0, 1, 2}};
    nearMesh.texCoords = {{{0.0, 0.0}}, {{0.0, 1.0}}, {{1.0, 0.0}}};

    const auto renderWithOrder = [&](bool farFirst) -> image::ImageBuffer {
        auto fb = makeFramebuffer(width, height);
        render::RenderSnapshot snapshot;
        snapshot.imageWidth = width;
        snapshot.imageHeight = height;
        snapshot.clearColor = color(0, 0, 0, 1);

        auto matFar = std::make_shared<render::Material>();
        matFar->color = color(1, 1, 1, 1);
        matFar->texture = redTexture;

        auto matNear = std::make_shared<render::Material>();
        matNear->color = color(1, 1, 1, 1);
        matNear->texture = greenTexture;

        snapshot.materials.push_back(matFar);  // 0
        snapshot.materials.push_back(matNear); // 1

        addResource3(snapshot, farMesh);  // 0
        addResource3(snapshot, nearMesh); // 1

        render::World3DPass pass;
        pass.camera = render::OrthographicCamera3{
            .worldToView = {},
            .width = 4.0,
            .height = 4.0,
            .nearPlane = 0.1,
            .farPlane = 10.0,
        };
        pass.lighting.ambient = {color(1, 1, 1, 1), 1.0};

        if (farFirst) {
            pass.drawItems.push_back({.resource = 0, .material = 0, .backfaceCulling = false});
            pass.drawItems.push_back({.resource = 1, .material = 1, .backfaceCulling = false});
        } else {
            pass.drawItems.push_back({.resource = 1, .material = 1, .backfaceCulling = false});
            pass.drawItems.push_back({.resource = 0, .material = 0, .backfaceCulling = false});
        }
        snapshot.passes.emplace_back(pass);

        renderer::SoftwareRenderer sr;
        const auto result = sr.render(snapshot, fb);
        CHECK(result.has_value());
        return fb;
    };

    const auto fbFarFirst = renderWithOrder(true);
    const auto fbNearFirst = renderWithOrder(false);

    // Левая часть (x = 24, y = 34) должна быть красной (виден дальний треугольник)
    CHECK(pixel(fbFarFirst, 24, 34) == RED);
    CHECK(pixel(fbNearFirst, 24, 34) == RED);

    // Правая часть (x = 40, y = 34) должна быть зелёной (ближний треугольник перекрывает дальний)
    CHECK(pixel(fbFarFirst, 40, 34) == GREEN);
    CHECK(pixel(fbNearFirst, 40, 34) == GREEN);
}

/// @brief Проверяет корректный откат (fallback) к сплошной заливке, если текстура не задана
/// или количество texCoords не совпадает с количеством позиций.
void testTexturedFallback() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;
    auto framebuffer = makeFramebuffer(width, height);

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    snapshot.clearColor = color(0, 0, 0, 1);

    auto mat = std::make_shared<render::Material>();
    mat->color = color(1, 0, 0, 1);
    mat->texture = nullptr; // Текстуры нет
    snapshot.materials.push_back(mat);

    auto quad = makeQuad2D(0, 0, 32, 32);
    // Пустой texCoords
    quad.texCoords.clear();
    addResource2(snapshot, quad);

    render::Hud2DPass hud;
    hud.drawItems.push_back({.resource = 0, .material = 0});
    snapshot.passes.emplace_back(hud);

    renderer::SoftwareRenderer sr;
    const auto result = sr.render(snapshot, framebuffer);
    CHECK(result.has_value());

    // Должна отработать стандартная заливка цветом материала
    CHECK(pixel(framebuffer, 16, 16) == RED);
}

int main() {
    try {
        testTextured2DQuadMapping();
        testSpriteTransparencyAndAlphaBlending();
        testSpriteMaterialTintModulation();
        testTextured3DPerspectiveCorrectness();
        testTextured3DLightingModulation();
        testTextured3DDepthOcclusion();
        testTexturedFallback();
        std::cout << "textured_rasterization_tests: ALL PASSED\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "textured_rasterization_tests ERROR: " << error.what() << '\n';
        return 1;
    }
}
