#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace huynity::math {

template <std::size_t dimension>
concept Dimension = dimension == 2 || dimension == 3 || dimension == 4;

namespace detail {

template <typename T, std::size_t dimension>
  requires Dimension<dimension> && std::is_arithmetic_v<T>
class DimensionalContainer {
public:
  constexpr DimensionalContainer() { m_components.fill(T()); }

  template <typename... Ts>
    requires(sizeof...(Ts) == dimension &&
             std::conjunction_v<std::is_constructible<T, Ts>...>)
  constexpr DimensionalContainer(Ts &&...values)
      : m_components{T(std::forward<Ts>(values))...} {}

  constexpr T &operator[](std::size_t i) { return m_components[i]; }
  constexpr T operator[](std::size_t i) const { return m_components[i]; }

  constexpr T *begin() { return m_components.begin(); }
  constexpr T *end() { return m_components.end(); }

  constexpr const T *cbegin() const { return m_components.cbegin(); }
  constexpr const T *cend() const { return m_components.cend(); }

  constexpr friend bool operator==(const DimensionalContainer &,
                                   const DimensionalContainer &) = default;

  [[nodiscard]] constexpr bool finite() const
    requires std::is_floating_point_v<T>
  {
    return std::ranges::all_of(m_components, std::isfinite);
  }

private:
  std::array<T, dimension> m_components;
};

} // namespace detail
} // namespace huynity::math
