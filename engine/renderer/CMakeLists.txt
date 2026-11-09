add_library(huynity_renderer INTERFACE)
add_library(huynity::renderer ALIAS huynity_renderer)
target_include_directories(huynity_renderer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(huynity_renderer INTERFACE huynity::render_contracts huynity::image)
target_compile_features(huynity_renderer INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
