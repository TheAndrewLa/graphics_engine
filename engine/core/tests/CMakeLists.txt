add_executable(huynity_core_tests core_tests.cpp)
target_link_libraries(huynity_core_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.storage COMMAND huynity_core_tests)

add_executable(huynity_core_scene_tests scene_tests.cpp)
target_link_libraries(huynity_core_scene_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.scene COMMAND huynity_core_scene_tests)

add_executable(huynity_core_hierarchy_tests hierarchy_tests.cpp)
target_link_libraries(huynity_core_hierarchy_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_hierarchy_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.hierarchy COMMAND huynity_core_hierarchy_tests)

add_executable(huynity_core_state_tests state_tests.cpp)
target_link_libraries(huynity_core_state_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_state_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.state COMMAND huynity_core_state_tests)

add_executable(huynity_core_command_tests command_tests.cpp)
target_link_libraries(huynity_core_command_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_command_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.commands COMMAND huynity_core_command_tests)

add_executable(huynity_core_spatial_tests spatial_tests.cpp)
target_link_libraries(huynity_core_spatial_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_spatial_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.spatial COMMAND huynity_core_spatial_tests)

add_executable(huynity_core_tessellation_tests tessellation_tests.cpp)
target_link_libraries(huynity_core_tessellation_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_tessellation_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.tessellation_cache COMMAND huynity_core_tessellation_tests)

add_executable(huynity_core_draw_list_tests draw_list_tests.cpp)
target_link_libraries(huynity_core_draw_list_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_draw_list_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.draw_lists COMMAND huynity_core_draw_list_tests)

add_executable(huynity_core_hud_tests hud_tests.cpp)
target_link_libraries(huynity_core_hud_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_hud_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.hud COMMAND huynity_core_hud_tests)

add_executable(huynity_core_snapshot_tests snapshot_tests.cpp)
target_link_libraries(huynity_core_snapshot_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_snapshot_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.snapshot COMMAND huynity_core_snapshot_tests)

add_executable(huynity_core_finite_tests finite_representation_tests.cpp)
target_link_libraries(huynity_core_finite_tests PRIVATE huynity::core)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(huynity_core_finite_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core.finite_representation COMMAND huynity_core_finite_tests)

if(TARGET huynity::window)
    add_executable(huynity_engine_tests engine_tests.cpp)
    target_link_libraries(huynity_engine_tests PRIVATE
        huynity::core
        huynity::runtime
        huynity::renderer
        huynity::window
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(huynity_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME core.engine COMMAND huynity_engine_tests)

    add_executable(huynity_engine_lifecycle_tests engine_lifecycle_tests.cpp)
    target_link_libraries(huynity_engine_lifecycle_tests PRIVATE
        huynity::core
        huynity::runtime
        huynity::renderer
        huynity::window
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(huynity_engine_lifecycle_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME core.engine_lifecycle COMMAND huynity_engine_lifecycle_tests)

    add_executable(huynity_engine_preview_tests engine_preview_tests.cpp)
    target_link_libraries(huynity_engine_preview_tests PRIVATE
        huynity::core
        huynity::runtime
        huynity::renderer
        huynity::window
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(huynity_engine_preview_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME core.engine_preview COMMAND huynity_engine_preview_tests)

    add_executable(huynity_engine_mixed_scene_tests engine_mixed_scene_tests.cpp)
    target_link_libraries(huynity_engine_mixed_scene_tests PRIVATE
        huynity::core
        huynity::runtime
        huynity::renderer
        huynity::window
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(huynity_engine_mixed_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME core.engine_mixed_scene COMMAND huynity_engine_mixed_scene_tests)
endif()
