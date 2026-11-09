#pragma once

#include <cstddef>

namespace huynity::math {

template <std::size_t dimension>
concept Dimension = dimension == 2 || dimension == 3;

} // namespace huynity::math
