#pragma once

#include "huynity/math/transform.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace huynity::image {
class ImageBuffer;
}

namespace huynity::render_contracts {

struct Color {
    double red{1};
    double green{1};
    double blue{1};
    double alpha{1};
};

struct PerspectiveCamera3 {
    math::Transform3 worldToView{};
    double verticalFieldOfView{};
    double nearPlane{};
    double farPlane{};
};

struct OrthographicCamera3 {
    math::Transform3 worldToView{};
    double width{};
    double height{};
    double nearPlane{};
    double farPlane{};
};

struct Camera2 {
    math::Transform2 worldToView{};
    double width{};
    double height{};
};

using Camera3 = std::variant<PerspectiveCamera3, OrthographicCamera3>;

enum class TextureFilter {
    Nearest,
    Bilinear,
};

enum class TextureWrap {
    Clamp,
    Repeat,
};

struct Material {
    Color color{};
    std::shared_ptr<const image::ImageBuffer> texture{};
    TextureFilter filter{TextureFilter::Nearest};
    TextureWrap wrap{TextureWrap::Clamp};
};

struct AmbientLight {
    Color color{};
    double intensity{};
};

struct DirectionalLight {
    math::Vector3 direction{};
    Color color{};
    double intensity{};
};

struct Lighting3 {
    AmbientLight ambient{};
    std::vector<DirectionalLight> directional;
};

struct TriangleIndices {
    std::uint32_t a{};
    std::uint32_t b{};
    std::uint32_t c{};
};

template <typename Point>
struct MeshResource {
    std::shared_ptr<const void> owner;
    std::span<const Point> positions;
    std::span<const std::byte> triangles;
    std::span<const math::Point2> texCoords{};
};

using MeshResource2 = MeshResource<math::Point2>;
using MeshResource3 = MeshResource<math::Point3>;

using ResourceIndex = std::size_t;
using MaterialIndex = std::size_t;

enum class ShadingModel {
    Flat,
    Gouraud
};

struct DrawItem3 {
    ResourceIndex resource{};
    math::Transform3 localToWorld{};
    MaterialIndex material{};
    bool backfaceCulling{true};
    bool wireframe{false};
    ShadingModel shading{ShadingModel::Flat};
};

struct DrawItem2 {
    ResourceIndex resource{};
    math::Transform2 localToWorld{};
    MaterialIndex material{};
    std::int32_t layer{};
    std::int32_t order{};
    bool wireframe{false};
};

struct World3DPass {
    Camera3 camera{PerspectiveCamera3{}};
    Lighting3 lighting{};
    std::vector<DrawItem3> drawItems;
};

struct World2DPass {
    Camera2 camera{};
    std::vector<DrawItem2> drawItems;
};

struct Hud2DPass {
    std::vector<DrawItem2> drawItems;
};

using RenderPass = std::variant<World3DPass, World2DPass, Hud2DPass>;

struct RenderSnapshot {
    std::uint64_t sceneRevision{};
    std::uint64_t renderRevision{};
    std::size_t imageWidth{};
    std::size_t imageHeight{};
    Color clearColor{};
    std::vector<std::shared_ptr<const Material>> materials;
    std::vector<MeshResource3> resources3;
    std::vector<MeshResource2> resources2;
    std::vector<RenderPass> passes;
};

} // namespace huynity::render_contracts
