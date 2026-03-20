# ImGui Node Editor - CMake 配置
cmake_minimum_required(VERSION 3.12)
project(ImGuiNodeEditor VERSION 0.9.1 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Json
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    # 模块包含目录
    ${CMAKE_CURRENT_SOURCE_DIR}/Runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/UI
    ${CMAKE_CURRENT_SOURCE_DIR}/Editor
)

# ============================================================================
# 模块化源文件配置
# ============================================================================

# Runtime 模块（新增）
set(RUNTIME_SOURCES
    Runtime/BlueprintExporter.cpp
    Runtime/BlueprintRunner.cpp
)

set(RUNTIME_HEADERS
    Runtime/Types.h
    Runtime/NodeDefinition.h
    Runtime/BlueprintData.h
    Runtime/BlueprintExporter.h
    Runtime/BlueprintRunner.h
)

# UI 模块（新增）
set(UI_SOURCES
    UI/Builders/NodeBuilder.cpp
)

set(UI_HEADERS
    UI/Canvas/Canvas.h
    UI/Drawing/DrawHelpers.h
    UI/Builders/NodeBuilder.h
)

# Editor 模块（新增包装层）
set(EDITOR_HEADERS
    Editor/API/NodeEditor.h
)

# JSON 工具源文件（位于 Utils/Json/ 目录）
set(JSON_SOURCES
    Utils/Json/crude_json.cpp
)

set(JSON_HEADERS
    Utils/Json/crude_json.h
)

# 编辑器核心源文件（位于 Editor/Core/ 目录）
set(CORE_SOURCES
    Editor/Core/imgui_node_editor.cpp
    Editor/Core/imgui_node_editor_api.cpp
    Editor/Core/imgui_canvas.cpp
)

set(CORE_HEADERS
    Editor/Core/imgui_node_editor.h
    Editor/Core/imgui_node_editor_internal.h
    Editor/Core/imgui_node_editor_internal.inl
    Editor/Core/imgui_canvas.h
    Editor/Core/imgui_bezier_math.h
    Editor/Core/imgui_bezier_math.inl
    Editor/Core/imgui_extra_math.h
    Editor/Core/imgui_extra_math.inl
)

# 合并所有源文件
set(IMGUI_NODE_EDITOR_SOURCES
    ${RUNTIME_SOURCES}
    ${UI_SOURCES}
    ${JSON_SOURCES}
    ${CORE_SOURCES}
)

set(IMGUI_NODE_EDITOR_HEADERS
    ${RUNTIME_HEADERS}
    ${UI_HEADERS}
    ${EDITOR_HEADERS}
    ${JSON_HEADERS}
    ${CORE_HEADERS}
)

# 创建静态库
add_library(ImGuiNodeEditor STATIC ${IMGUI_NODE_EDITOR_SOURCES} ${IMGUI_NODE_EDITOR_HEADERS})

# 设置库属性
set_target_properties(ImGuiNodeEditor PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# 启用 ImGui 数学运算符
target_compile_definitions(ImGuiNodeEditor PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# 查找ImGui库
find_path(IMGUI_INCLUDE_DIR imgui.h
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    NO_DEFAULT_PATH
)

if(IMGUI_INCLUDE_DIR)
    message(STATUS "Found ImGui headers at: ${IMGUI_INCLUDE_DIR}")
    
    # 添加ImGui源文件
    set(IMGUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    )
    
    # 创建ImGui静态库
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
    
    # 链接依赖
    target_link_libraries(ImGuiNodeEditor PRIVATE imgui)
else()
    message(WARNING "ImGui headers not found, building without ImGui support")
endif()

# 编译器警告设置
if(MSVC)
    target_compile_options(ImGuiNodeEditor PRIVATE /W4 /utf-8)
else()
    target_compile_options(ImGuiNodeEditor PRIVATE -Wall -Wextra)
endif()

# 发布模式优化
if(MSVC)
    target_compile_options(ImGuiNodeEditor PRIVATE $<$<CONFIG:Release>:/O2>)
else()
    target_compile_options(ImGuiNodeEditor PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

# 安装配置（可选）
install(TARGETS ImGuiNodeEditor
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${CORE_HEADERS} ${RUNTIME_HEADERS} ${UI_HEADERS} ${EDITOR_HEADERS} ${JSON_HEADERS}
    DESTINATION include/ImGuiNodeEditor
)

# ============================================================================
# 编译示例程序（可选）
# ============================================================================
option(BUILD_EXAMPLES "Build example applications" ON)

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    
    # 设置示例程序需要的变量
    set(IMGUI_NODE_EDITOR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    
    # 添加示例程序（使用简化配置）
    add_subdirectory(examples)
endif()