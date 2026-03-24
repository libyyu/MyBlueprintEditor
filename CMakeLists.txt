# ImGui Node Editor - CMake 配置
cmake_minimum_required(VERSION 3.12)
project(ImGuiNodeEditor VERSION 0.9.1 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# BlueprintRuntime 库（独立于 ImGui，可提供给第三方）
# ============================================================================

set(RUNTIME_SOURCES
    Runtime/BlueprintExporter.cpp
    Runtime/BlueprintRunner.cpp
    Runtime/FrameTimerManager.cpp
    Runtime/BuiltinNodeDefs.cpp
    Runtime/BuiltinHandlers.cpp
    Utils/Json/crude_json.cpp
)

set(RUNTIME_HEADERS
    Runtime/Types.h
    Runtime/NodeDefinition.h
    Runtime/BlueprintData.h
    Runtime/BlueprintExporter.h
    Runtime/BlueprintRunner.h
    Runtime/FrameTimerManager.h
    Runtime/BuiltinNodeDefs.h
    Runtime/BuiltinHandlers.h
    Utils/Json/crude_json.h
)

add_library(BlueprintRuntime STATIC ${RUNTIME_SOURCES} ${RUNTIME_HEADERS})

target_include_directories(BlueprintRuntime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Json
)

set_target_properties(BlueprintRuntime PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# 编译器警告设置
if(MSVC)
    target_compile_options(BlueprintRuntime PRIVATE /W4 /utf-8)
else()
    target_compile_options(BlueprintRuntime PRIVATE -Wall -Wextra)
endif()

# 发布模式优化
if(MSVC)
    target_compile_options(BlueprintRuntime PRIVATE $<$<CONFIG:Release>:/O2>)
else()
    target_compile_options(BlueprintRuntime PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

# ============================================================================
# ImGuiNodeEditor 库（编辑器核心，依赖 ImGui + BlueprintRuntime）
# ============================================================================

# UI 模块
set(UI_SOURCES
    UI/Builders/NodeBuilder.cpp
)

set(UI_HEADERS
    UI/Builders/NodeBuilder.h
)

# Editor 模块
set(EDITOR_HEADERS
    Editor/API/NodeEditor.h
)

# 编辑器核心
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

set(IMGUI_NODE_EDITOR_SOURCES
    ${UI_SOURCES}
    ${CORE_SOURCES}
)

set(IMGUI_NODE_EDITOR_HEADERS
    ${UI_HEADERS}
    ${EDITOR_HEADERS}
    ${CORE_HEADERS}
)

add_library(ImGuiNodeEditor STATIC ${IMGUI_NODE_EDITOR_SOURCES} ${IMGUI_NODE_EDITOR_HEADERS})

target_include_directories(ImGuiNodeEditor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Editor
        ${CMAKE_CURRENT_SOURCE_DIR}/UI
)

set_target_properties(ImGuiNodeEditor PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# 启用 ImGui 数学运算符
target_compile_definitions(ImGuiNodeEditor PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# 查找 ImGui 库
find_path(IMGUI_INCLUDE_DIR imgui.h
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    NO_DEFAULT_PATH
)

if(IMGUI_INCLUDE_DIR)
    message(STATUS "Found ImGui headers at: ${IMGUI_INCLUDE_DIR}")
    
    # 添加 ImGui 源文件
    set(IMGUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    )
    
    # 创建 ImGui 静态库
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
    
    # ImGuiNodeEditor 链接 imgui + BlueprintRuntime
    target_link_libraries(ImGuiNodeEditor PUBLIC imgui BlueprintRuntime)
else()
    message(WARNING "ImGui headers not found, building without ImGui support")
    # 即便没有 ImGui，仍然链接 BlueprintRuntime
    target_link_libraries(ImGuiNodeEditor PUBLIC BlueprintRuntime)
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

# ============================================================================
# 安装配置
# ============================================================================

# 安装 BlueprintRuntime
install(TARGETS BlueprintRuntime
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${RUNTIME_HEADERS}
    DESTINATION include/BlueprintRuntime
)

# 安装 ImGuiNodeEditor
install(TARGETS ImGuiNodeEditor
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${CORE_HEADERS} ${UI_HEADERS} ${EDITOR_HEADERS}
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
    
    # 添加示例程序
    add_subdirectory(examples)
endif()
