add_library(huynity_renderer_actor INTERFACE)
add_library(huynity::renderer_actor ALIAS huynity_renderer_actor)
target_include_directories(huynity_renderer_actor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(huynity_renderer_actor INTERFACE huynity::runtime)
target_compile_features(huynity_renderer_actor INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
