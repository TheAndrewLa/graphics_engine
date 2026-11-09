add_executable(huynity_renderer_gallery main.cpp)

target_link_libraries(huynity_renderer_gallery PRIVATE
    huynity::engine
    huynity::renderer
    huynity::core
    huynity::window
    huynity::runtime
    huynity::image
    huynity::math
)

target_compile_features(huynity_renderer_gallery PRIVATE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_renderer_gallery PRIVATE -Wall -Wextra -Wpedantic)
endif()
