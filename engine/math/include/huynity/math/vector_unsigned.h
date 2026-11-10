#pragma once

#include "huynity/math/dimension.h"

namespace huynity::math {

template <std::size_t dimension>
struct VectorUnsigned : detail::DimContainer<unsigned, dimension> {
    using detail::DimContainer<unsigned, dimension>::DimContainer;
};

using VectorUnsinged2 = VectorUnsigned<2>;
using VectorUnsigned3 = VectorUnsigned<3>;
} // namespace huynity::math
