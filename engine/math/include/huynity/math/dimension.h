#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace huynity::math {

template <std::size_t dimension>
concept Dimension = dimension == 2 || dimension == 3;

namespace detail {

template <typename T, std::size_t dimension>
  requires Dimension<dimension> && std::is_arithmetic_v<T>
class DimensionalContainer {
public:
  DimensionalContainer() : DimensionalContainer(T()) {}

  explicit DimensionalContainer(T component) { m_components.fill(component); }

  template <typename... Ts>
    requires(sizeof...(Ts) == dimension &&
             std::conjunction_v<std::is_constructible<T, Ts>...>)
  DimensionalContainer(Ts &&...values) : m_components(T(values)...) {}

  constexpr T &operator[](std::size_t index) { return m_components[index]; }

  constexpr T operator[](std::size_t index) const {
    return m_components[index];
  }

  constexpr T *begin() { return m_components.begin(); }
  constexpr const T *cbegin() const { return m_components.cbegin(); }

  constexpr T *end() { return m_components.end(); }
  constexpr const T *cend() const { return m_components.cend(); }

  constexpr friend bool operator==(const DimensionalContainer &,
                                   const DimensionalContainer &) = default;

  constexpr bool finite() const { return true; }

  constexpr bool finite() const
    requires std::is_floating_point_v<T>
  {
    return std::ranges::all_of(m_components, std::isfinite);
  }

private:
  std::array<T, dimension> m_components{};
};

} // namespace detail
} // namespace huynity::math
