# ============================================================================
# tests/CMakeLists.txt — BlueprintRuntime 单元测试
# ============================================================================
#
# 使用 Google Test（via FetchContent，无需手动安装）
# 运行：cmake --build build-windows --target run_tests
#       或 ctest --test-dir build-windows -C Release
# ============================================================================

cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Google Test：优先使用系统安装版，fallback 到 FetchContent
# ---------------------------------------------------------------------------
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL      https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    # FetchContent 版本的 target 名与 find_package 一致
    if(NOT TARGET GTest::gtest_main)
        add_library(GTest::gtest_main ALIAS gtest_main)
    endif()
endif()

# ---------------------------------------------------------------------------
# 测试可执行文件
# ---------------------------------------------------------------------------
add_executable(blueprint_tests
    test_registry.cpp
    test_serialization.cpp
    test_topology.cpp
    test_handlers.cpp
    test_agent_demo.cpp
)

target_link_libraries(blueprint_tests
    PRIVATE
        BlueprintRuntime
        crude_json
        GTest::gtest_main
)

# TEST_ASSETS_DIR：指向 tests/assets，供 test_handlers 等定位 bjson 文件
# TEST_PROJECT_DIR：指向项目根目录，供 test_agent_demo 定位 examples/
target_compile_definitions(blueprint_tests PRIVATE
    TEST_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    TEST_PROJECT_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(blueprint_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/Utils/Json
    $<TARGET_PROPERTY:BlueprintRuntime,INTERFACE_INCLUDE_DIRECTORIES>
)

# Lua headers (baked into BlueprintRuntime; expose include path for test files)
if(TARGET lua54_static)
    target_include_directories(blueprint_tests PRIVATE $<TARGET_PROPERTY:lua54_static,INTERFACE_INCLUDE_DIRECTORIES>)
elseif(TARGET lua54)
    target_include_directories(blueprint_tests PRIVATE $<TARGET_PROPERTY:lua54,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

if(BLUEPRINT_LUA)
    target_compile_definitions(blueprint_tests PRIVATE BLUEPRINT_HAS_LUA)
endif()

set_target_properties(blueprint_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release"
    FOLDER "Tests"
)

# ---------------------------------------------------------------------------
# CTest 集成
# ---------------------------------------------------------------------------
include(GoogleTest)

# 动态库构建时，测试运行需要能找到 BlueprintRuntime.dll/.so
# 通过 DISCOVERY_MODE PRE_TEST 推迟发现（避免 configure 时运行 exe）
# 通过 ENVIRONMENT 传入库路径
if(BUILD_SHARED_LIBS)
    set(_lib_dir "$<TARGET_FILE_DIR:BlueprintRuntime>")
    if(WIN32)
        gtest_discover_tests(blueprint_tests
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            PROPERTIES ENVIRONMENT "PATH=${_lib_dir};$ENV{PATH}"
            DISCOVERY_MODE PRE_TEST
        )
    else()
        gtest_discover_tests(blueprint_tests
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${_lib_dir}:$ENV{LD_LIBRARY_PATH}"
            DISCOVERY_MODE PRE_TEST
        )
    endif()
else()
    gtest_discover_tests(blueprint_tests
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DISCOVERY_MODE PRE_TEST
    )
endif()

# 便捷目标：cmake --build ... --target run_tests
add_custom_target(run_tests
    COMMAND blueprint_tests --gtest_color=yes
    DEPENDS blueprint_tests
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Running BlueprintRuntime unit tests..."
)
