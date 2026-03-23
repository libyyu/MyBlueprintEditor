cmake_minimum_required(VERSION 3.12)

# 简化的示例构建配置
# 直接使用主项目中已编译的库:
#   - BlueprintRuntime: 独立运行时库（无 ImGui 依赖）
#   - ImGuiNodeEditor:  编辑器库（依赖 ImGui + BlueprintRuntime）
#   - imgui:            ImGui 库

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ============================================================================
# 公共配置
# ============================================================================

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# MSVC: treat source/execution charset as UTF-8 to avoid C4819 and encoding issues
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 包含目录
include_directories(
    ${IMGUI_NODE_EDITOR_ROOT_DIR}
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/Editor/Core
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/Utils/Json
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/external/imgui
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/external/stb_image
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/Runtime
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/Editor
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/UI
    ${IMGUI_NODE_EDITOR_ROOT_DIR}/Utils
    ${CMAKE_BINARY_DIR}/examples  # for config.h
)

# 启用 ImGui 数学运算符
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)

# 输出目录
set(EXAMPLE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

# ============================================================================
# Application 库（示例框架）
# ============================================================================

project(application)

# 生成 config.h
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application/source/config.h.in
    ${CMAKE_BINARY_DIR}/examples/config.h
    @ONLY
)

set(APPLICATION_SOURCES
    application/include/application.h
    application/source/application.cpp
    application/source/entry_point.cpp
    application/source/imgui_extra_keys.h
    application/source/setup.h
    application/source/platform.h
    application/source/platform_win32.cpp
    application/source/renderer.h
    application/source/renderer_dx11.cpp
)

if(WIN32)
    list(APPEND APPLICATION_SOURCES
        application/source/imgui_impl_dx11.cpp
        application/source/imgui_impl_dx11.h
        application/source/imgui_impl_win32.cpp
        application/source/imgui_impl_win32.h
    )
    
    # DirectX SDK
    set(DXSDK_DIR ${IMGUI_NODE_EDITOR_ROOT_DIR}/external/DXSDK)
    set(DXSDK_ARCH x86)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(DXSDK_ARCH x64)
    endif()
    
    add_library(dxerr STATIC ${DXSDK_DIR}/src/dxerr.cpp)
    target_include_directories(dxerr PUBLIC ${DXSDK_DIR}/include)
    
    add_library(d3dx11 UNKNOWN IMPORTED)
    set_target_properties(d3dx11 PROPERTIES
        IMPORTED_LOCATION "${DXSDK_DIR}/lib/${DXSDK_ARCH}/d3dx11.lib"
        IMPORTED_LOCATION_DEBUG "${DXSDK_DIR}/lib/${DXSDK_ARCH}/d3dx11d.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${DXSDK_DIR}/include"
    )
endif()

add_library(application STATIC ${APPLICATION_SOURCES})
target_include_directories(application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/include)

# application 链接 ImGuiNodeEditor（会传递链接 imgui 和 BlueprintRuntime）
target_link_libraries(application PUBLIC ImGuiNodeEditor)
if(WIN32)
    target_link_libraries(application PRIVATE d3d11.lib d3dcompiler.lib d3dx11 dxerr)
endif()

set_property(TARGET application PROPERTY FOLDER "examples")

# ============================================================================
# 示例程序
# ============================================================================

# ============================================================================
# 资源复制 target（独立 target，避免并行构建时的竞态冲突）
# ============================================================================

set(_COPY_RESOURCE_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLE_OUTPUT_DIR}/data
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB _COMMON_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    foreach(RES ${_COMMON_RESOURCES})
        list(APPEND _COPY_RESOURCE_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RES} ${EXAMPLE_OUTPUT_DIR}/data/
        )
    endforeach()
endif()

# blueprints-example 特有的纹理资源
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/blueprints-example/data")
    file(GLOB _BP_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/blueprints-example/data/*")
    foreach(RES ${_BP_RESOURCES})
        list(APPEND _COPY_RESOURCE_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RES} ${EXAMPLE_OUTPUT_DIR}/data/
        )
    endforeach()
endif()

add_custom_target(copy_resources ALL
    ${_COPY_RESOURCE_COMMANDS}
    COMMENT "Copying example resources to output directory"
)
set_property(TARGET copy_resources PROPERTY FOLDER "examples")

# ============================================================================
# 辅助宏：创建示例程序
# ============================================================================

macro(add_simple_example NAME)
    project(${NAME})
    
    # 递归搜索所有源文件
    file(GLOB_RECURSE EXAMPLE_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.h"
    )
    
    if(WIN32)
        # Windows 图标
        set(APP_ICON "${IMGUI_NODE_EDITOR_ROOT_DIR}/examples/application/support/Icon.ico")
        
        if(EXISTS "${APP_ICON}")
            # 使用 @only 模式配置资源文件
            set(ApplicationIcon "${APP_ICON}")
            
            configure_file(
                ${IMGUI_NODE_EDITOR_ROOT_DIR}/examples/application/support/Resource.rc.in
                ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_Resource.rc
            )
            list(APPEND EXAMPLE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_Resource.rc)
        endif()
        
        add_executable(${NAME} WIN32 ${EXAMPLE_SOURCES})
    else()
        add_executable(${NAME} ${EXAMPLE_SOURCES})
    endif()
    
    # 链接 application（传递 ImGuiNodeEditor -> imgui + BlueprintRuntime）
    target_link_libraries(${NAME} PRIVATE application)
    
    set_target_properties(${NAME} PROPERTIES
        FOLDER "examples"
        RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLE_OUTPUT_DIR}"
    )
    
    # 依赖资源复制 target
    add_dependencies(${NAME} copy_resources)
endmacro()

# 添加示例程序
add_simple_example(simple-example)
add_simple_example(canvas-example)
add_simple_example(widgets-example)
add_simple_example(basic-interaction-example)
add_simple_example(blueprints-example)

# ============================================================================
# 运行时示例（纯控制台程序，只依赖 BlueprintRuntime，不依赖 ImGui/编辑器）
# ============================================================================

project(runtime-example)
add_executable(runtime-example
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime-example/runtime-example.cpp
)
# 只链接 BlueprintRuntime（不需要 ImGui 和 ImGuiNodeEditor）
target_link_libraries(runtime-example PRIVATE BlueprintRuntime)
set_target_properties(runtime-example PROPERTIES
    FOLDER "examples"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EXAMPLE_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EXAMPLE_OUTPUT_DIR}"
)

message(STATUS "Examples will be built in: ${EXAMPLE_OUTPUT_DIR}")
