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
# Google Test（FetchContent 自动下载，仅在测试目标被构建时触发）
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL      https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
# 禁止 gtest 安装到系统目录
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# 测试可执行文件
# ---------------------------------------------------------------------------
add_executable(blueprint_tests
    test_registry.cpp
    test_serialization.cpp
    test_topology.cpp
    test_handlers.cpp
)

target_link_libraries(blueprint_tests
    PRIVATE
        BlueprintRuntime
        crude_json
        GTest::gtest_main
)

target_include_directories(blueprint_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/Utils/Json
)

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
