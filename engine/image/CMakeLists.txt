add_library(huynity_image INTERFACE)
add_library(huynity::image ALIAS huynity_image)
target_include_directories(huynity_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(huynity_image INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
