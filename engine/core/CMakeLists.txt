if(TARGET huynity::window)
    add_library(huynity_core
        src/engine.cpp
    )
    add_library(huynity::core ALIAS huynity_core)
    add_library(huynity::engine ALIAS huynity_core)
    target_include_directories(huynity_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(huynity_core PUBLIC
        huynity::objectlib
        huynity::render_contracts
        huynity::runtime
        huynity::renderer
        huynity::window
    )
    target_compile_features(huynity_core PUBLIC cxx_std_23)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(huynity_core PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    add_library(huynity_core INTERFACE)
    add_library(huynity::core ALIAS huynity_core)
    add_library(huynity::engine ALIAS huynity_core)
    target_include_directories(huynity_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(huynity_core INTERFACE huynity::objectlib huynity::render_contracts)
    target_compile_features(huynity_core INTERFACE cxx_std_23)
endif()

add_custom_target(huynity_engine DEPENDS huynity_core)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
