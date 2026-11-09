#include "huynity/objectlib/create_object.h"
#include "huynity/objectlib/operations/tessellate.h"

#include <iostream>

int main() {
    namespace objects = huynity::objectlib;
    auto sphere = objects::createObject<objects::ObjectType::Sphere3>(objects::Sphere3Params{.radius = 2.0});
    if (!sphere) {
        return 1;
    }
    auto mesh = objects::tessellate(*sphere, {.maxDeviation = 0.05, .maxPrimitiveCount = 4096});
    if (!mesh) {
        return 2;
    }
    auto geometry = objects::createObject(objects::ObjectSpec{objects::Sphere3Params{.radius = 2.0}});
    if (!geometry || !std::holds_alternative<objects::Sphere3>(*geometry)) {
        return 3;
    }
    std::cout << "Sphere: radius=" << sphere->radius() << ", vertices=" << mesh->positions().size()
              << ", triangles=" << mesh->triangles().size() << '\n';
}
