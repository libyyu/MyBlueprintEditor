cmake_minimum_required(VERSION 3.12)

# ============================================================================
# 示例程序（简单演示，不含 BlueprintEditor）
# ============================================================================
#
# BlueprintEditor 和 Application 框架已提升为顶级构建目标，
# 由根 CMakeLists.txt 直接管理。此文件仅构建轻量级示例。

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# 包含目录已通过 target_link_libraries → Application → ImGuiNodeEditor 传播，
# 无需额外 include_directories()。

# 输出目录（与 BlueprintEditor 共用）
set(EXAMPLE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

# ============================================================================
# 辅助宏：创建示例程序
# ============================================================================

macro(add_simple_example NAME)
    project(${NAME})

    file(GLOB_RECURSE EXAMPLE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.h"
    )

    if(WIN32)
        set(APP_ICON "${IMGUI_NODE_EDITOR_ROOT_DIR}/Application/support/Icon.ico")

        if(EXISTS "${APP_ICON}")
            set(ApplicationIcon "${APP_ICON}")
            configure_file(
                ${IMGUI_NODE_EDITOR_ROOT_DIR}/Application/support/Resource.rc.in
                ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_Resource.rc
            )
            list(APPEND EXAMPLE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_Resource.rc)
        endif()

        add_executable(${NAME} WIN32 ${EXAMPLE_SOURCES})
    else()
        add_executable(${NAME} ${EXAMPLE_SOURCES})
    endif()

    # 链接 Application（传递 ImGuiNodeEditor -> imgui + BlueprintRuntime）
    target_link_libraries(${NAME} PRIVATE Application)

    set_target_properties(${NAME} PROPERTIES
        FOLDER "Examples"
        RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLE_OUTPUT_DIR}"
    )

    add_dependencies(${NAME} copy_resources)
endmacro()

# 添加示例程序
add_simple_example(simple-example)
add_simple_example(canvas-example)
add_simple_example(widgets-example)
add_simple_example(basic-interaction-example)

# ============================================================================
# 运行时示例（纯控制台程序，只依赖 BlueprintRuntime，不依赖 ImGui/编辑器）
# ============================================================================

# ============================================================================
# 运行时示例（纯控制台程序，只依赖 BlueprintRuntime，不依赖 ImGui/编辑器）
# 注意：runtime-example 由根 CMakeLists.txt 无条件定义（BUILD_RUNTIME_ONLY 也会构建），
#       此处仅在尚未定义时才补充定义，避免重复 target 冲突。
# ============================================================================
if(NOT TARGET runtime-example)
    project(runtime-example)
    add_executable(runtime-example
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime-example/runtime-example.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime-example/test_new_features.cpp
    )
    target_link_libraries(runtime-example PRIVATE BlueprintRuntime crude_json)
    target_include_directories(runtime-example PRIVATE
        ${CMAKE_SOURCE_DIR}/Utils/Json   # crude_json.h
    )
    set_target_properties(runtime-example PROPERTIES
        FOLDER "Examples"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLE_OUTPUT_DIR}"
    )
endif()

message(STATUS "Examples will be built in: ${EXAMPLE_OUTPUT_DIR}")
