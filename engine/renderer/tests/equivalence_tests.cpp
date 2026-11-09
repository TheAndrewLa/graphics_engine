#include "huynity/image/image_buffer.h"
#include "huynity/math/point.h"
#include "huynity/math/transform.h"
#include "huynity/math/vector.h"
#include "huynity/render_contracts/render_snapshot.h"
#include "huynity/renderer/software_renderer.h"
#include "huynity/runtime/frame_completions.h"
#include "huynity/runtime/renderer_port.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace render = huynity::render_contracts;
namespace image = huynity::image;
namespace math = huynity::math;
namespace renderer = huynity::renderer;
namespace runtime = huynity::runtime;

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

render::Color makeColor(double red, double green, double blue, double alpha = 1.0) {
    return {red, green, blue, alpha};
}

std::shared_ptr<const render::Material> makeMaterial(double red, double green, double blue, double alpha = 1.0) {
    return std::make_shared<const render::Material>(makeColor(red, green, blue, alpha));
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

/// @brief Адаптер асинхронной доставки кадров к программному рендереру.
///
/// Реализует неблокирующий RendererPort: при trySubmit() принимает владение RenderFrame,
/// затем при обработке (processAndDeliver) выполняет программную растеризацию в
/// frame.framebuffer и доставляет FrameReady через CompletionChannel.
class FakeAsyncRendererAdapter final : public runtime::RendererPort {
public:
    explicit FakeAsyncRendererAdapter(runtime::CompletionChannel& channel) : m_channel{&channel} {}

    [[nodiscard]] runtime::SubmitResult trySubmit(runtime::RenderFrame& frame) override {
        if (m_active || (m_channel && !m_channel->reserve())) {
            return runtime::SubmitResult::Rejected;
        }
        m_active.emplace(std::move(frame));
        return runtime::SubmitResult::Accepted;
    }

    [[nodiscard]] bool isBusy() const noexcept { return m_active.has_value(); }

    [[nodiscard]] bool processAndDeliver() {
        if (!m_active || !m_channel) {
            return false;
        }
        auto frame = std::move(*m_active);
        m_active.reset();

        auto renderResult = m_renderer.render(*frame.snapshot, *frame.framebuffer);
        if (!renderResult) {
            return m_channel->publish(runtime::FrameFailed{frame.frameId, std::move(frame), "render error"});
        }
        const auto id = frame.frameId;
        return m_channel->publish(runtime::FrameReady{id, std::move(frame)});
    }

    [[nodiscard]] std::size_t shadingCacheHits() const noexcept { return m_renderer.shadingCacheHits(); }
    [[nodiscard]] std::size_t shadingCacheMisses() const noexcept { return m_renderer.shadingCacheMisses(); }

private:
    std::optional<runtime::RenderFrame> m_active;
    runtime::CompletionChannel* m_channel{};
    renderer::SoftwareRenderer m_renderer;
};

void testComplexSceneEquivalence() {
    constexpr std::size_t width = 64;
    constexpr std::size_t height = 64;

    // Геометрия 3D-мира: треугольник на глубине z = 3, ориентированный лицевой стороной к камере
    TestMesh3 mesh3{
        .positions = {{-1.0, -1.0, 3.0}, {0.0, 1.0, 3.0}, {1.0, -1.0, 3.0}},
        .triangles = {{0, 1, 2}},
        .owner = std::make_shared<const int>(1),
    };

    // Геометрия 2D-мира: треугольник в мировых координатах
    TestMesh2 mesh2World{
        .positions = {{-1.5, 0.5}, {-0.5, 1.5}, {-0.5, 0.5}},
        .triangles = {{0, 1, 2}},
        .owner = std::make_shared<const int>(2),
    };

    // Геометрия HUD: непрозрачный базовый прямоугольник и полупрозрачный слой поверх него
    TestMesh2 mesh2HudBase{
        .positions = {{40.0, 40.0}, {60.0, 40.0}, {60.0, 60.0}, {40.0, 60.0}},
        .triangles = {{0, 1, 2}, {0, 2, 3}},
        .owner = std::make_shared<const int>(3),
    };

    TestMesh2 mesh2HudOverlay{
        .positions = {{45.0, 45.0}, {62.0, 45.0}, {45.0, 62.0}},
        .triangles = {{0, 1, 2}},
        .owner = std::make_shared<const int>(4),
    };

    render::RenderSnapshot snapshot;
    snapshot.imageWidth = width;
    snapshot.imageHeight = height;
    snapshot.clearColor = makeColor(0.05, 0.05, 0.1, 1.0); // тёмно-синий фон

    // Материалы: 0 - 3D красный, 1 - 2D зелёный, 2 - HUD синий, 3 - HUD полупрозрачный жёлтый (alpha 0.5)
    snapshot.materials = {
        makeMaterial(0.8, 0.2, 0.2, 1.0),
        makeMaterial(0.2, 0.8, 0.3, 1.0),
        makeMaterial(0.1, 0.2, 0.9, 1.0),
        makeMaterial(1.0, 0.8, 0.0, 0.5),
    };

    addResource3(snapshot, mesh3);
    addResource2(snapshot, mesh2World);
    addResource2(snapshot, mesh2HudBase);
    addResource2(snapshot, mesh2HudOverlay);

    // Pass 0: World3DPass со светом (ambient + directional), камерой и flat shading
    render::World3DPass world3;
    world3.camera = render::PerspectiveCamera3{
        .worldToView = math::Transform3{},
        .verticalFieldOfView = std::numbers::pi / 3,
        .nearPlane = 0.5,
        .farPlane = 50.0,
    };
    world3.lighting = {
        .ambient = {render::Color{0.25, 0.25, 0.25, 1.0}, 1.0},
        .directional =
            {
                {
                    .direction = math::Vector3{0.0, 0.0, 1.0},
                    .color = render::Color{1.0, 1.0, 1.0, 1.0},
                    .intensity = 0.75,
                },
            },
    };
    world3.drawItems.push_back({0, math::Transform3{}, 0, true});
    snapshot.passes.push_back(world3);

    // Pass 1: World2DPass с собственной 2D-камерой
    render::World2DPass world2;
    world2.camera = render::Camera2{
        .worldToView = math::Transform2{},
        .width = 4.0,
        .height = 4.0,
    };
    world2.drawItems.push_back({0, math::Transform2{}, 1, 0, 0});
    snapshot.passes.push_back(world2);

    // Pass 2: Hud2DPass в экранных пикселях с alpha blending
    render::Hud2DPass hud;
    hud.drawItems = {
        {1, math::Transform2{}, 2, 0, 0}, // непрозрачный синий слой
        {2, math::Transform2{}, 3, 0, 1}, // полупрозрачный жёлтый поверх него (alpha 0.5)
    };
    snapshot.passes.push_back(hud);

    // 1. Эталонный синхронный рендеринг напрямую в syncFramebuffer
    auto syncFramebuffer = makeFramebuffer(width, height);
    renderer::SoftwareRenderer syncRenderer;
    const auto syncResult = syncRenderer.render(snapshot, syncFramebuffer);
    CHECK(syncResult.has_value());

    // 2. Асинхронная доставка того же снимка через RendererPort и CompletionChannel
    runtime::CompletionChannel channel;
    FakeAsyncRendererAdapter adapter{channel};

    auto asyncSnapshot = std::make_unique<render::RenderSnapshot>(snapshot);
    auto asyncFramebufferPtr = std::make_unique<image::ImageBuffer>(makeFramebuffer(width, height));

    runtime::RenderFrame frame{
        .frameId = 101,
        .snapshot = std::move(asyncSnapshot),
        .framebuffer = std::move(asyncFramebufferPtr),
    };

    const auto submitResult = adapter.trySubmit(frame);
    CHECK(submitResult == runtime::SubmitResult::Accepted);
    CHECK(frame.snapshot == nullptr);    // владение снимком передано адаптеру
    CHECK(frame.framebuffer == nullptr); // владение буфером передано адаптеру
    CHECK(adapter.isBusy());

    // Адаптер выполняет программную растеризацию и доставляет результат в CompletionChannel
    CHECK(adapter.processAndDeliver());
    CHECK(!adapter.isBusy());
    CHECK(channel.hasCompletion());

    // Получаем результат кадра из канала
    auto completion = channel.take();
    CHECK(completion.has_value());
    CHECK(std::holds_alternative<runtime::FrameReady>(*completion));

    auto& ready = std::get<runtime::FrameReady>(*completion);
    CHECK(ready.frameId == 101);
    CHECK(ready.frame.framebuffer != nullptr);

    const auto& asyncFramebuffer = *ready.frame.framebuffer;

    // 3. Сравнение параметров изображения
    CHECK(syncFramebuffer.width() == asyncFramebuffer.width());
    CHECK(syncFramebuffer.height() == asyncFramebuffer.height());
    CHECK(syncFramebuffer.stride() == asyncFramebuffer.stride());
    CHECK(syncFramebuffer.format() == asyncFramebuffer.format());
    CHECK(syncFramebuffer.sizeBytes() == asyncFramebuffer.sizeBytes());
    CHECK(syncFramebuffer.bytes().size() == asyncFramebuffer.bytes().size());

    // 4. Побайтовое сравнение всего содержимого (sync.bytes() == async.bytes())
    CHECK(std::ranges::equal(syncFramebuffer.bytes(), asyncFramebuffer.bytes()));
    CHECK(std::memcmp(syncFramebuffer.bytes().data(), asyncFramebuffer.bytes().data(), syncFramebuffer.sizeBytes()) ==
          0);

    // 5. Построчное сравнение и проверка каждого пикселя
    for (std::size_t y = 0; y < height; ++y) {
        CHECK(std::ranges::equal(syncFramebuffer.row(y), asyncFramebuffer.row(y)));
        for (std::size_t x = 0; x < width; ++x) {
            const auto syncPx = pixel(syncFramebuffer, x, y);
            const auto asyncPx = pixel(asyncFramebuffer, x, y);
            CHECK(syncPx == asyncPx);
        }
    }

    // 6. Проверка нетривиальности изображения (все слои дали свой вклад)
    const Rgba clearPx = pixel(syncFramebuffer, 0, 0); // фоновый угол
    CHECK(clearPx == Rgba{13, 13, 26, 255});           // 0.05, 0.05, 0.1 * 255

    const Rgba world3dPx = pixel(syncFramebuffer, 32, 32); // центр 3D-треугольника
    CHECK(world3dPx == Rgba{204, 51, 51, 255});            // 0.8 * 255, 0.2 * 255, 0.2 * 255

    const Rgba world2dPx = pixel(syncFramebuffer, 20, 20); // 2D-треугольник мира
    CHECK(world2dPx == Rgba{51, 204, 77, 255});            // 0.2 * 255, 0.8 * 255, 0.3 * 255

    const Rgba hudBasePx = pixel(syncFramebuffer, 42, 42); // базовый синий слой HUD
    CHECK(hudBasePx == Rgba{26, 51, 230, 255});            // 0.1, 0.2, 0.9 * 255

    const Rgba hudBlendedPx = pixel(syncFramebuffer, 50, 50); // смешанный HUD с alpha 0.5
    // source: yellow (255, 204, 0), dest: blue (26, 51, 230), alpha 0.5
    // red: 255 * 0.5 + 26 * 0.5 = 140, green: 204 * 0.5 + 51 * 0.5 = 128, blue: 0 * 0.5 + 230 * 0.5 = 115
    CHECK(hudBlendedPx.red > 130 && hudBlendedPx.red < 150);
    CHECK(hudBlendedPx.green > 120 && hudBlendedPx.green < 135);
    CHECK(hudBlendedPx.blue > 105 && hudBlendedPx.blue < 125);
    CHECK(hudBlendedPx.alpha == 255);
}

void testMultiFrameEquivalenceAndCacheConsistency() {
    constexpr std::size_t width = 32;
    constexpr std::size_t height = 32;

    TestMesh3 mesh3{
        .positions = {{-1.0, -1.0, 2.0}, {0.0, 1.0, 2.0}, {1.0, -1.0, 2.0}},
        .triangles = {{0, 1, 2}},
        .owner = std::make_shared<const int>(10),
    };

    render::RenderSnapshot snapshot1;
    snapshot1.imageWidth = width;
    snapshot1.imageHeight = height;
    snapshot1.clearColor = makeColor(0.0, 0.0, 0.0, 1.0);
    snapshot1.materials = {makeMaterial(0.9, 0.1, 0.1, 1.0)};
    addResource3(snapshot1, mesh3);

    render::World3DPass world3;
    world3.camera = render::OrthographicCamera3{
        .worldToView = math::Transform3{},
        .width = 4.0,
        .height = 4.0,
        .nearPlane = 0.1,
        .farPlane = 20.0,
    };
    world3.lighting = {{render::Color{1.0, 1.0, 1.0, 1.0}, 1.0}, {}};
    world3.drawItems.push_back({0, math::Transform3{}, 0, true});
    snapshot1.passes.push_back(world3);

    renderer::SoftwareRenderer syncRenderer;
    runtime::CompletionChannel channel;
    FakeAsyncRendererAdapter asyncAdapter{channel};

    // Кадр 1: холодный кеш (miss)
    auto syncFb1 = makeFramebuffer(width, height);
    CHECK(syncRenderer.render(snapshot1, syncFb1).has_value());

    runtime::RenderFrame frame1{
        .frameId = 1,
        .snapshot = std::make_unique<render::RenderSnapshot>(snapshot1),
        .framebuffer = std::make_unique<image::ImageBuffer>(makeFramebuffer(width, height)),
    };
    CHECK(asyncAdapter.trySubmit(frame1) == runtime::SubmitResult::Accepted);
    CHECK(asyncAdapter.processAndDeliver());
    auto comp1 = channel.take();
    CHECK(comp1.has_value());
    const auto& asyncFb1 = *std::get<runtime::FrameReady>(*comp1).frame.framebuffer;

    CHECK(std::ranges::equal(syncFb1.bytes(), asyncFb1.bytes()));
    CHECK(syncRenderer.shadingCacheMisses() == 1);
    CHECK(asyncAdapter.shadingCacheMisses() == 1);

    // Кадр 2: повторный снимок с неизменной геометрией (кеш hit)
    auto snapshot2 = snapshot1;
    auto& pass = std::get<render::World3DPass>(snapshot2.passes[0]);
    pass.lighting.ambient.intensity = 0.5; // изменили только освещение

    auto syncFb2 = makeFramebuffer(width, height);
    CHECK(syncRenderer.render(snapshot2, syncFb2).has_value());

    runtime::RenderFrame frame2{
        .frameId = 2,
        .snapshot = std::make_unique<render::RenderSnapshot>(snapshot2),
        .framebuffer = std::make_unique<image::ImageBuffer>(makeFramebuffer(width, height)),
    };
    CHECK(asyncAdapter.trySubmit(frame2) == runtime::SubmitResult::Accepted);
    CHECK(asyncAdapter.processAndDeliver());
    auto comp2 = channel.take();
    CHECK(comp2.has_value());
    const auto& asyncFb2 = *std::get<runtime::FrameReady>(*comp2).frame.framebuffer;

    CHECK(std::ranges::equal(syncFb2.bytes(), asyncFb2.bytes()));
    CHECK(syncRenderer.shadingCacheHits() == 1);
    CHECK(asyncAdapter.shadingCacheHits() == 1);
}

void testAsyncRejectionWhenBusyPreservesFrame() {
    runtime::CompletionChannel channel;
    FakeAsyncRendererAdapter adapter{channel};

    auto fb1 = std::make_unique<image::ImageBuffer>(makeFramebuffer(16, 16));
    auto snap1 = std::make_unique<render::RenderSnapshot>();
    snap1->imageWidth = 16;
    snap1->imageHeight = 16;

    runtime::RenderFrame frame1{1, std::move(snap1), std::move(fb1)};
    CHECK(adapter.trySubmit(frame1) == runtime::SubmitResult::Accepted);
    CHECK(adapter.isBusy());

    auto fb2 = std::make_unique<image::ImageBuffer>(makeFramebuffer(16, 16));
    auto snap2 = std::make_unique<render::RenderSnapshot>();
    snap2->imageWidth = 16;
    snap2->imageHeight = 16;

    runtime::RenderFrame frame2{2, std::move(snap2), std::move(fb2)};
    CHECK(adapter.trySubmit(frame2) == runtime::SubmitResult::Rejected);
    // При Rejected владение фреймом сохраняется у вызывающей стороны
    CHECK(frame2.snapshot != nullptr);
    CHECK(frame2.framebuffer != nullptr);

    CHECK(adapter.processAndDeliver());
    CHECK(!adapter.isBusy());
    CHECK(channel.hasCompletion());

    // Потребитель забирает завершение из канала, освобождая зарезервированный слот
    auto completion1 = channel.take();
    CHECK(completion1.has_value());

    // После освобождения адаптера и канала повторная подача принимается
    CHECK(adapter.trySubmit(frame2) == runtime::SubmitResult::Accepted);
    CHECK(frame2.snapshot == nullptr);
    CHECK(frame2.framebuffer == nullptr);
}

int main() {
    try {
        testComplexSceneEquivalence();
        testMultiFrameEquivalenceAndCacheConsistency();
        testAsyncRejectionWhenBusyPreservesFrame();
        std::cout << "renderer_equivalence_tests: OK\n";
    } catch (const std::exception& error) {
        std::cerr << "renderer_equivalence_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
