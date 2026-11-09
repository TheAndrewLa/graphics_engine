add_executable(huynity_hello_cube main.cpp)
target_link_libraries(huynity_hello_cube PRIVATE
    huynity::engine
)
target_compile_features(huynity_hello_cube PRIVATE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_hello_cube PRIVATE -Wall -Wextra -Wpedantic)
endif()
