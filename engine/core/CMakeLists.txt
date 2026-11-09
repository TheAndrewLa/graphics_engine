add_library(huynity_core INTERFACE)
add_library(huynity::core ALIAS huynity_core)
target_include_directories(huynity_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(huynity_core INTERFACE huynity::objectlib huynity::render_contracts)
target_compile_features(huynity_core INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
