add_library(huynity_render_contracts INTERFACE)
add_library(huynity::render_contracts ALIAS huynity_render_contracts)
target_include_directories(huynity_render_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(huynity_render_contracts INTERFACE huynity::math)
target_compile_features(huynity_render_contracts INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
