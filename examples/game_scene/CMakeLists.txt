add_executable(huynity_game_scene main.cpp)
target_link_libraries(huynity_game_scene PRIVATE
    huynity::engine
    huynity::core
    huynity::renderer
    huynity::runtime
    huynity::image
    huynity::math
    huynity::window
)
set_target_properties(huynity_game_scene PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_game_scene PRIVATE -Wall -Wextra -Wpedantic)
endif()
