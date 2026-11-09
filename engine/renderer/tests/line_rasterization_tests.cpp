#include "huynity/renderer/software_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace render = huynity::render_contracts;
namespace image = huynity::image;
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

struct Point2i {
    int x;
    int y;

    friend auto operator<=>(const Point2i&, const Point2i&) = default;
};

std::vector<Point2i> collectColoredPixels(const image::ImageBuffer& framebuffer, const Rgba& expected) {
    std::vector<Point2i> result;
    for (std::size_t y = 0; y < framebuffer.height(); ++y) {
        for (std::size_t x = 0; x < framebuffer.width(); ++x) {
            if (pixel(framebuffer, x, y) == expected) {
                result.push_back({static_cast<int>(x), static_cast<int>(y)});
            }
        }
    }
    return result;
}

void verifyLineContinuity(int x0, int y0, int x1, int y1, const std::vector<Point2i>& pixels) {
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const std::size_t expectedCount = static_cast<std::size_t>(std::max(dx, dy) + 1);
    CHECK(pixels.size() == expectedCount);

    const auto containsPoint = [&](int x, int y) {
        return std::ranges::any_of(pixels, [&](const Point2i& pt) { return pt.x == x && pt.y == y; });
    };

    CHECK(containsPoint(x0, y0));
    CHECK(containsPoint(x1, y1));

    if (dx >= dy) {
        // x — доминирующая ось: для каждого x от min(x0, x1) до max(x0, x1) ровно один пиксель
        const int minX = std::min(x0, x1);
        const int maxX = std::max(x0, x1);
        std::map<int, int> xToY;
        for (const auto& pt : pixels) {
            CHECK(!xToY.contains(pt.x)); // ровно один пиксель на столбец
            xToY[pt.x] = pt.y;
        }
        for (int x = minX; x <= maxX; ++x) {
            CHECK(xToY.contains(x));
        }
        for (int x = minX; x < maxX; ++x) {
            CHECK(std::abs(xToY[x + 1] - xToY[x]) <= 1); // 8-связность без разрывов
        }
    } else {
        // y — доминирующая ось: для каждого y от min(y0, y1) до max(y0, y1) ровно один пиксель
        const int minY = std::min(y0, y1);
        const int maxY = std::max(y0, y1);
        std::map<int, int> yToX;
        for (const auto& pt : pixels) {
            CHECK(!yToX.contains(pt.y)); // ровно один пиксель на строку
            yToX[pt.y] = pt.x;
        }
        for (int y = minY; y <= maxY; ++y) {
            CHECK(yToX.contains(y));
        }
        for (int y = minY; y < maxY; ++y) {
            CHECK(std::abs(yToX[y + 1] - yToX[y]) <= 1); // 8-связность без разрывов
        }
    }
}

void testOctantsAndContinuity() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;
    const auto lineColor = color(0, 1, 0);

    const std::vector<std::pair<Point2i, Point2i>> lineSegments = {
        // 8 октантов вокруг центра (32, 32)
        {{32, 32}, {52, 38}}, // Октант 1: dx >= dy > 0
        {{32, 32}, {38, 52}}, // Октант 2: dy > dx > 0
        {{32, 32}, {26, 52}}, // Октант 3: dy > |dx| > 0, dx < 0
        {{32, 32}, {12, 38}}, // Октант 4: |dx| >= dy > 0, dx < 0
        {{32, 32}, {12, 26}}, // Октант 5: |dx| >= |dy| > 0, dx < 0, dy < 0
        {{32, 32}, {26, 12}}, // Октант 6: |dy| > |dx| > 0, dx < 0, dy < 0
        {{32, 32}, {38, 12}}, // Октант 7: |dy| > dx > 0, dy < 0
        {{32, 32}, {52, 26}}, // Октант 8: dx >= |dy| > 0, dy < 0

        // Осевые направления
        {{10, 32}, {54, 32}}, // Горизонтальная вправо
        {{54, 32}, {10, 32}}, // Горизонтальная влево
        {{32, 10}, {32, 54}}, // Вертикальная вниз
        {{32, 54}, {32, 10}}, // Вертикальная вверх

        // Диагональные направления под 45 градусов
        {{10, 10}, {50, 50}}, // Диагональ +x +y
        {{50, 50}, {10, 10}}, // Диагональ -x -y
        {{50, 10}, {10, 50}}, // Диагональ -x +y
        {{10, 50}, {50, 10}}, // Диагональ +x -y

        // Вырожденный отрезок в одну точку
        {{32, 32}, {32, 32}}};

    for (const auto& [start, end] : lineSegments) {
        auto fb = makeFramebuffer(width, height);
        renderer::detail::clear(fb, color(0, 0, 0));
        renderer::rasterizeLine2D(start.x, start.y, end.x, end.y, lineColor, fb);

        const auto drawn = collectColoredPixels(fb, GREEN);
        verifyLineContinuity(start.x, start.y, end.x, end.y, drawn);
    }
}

void testZBufferCloserOverwritesFarther() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;
    auto fb = makeFramebuffer(width, height);
    renderer::detail::clear(fb, color(0, 0, 0));
    std::vector<double> depth(width * height, std::numeric_limits<double>::infinity());

    // Линия A: дальняя (z = 10.0), красный цвет, от (6, 6) до (26, 26)
    renderer::rasterizeLine3D(6, 6, 10.0, 26, 26, 10.0, color(1, 0, 0), fb, depth);

    // Линия B: ближняя (z = 5.0), зелёный цвет, от (6, 26) до (26, 6)
    // Пересекает линию A в точке (16, 16)
    renderer::rasterizeLine3D(6, 26, 5.0, 26, 6, 5.0, color(0, 1, 0), fb, depth);

    // Точка пересечения (16, 16) должна быть зелёной (ближняя линия)
    CHECK(pixel(fb, 16, 16) == GREEN);
    CHECK(std::abs(depth[16 * width + 16] - 5.0) < 1e-6);

    // Непересекающиеся пиксели линии A должны остаться красными
    CHECK(pixel(fb, 6, 6) == RED);
    CHECK(std::abs(depth[6 * width + 6] - 10.0) < 1e-6);

    // Непересекающиеся пиксели линии B должны быть зелёными
    CHECK(pixel(fb, 6, 26) == GREEN);
    CHECK(std::abs(depth[26 * width + 6] - 5.0) < 1e-6);
}

void testZBufferFartherDoesNotOverwriteCloser() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;
    auto fb = makeFramebuffer(width, height);
    renderer::detail::clear(fb, color(0, 0, 0));
    std::vector<double> depth(width * height, std::numeric_limits<double>::infinity());

    // Линия B: ближняя (z = 5.0), зелёный цвет, от (6, 26) до (26, 6)
    renderer::rasterizeLine3D(6, 26, 5.0, 26, 6, 5.0, color(0, 1, 0), fb, depth);

    // Линия A: дальняя (z = 10.0), красный цвет, от (6, 6) до (26, 26)
    renderer::rasterizeLine3D(6, 6, 10.0, 26, 26, 10.0, color(1, 0, 0), fb, depth);

    // Точка пересечения (16, 16) не должна быть перезаписана дальней линией A
    CHECK(pixel(fb, 16, 16) == GREEN);
    CHECK(std::abs(depth[16 * width + 16] - 5.0) < 1e-6);

    // Концы линии A должны быть красными с z = 10.0
    CHECK(pixel(fb, 6, 6) == RED);
    CHECK(std::abs(depth[6 * width + 6] - 10.0) < 1e-6);
}

void testZBufferLinearInterpolation() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;
    auto fb = makeFramebuffer(width, height);
    renderer::detail::clear(fb, color(0, 0, 0));
    std::vector<double> depth(width * height, std::numeric_limits<double>::infinity());

    // Горизонтальный отрезок от x=5 (z=2.0) до x=25 (z=6.0) на y=10
    renderer::rasterizeLine3D(5, 10, 2.0, 25, 10, 6.0, color(1, 0, 0), fb, depth);

    // Проверяем монотонность и значения интерполированной глубины
    for (int x = 5; x <= 25; ++x) {
        const double expectedZ = 2.0 + static_cast<double>(x - 5) / 20.0 * 4.0;
        const double actualZ = depth[10 * width + static_cast<std::size_t>(x)];
        CHECK(std::abs(actualZ - expectedZ) < 1e-5);
        if (x > 5) {
            const double prevZ = depth[10 * width + static_cast<std::size_t>(x - 1)];
            CHECK(actualZ > prevZ);
        }
    }

    // Теперь рисуем поверх отрезок с противоположным уклоном глубины: от x=5 (z=6.0) до x=25 (z=2.0) цветом BLUE
    renderer::rasterizeLine3D(5, 10, 6.0, 25, 10, 2.0, color(0, 0, 1), fb, depth);

    // Для x от 5 до 14: первый отрезок имел меньший z, поэтому остался RED
    for (int x = 5; x < 15; ++x) {
        CHECK(pixel(fb, static_cast<std::size_t>(x), 10) == RED);
    }
    // В точке x=15: z=4.0 у обоих; строгого неравенства z < depth нет, остаётся RED
    CHECK(pixel(fb, 15, 10) == RED);
    // Для x от 16 до 25: второй отрезок имеет меньший z, поэтому перезаписал на BLUE
    for (int x = 16; x <= 25; ++x) {
        CHECK(pixel(fb, static_cast<std::size_t>(x), 10) == BLUE);
    }
}

void testAlphaBlending() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;
    auto fb = makeFramebuffer(width, height);
    renderer::detail::clear(fb, color(0, 0, 0)); // Чёрный непрозрачный фон

    // Полупрозрачная красная линия с alpha = 0.5
    renderer::rasterizeLine2D(5, 5, 25, 5, color(1, 0, 0, 0.5), fb);

    for (std::size_t x = 5; x <= 25; ++x) {
        const auto px = pixel(fb, x, 5);
        // 0 * 0.5 + 255 * 0.5 ~ 128
        CHECK(px.red >= 126 && px.red <= 129);
        CHECK(px.green == 0);
        CHECK(px.blue == 0);
        CHECK(px.alpha == 255);
    }
}

void testBoundsClipping() {
    constexpr std::size_t width = 20;
    constexpr std::size_t height = 20;
    auto fb = makeFramebuffer(width, height);
    renderer::detail::clear(fb, color(0, 0, 0));

    // Отрезок, начинающийся и заканчивающийся вне буфера
    renderer::rasterizeLine2D(-10, 10, 30, 10, color(1, 0, 0), fb);

    // Вся горизонталь y=10 внутри [0, 19] должна быть красной
    for (std::size_t x = 0; x < width; ++x) {
        CHECK(pixel(fb, x, 10) == RED);
    }
    // Соседние строки должны остаться чёрными
    for (std::size_t x = 0; x < width; ++x) {
        CHECK(pixel(fb, x, 9) == BLACK);
        CHECK(pixel(fb, x, 11) == BLACK);
    }

    // Полностью внешний отрезок не должен изменять буфер
    renderer::rasterizeLine2D(-50, -50, -10, -10, color(0, 1, 0), fb);
    CHECK(pixel(fb, 0, 0) == BLACK);

    // 3D-отрезок с Z-буфером, частично выходящий за границы
    std::vector<double> depth(width * height, std::numeric_limits<double>::infinity());
    renderer::rasterizeLine3D(-5, 5, 1.0, 25, 5, 1.0, color(0, 0, 1), fb, depth);
    for (std::size_t x = 0; x < width; ++x) {
        CHECK(pixel(fb, x, 5) == BLUE);
        CHECK(std::abs(depth[5 * width + x] - 1.0) < 1e-6);
    }
}

int main() {
    try {
        testOctantsAndContinuity();
        testZBufferCloserOverwritesFarther();
        testZBufferFartherDoesNotOverwriteCloser();
        testZBufferLinearInterpolation();
        testAlphaBlending();
        testBoundsClipping();
        std::cout << "All line rasterization tests passed successfully.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
