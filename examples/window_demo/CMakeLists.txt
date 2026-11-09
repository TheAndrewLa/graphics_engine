add_executable(huynity_window_demo main.cpp)
target_link_libraries(huynity_window_demo PRIVATE
    huynity::window
    huynity::runtime
    huynity::core
    huynity::renderer
    huynity::objectlib
    huynity::image
    huynity::renderer_actor
)
target_compile_features(huynity_window_demo PRIVATE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_window_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
