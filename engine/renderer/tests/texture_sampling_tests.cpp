#include "huynity/renderer/software_renderer.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace render = huynity::render_contracts;
namespace image = huynity::image;
namespace renderer = huynity::renderer;

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (!(__VA_ARGS__)) {                                                                                          \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__);    \
        }                                                                                                              \
    } while (false)

constexpr double EPSILON = 1e-5;

[[nodiscard]] bool colorsEqual(const render::Color& a, const render::Color& b, double eps = EPSILON) noexcept {
    return std::abs(a.red - b.red) < eps && std::abs(a.green - b.green) < eps && std::abs(a.blue - b.blue) < eps &&
           std::abs(a.alpha - b.alpha) < eps;
}

image::ImageBuffer createTestTexture2x2() {
    auto created = image::ImageBuffer::create({2, 2});
    if (!created) {
        throw std::runtime_error("failed to create 2x2 test texture");
    }
    auto texture = std::move(*created);
    renderer::detail::writePixel(texture, 0, 0, {1.0, 0.0, 0.0, 1.0}); // Красный (0, 0)
    renderer::detail::writePixel(texture, 1, 0, {0.0, 1.0, 0.0, 1.0}); // Зелёный (1, 0)
    renderer::detail::writePixel(texture, 0, 1, {0.0, 0.0, 1.0, 1.0}); // Синий (0, 1)
    renderer::detail::writePixel(texture, 1, 1, {1.0, 1.0, 0.0, 1.0}); // Жёлтый (1, 1)
    return texture;
}

// Проверка выборки методом ближайшего соседа по четырём углам и внутри квадрантов.
void testSampleTextureNearestCorners() {
    const auto texture = createTestTexture2x2();

    const render::Color red{1.0, 0.0, 0.0, 1.0};
    const render::Color green{0.0, 1.0, 0.0, 1.0};
    const render::Color blue{0.0, 0.0, 1.0, 1.0};
    const render::Color yellow{1.0, 1.0, 0.0, 1.0};

    // Точные угловые координаты
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.0, 0.0), red));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 1.0, 0.0), green));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.0, 1.0), blue));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 1.0, 1.0), yellow));

    // Координаты внутри четырёх квадрантов
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.25, 0.25), red));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.75, 0.25), green));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.25, 0.75), blue));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.75, 0.75), yellow));

    // Проверка через диспетчер sampleTexture
    CHECK(colorsEqual(renderer::sampleTexture(texture, 0.25, 0.25, renderer::TextureFilter::Nearest), red));
    CHECK(colorsEqual(renderer::sampleTexture(texture, 0.75, 0.25, renderer::TextureFilter::Nearest), green));
}

// Проверка билинейной интерполяции: центр (среднее 4 цветов) и рёбра (среднее двух цветов).
void testSampleTextureBilinear() {
    const auto texture = createTestTexture2x2();

    const render::Color red{1.0, 0.0, 0.0, 1.0};
    const render::Color green{0.0, 1.0, 0.0, 1.0};
    const render::Color blue{0.0, 0.0, 1.0, 1.0};
    const render::Color yellow{1.0, 1.0, 0.0, 1.0};

    // Центр (0.5, 0.5): среднее всех 4 цветов
    // r = (1 + 0 + 0 + 1) / 4 = 0.5
    // g = (0 + 1 + 0 + 1) / 4 = 0.5
    // b = (0 + 0 + 1 + 0) / 4 = 0.25
    // a = (1 + 1 + 1 + 1) / 4 = 1.0
    const render::Color expectedCenter{0.5, 0.5, 0.25, 1.0};
    const auto centerColor = renderer::sampleTextureBilinear(texture, 0.5, 0.5);
    CHECK(colorsEqual(centerColor, expectedCenter));

    // Проверка через диспетчер sampleTexture
    const auto centerDispatched = renderer::sampleTexture(texture, 0.5, 0.5, renderer::TextureFilter::Bilinear);
    CHECK(colorsEqual(centerDispatched, expectedCenter));

    // Верхнее ребро (0.5, 0.0): среднее Red и Green
    // r = (1 + 0) / 2 = 0.5, g = (0 + 1) / 2 = 0.5, b = 0.0, a = 1.0
    const render::Color expectedTopEdge{0.5, 0.5, 0.0, 1.0};
    const auto topEdgeColor = renderer::sampleTextureBilinear(texture, 0.5, 0.0);
    CHECK(colorsEqual(topEdgeColor, expectedTopEdge));

    // Левое ребро (0.0, 0.5): среднее Red и Blue
    const render::Color expectedLeftEdge{0.5, 0.0, 0.5, 1.0};
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.0, 0.5), expectedLeftEdge));

    // Правое ребро (1.0, 0.5): среднее Green и Yellow
    const render::Color expectedRightEdge{0.5, 1.0, 0.0, 1.0};
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 1.0, 0.5), expectedRightEdge));

    // Нижнее ребро (0.5, 1.0): среднее Blue и Yellow
    const render::Color expectedBottomEdge{0.5, 0.5, 0.5, 1.0};
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.5, 1.0), expectedBottomEdge));

    // Угловые точки при билинейной интерполяции совпадают с чистыми цветами
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.0, 0.0), red));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 1.0, 0.0), green));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.0, 1.0), blue));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 1.0, 1.0), yellow));
}

// Проверка режимов наложения TextureWrap: Repeat и Clamp для отрицательных и > 1.0 координат.
void testTextureWrapModes() {
    const auto texture = createTestTexture2x2();

    const render::Color red{1.0, 0.0, 0.0, 1.0};
    const render::Color yellow{1.0, 1.0, 0.0, 1.0};
    const render::Color expectedCenter{0.5, 0.5, 0.25, 1.0};
    const render::Color expectedTopEdge{0.5, 0.5, 0.0, 1.0};

    // Repeat: координаты > 1.0
    CHECK(
        colorsEqual(renderer::sampleTextureBilinear(texture, 1.5, 1.5, renderer::TextureWrap::Repeat), expectedCenter));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 2.5, 0.0, renderer::TextureWrap::Repeat),
                      expectedTopEdge));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 2.25, 3.25, renderer::TextureWrap::Repeat), red));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 4.75, 5.75, renderer::TextureWrap::Repeat), yellow));

    // Repeat: отрицательные координаты
    // -0.5 оборачивается в 0.5: -0.5 - floor(-0.5) = -0.5 - (-1.0) = 0.5
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, -0.5, -0.5, renderer::TextureWrap::Repeat),
                      expectedCenter));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, -1.5, 0.0, renderer::TextureWrap::Repeat),
                      expectedTopEdge));
    // -0.75 оборачивается в 0.25: -0.75 - (-1.0) = 0.25
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, -0.75, -0.75, renderer::TextureWrap::Repeat), red));
    // -0.25 оборачивается в 0.75: -0.25 - (-1.0) = 0.75
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, -0.25, -0.25, renderer::TextureWrap::Repeat), yellow));

    // Clamp: отрицательные координаты зажимаются в 0.0 (Red), > 1.0 зажимаются в 1.0 (Yellow)
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, -2.0, -3.0, renderer::TextureWrap::Clamp), red));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 5.0, 7.0, renderer::TextureWrap::Clamp), yellow));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, -1.0, -1.0, renderer::TextureWrap::Clamp), red));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 2.0, 2.0, renderer::TextureWrap::Clamp), yellow));
}

// Проверка пустой текстуры и некорректных координат (NaN, Inf).
void testEmptyTextureAndInvalidCoords() {
    auto valid = createTestTexture2x2();
    auto moved = std::move(valid);

    const render::Color zeroColor{0.0, 0.0, 0.0, 0.0};

    // Выборка из перемещённой (пустой) текстуры
    CHECK(colorsEqual(renderer::sampleTextureNearest(valid, 0.5, 0.5), zeroColor));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(valid, 0.5, 0.5), zeroColor));
    CHECK(colorsEqual(renderer::sampleTexture(valid, 0.5, 0.5), zeroColor));

    // Проверка нечисловых координат (NaN, Inf) на валидной текстуре
    const auto texture = createTestTexture2x2();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, nan, 0.5), zeroColor));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.5, nan), zeroColor));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, nan, 0.5), zeroColor));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, inf, 0.5), zeroColor));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.5, -inf), zeroColor));
}

// Проверка текстуры размером 1x1.
void testSinglePixelTexture() {
    auto created = image::ImageBuffer::create({1, 1});
    if (!created) {
        throw std::runtime_error("failed to create 1x1 test texture");
    }
    auto texture = std::move(*created);
    const render::Color magenta{1.0, 0.0, 1.0, 1.0};
    renderer::detail::writePixel(texture, 0, 0, magenta);

    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.0, 0.0), magenta));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 0.5, 0.5), magenta));
    CHECK(colorsEqual(renderer::sampleTextureNearest(texture, 1.0, 1.0), magenta));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.0, 0.0), magenta));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 0.5, 0.5), magenta));
    CHECK(colorsEqual(renderer::sampleTextureBilinear(texture, 1.0, 1.0), magenta));
}

int main() {
    try {
        testSampleTextureNearestCorners();
        testSampleTextureBilinear();
        testTextureWrapModes();
        testEmptyTextureAndInvalidCoords();
        testSinglePixelTexture();
        std::cout << "All texture sampling tests passed successfully." << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << std::endl;
        return 1;
    }
}
