add_library(huynity_runtime INTERFACE)
add_library(huynity::runtime ALIAS huynity_runtime)
target_include_directories(huynity_runtime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(huynity_runtime INTERFACE huynity::render_contracts huynity::image)
target_compile_features(huynity_runtime INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
