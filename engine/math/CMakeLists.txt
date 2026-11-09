add_library(huynity_math INTERFACE)
add_library(huynity::math ALIAS huynity_math)
target_include_directories(huynity_math INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(huynity_math INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
