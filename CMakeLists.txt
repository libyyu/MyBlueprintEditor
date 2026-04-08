# ==============================================================================
# MyBlueprintEditor – Root CMakeLists.txt
# ==============================================================================
#
# Key options
# -----------
#   BUILD_SHARED_LIBS       ON  → BlueprintRuntime built as shared (.dll/.so/.dylib)
#                           OFF → static (.lib/.a)  [default]
#
#   BUILD_RUNTIME_ONLY      ON  → build only BlueprintRuntime (no Editor, no Application)
#                                 Required/auto-set for Emscripten/WebGL
#                           OFF → also build Editor, Application, BlueprintEditor exe  [default]
#
#   BUILD_EXAMPLES          ON  → build example applications  [default]
#                           OFF → skip examples
#
#   BLUEPRINT_NO_FILESYSTEM ON  → exclude DefaultFileSystem from BlueprintRuntime
#                                 (callers must provide SetDefaultFileSystem())
#                           OFF [default]
#
# Typical builds
# --------------
#   # Windows – full build (static runtime + editor)
#   cmake -B build -A x64
#   cmake --build build --config Release
#
#   # Linux / macOS – shared library only
#   cmake -B build -DBUILD_SHARED_LIBS=ON -DBUILD_RUNTIME_ONLY=ON -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#
#   # Unity WebGL (Emscripten) – static runtime only
#   emcmake cmake -B build-wasm -DBUILD_RUNTIME_ONLY=ON -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-wasm
#   # → build-wasm/Runtime/libBlueprintRuntime.a  (drop into Assets/Plugins/WebGL/)
#
# ==============================================================================

cmake_minimum_required(VERSION 3.14)

project(MyBlueprintEditor VERSION 0.9.1 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(BUILD_SHARED_LIBS       "Build BlueprintRuntime as shared library (.dll/.so/.dylib)" ON)
option(BUILD_RUNTIME_ONLY      "Build only BlueprintRuntime (no Editor / Application)" OFF)
option(BUILD_EXAMPLES          "Build example applications" ON)
option(BLUEPRINT_NO_FILESYSTEM "Exclude DefaultFileSystem from BlueprintRuntime" OFF)

# Emscripten: force runtime-only + static
if(EMSCRIPTEN)
    set(BUILD_RUNTIME_ONLY TRUE)
    set(BUILD_SHARED_LIBS  FALSE CACHE BOOL "Always OFF for Emscripten" FORCE)
    message(STATUS "[MyBlueprintEditor] Emscripten build → BUILD_RUNTIME_ONLY=ON, BUILD_SHARED_LIBS=OFF")
endif()

# ---------------------------------------------------------------------------
# MSVC global options
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ImGui math operators — defined via target_compile_definitions(imgui PUBLIC ...) below.
# The guard in BlueprintEditor.h provides a safety net for standalone compilation.

# ---------------------------------------------------------------------------
# Project root dir (referenced by Runtime/CMakeLists.txt for Utils sources)
# ---------------------------------------------------------------------------
set(BLUEPRINT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------------
# crude_json  (lightweight JSON parser – used by Runtime and Editor)
# ---------------------------------------------------------------------------
add_subdirectory(Utils/Json)

# ---------------------------------------------------------------------------
# BlueprintRuntime  (always built)
# ---------------------------------------------------------------------------
add_subdirectory(Runtime)

# ---------------------------------------------------------------------------
# runtime-example — built only on desktop platforms (not iOS/Android/Emscripten)
#   Use:  runtime-example --test [flow_test.json]
# ---------------------------------------------------------------------------
set(_IS_MOBILE_PLATFORM FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR
   CMAKE_SYSTEM_NAME STREQUAL "Android" OR
   EMSCRIPTEN)
    set(_IS_MOBILE_PLATFORM TRUE)
endif()

if((BUILD_EXAMPLES OR BUILD_RUNTIME_ONLY) AND NOT _IS_MOBILE_PLATFORM)
    set(_RE_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/runtime-example/runtime-example.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/runtime-example/test_new_features.cpp
    )
    add_executable(runtime-example ${_RE_SRC})
    target_link_libraries(runtime-example PRIVATE BlueprintRuntime crude_json)
    target_include_directories(runtime-example PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Json
    )
    set_target_properties(runtime-example PROPERTIES
        FOLDER "Examples"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release"
    )
    message(STATUS "[runtime-example] target added (BUILD_RUNTIME_ONLY=${BUILD_RUNTIME_ONLY})")
endif()

# ---------------------------------------------------------------------------
# Editor / Application / BlueprintEditor exe  (skipped when runtime-only)
# ---------------------------------------------------------------------------
if(NOT BUILD_RUNTIME_ONLY)

    # ------------------------------------------------------------------
    # ImGui (static, always)
    # ------------------------------------------------------------------
    find_path(IMGUI_INCLUDE_DIR imgui.h
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        NO_DEFAULT_PATH
    )

    if(NOT IMGUI_INCLUDE_DIR)
        message(FATAL_ERROR "ImGui headers not found at external/imgui/")
    endif()

    message(STATUS "Found ImGui headers at: ${IMGUI_INCLUDE_DIR}")

    set(IMGUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    )

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_INCLUDE_DIR})
    target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
    if(MSVC)
        target_compile_options(imgui PRIVATE /W0)
    else()
        target_compile_options(imgui PRIVATE -w)
    endif()
    set_property(TARGET imgui PROPERTY FOLDER "Libraries")

    # ------------------------------------------------------------------
    # ImGuiColorTextEdit  (彩色文本编辑器，用于日志窗口选词 + 颜色)
    # ------------------------------------------------------------------
    add_library(ImGuiColorTextEdit STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuiColorTextEdit/TextEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuiColorTextEdit/LanguageDefinitions.cpp
    )
    # 用 SYSTEM 标记头文件目录，使编译器将其视为系统头文件，
    # 从而在 GCC/Clang 上屏蔽 -Wall/-Wextra 警告，
    # 在 MSVC 上触发 /external:I 语义（需 CMake ≥ 3.25 或手动追加选项）。
    target_include_directories(ImGuiColorTextEdit SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuiColorTextEdit
    )
    target_link_libraries(ImGuiColorTextEdit PUBLIC imgui)
    # MSVC 兼容：直接对该 target 关闭所有警告（/W0），
    # 因为 MSVC 的 /external:W0 在 CMake < 3.25 下不自动生效。
    if(MSVC)
        target_compile_options(ImGuiColorTextEdit PRIVATE /W0)
    else()
        target_compile_options(ImGuiColorTextEdit PRIVATE -w)
    endif()
    set_property(TARGET ImGuiColorTextEdit PROPERTY FOLDER "Libraries")

    # ------------------------------------------------------------------
    # ImGuiNodeEditor  (Editor core – depends on imgui + BlueprintRuntime)
    # ------------------------------------------------------------------
    set(CORE_SOURCES
        Editor/Core/imgui_node_editor.cpp
        Editor/Core/imgui_node_editor_api.cpp
        Editor/Core/imgui_canvas.cpp
    )

    set(CORE_HEADERS
        Editor/API/NodeEditor.h
        Editor/Core/imgui_node_editor.h
        Editor/Core/imgui_node_editor_internal.h
        Editor/Core/imgui_node_editor_internal.inl
        Editor/Core/imgui_canvas.h
        Editor/Core/imgui_bezier_math.h
        Editor/Core/imgui_bezier_math.inl
        Editor/Core/imgui_extra_math.h
        Editor/Core/imgui_extra_math.inl
    )

    set(UI_SOURCES
        UI/Builders/NodeBuilder.cpp
    )

    set(UI_HEADERS
        UI/Builders/NodeBuilder.h
    )

    add_library(ImGuiNodeEditor STATIC
        ${CORE_SOURCES} ${CORE_HEADERS}
        ${UI_SOURCES} ${UI_HEADERS}
    )

    # MSVC: attach .natvis files so Visual Studio debugger displays types nicely
    if(MSVC)
        target_sources(ImGuiNodeEditor PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/natvis/imgui_node_editor.natvis
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/natvis/crude_json.natvis
        )
    endif()

    target_include_directories(ImGuiNodeEditor
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Core
            ${CMAKE_CURRENT_SOURCE_DIR}/Editor
            ${CMAKE_CURRENT_SOURCE_DIR}/UI
    )

    target_link_libraries(ImGuiNodeEditor PUBLIC imgui BlueprintRuntime)

    set_target_properties(ImGuiNodeEditor PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
        FOLDER "Libraries"
    )

    if(MSVC)
        target_compile_options(ImGuiNodeEditor PRIVATE /W4)
    else()
        target_compile_options(ImGuiNodeEditor PRIVATE -Wall -Wextra)
    endif()

    # ------------------------------------------------------------------
    # Application framework  (platform abstraction + rendering backend)
    # ------------------------------------------------------------------

    # ------------------------------------------------------------------
    # Detect platform backends (GLFW + OpenGL for non-Windows)
    # ------------------------------------------------------------------
    set(HAVE_GLFW3  0)
    set(HAVE_OPENGL 0)

    if(WIN32)
        # Windows always uses Win32 + DX11; no GLFW/OpenGL needed
    else()
        find_package(glfw3 QUIET)
        find_package(OpenGL QUIET)
        if(glfw3_FOUND AND OpenGL_FOUND)
            set(HAVE_GLFW3  1)
            set(HAVE_OPENGL 1)
            message(STATUS "[Application] GLFW3 + OpenGL found → GLFW/OGL3 backend")
        else()
            if(NOT glfw3_FOUND)
                message(WARNING "[Application] glfw3 not found. Install: apt install libglfw3-dev / brew install glfw")
            endif()
            if(NOT OpenGL_FOUND)
                message(WARNING "[Application] OpenGL not found.")
            endif()
            message(FATAL_ERROR "[Application] GLFW3 and OpenGL are required on non-Windows platforms. "
                "Install them or pass -DBUILD_RUNTIME_ONLY=ON to skip the editor.")
        endif()
    endif()

    # Regenerate config.h with HAVE_GLFW3 / HAVE_OPENGL
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Application/source/config.h.in
        ${CMAKE_BINARY_DIR}/Application/config.h
        @ONLY
    )

    set(APPLICATION_SOURCES
        Application/include/application.h
        Application/source/application.cpp
        Application/source/entry_point.cpp
        Application/source/imgui_extra_keys.h
        Application/source/setup.h
        Application/source/platform.h
        Application/source/renderer.h
    )

    if(WIN32)
        list(APPEND APPLICATION_SOURCES
            Application/source/platform_win32.cpp
            Application/source/renderer_dx11.cpp
            Application/source/imgui_impl_dx11.cpp
            Application/source/imgui_impl_dx11.h
            Application/source/imgui_impl_win32.cpp
            Application/source/imgui_impl_win32.h
        )

        # DirectX SDK
        set(DXSDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/DXSDK)
        set(DXSDK_ARCH x86)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(DXSDK_ARCH x64)
        endif()

        add_library(dxerr STATIC ${DXSDK_DIR}/src/dxerr.cpp)
        target_include_directories(dxerr PUBLIC ${DXSDK_DIR}/include)
        set_property(TARGET dxerr PROPERTY FOLDER "External")

        add_library(d3dx11 UNKNOWN IMPORTED)
        set_target_properties(d3dx11 PROPERTIES
            IMPORTED_LOCATION       "${DXSDK_DIR}/lib/${DXSDK_ARCH}/d3dx11.lib"
            IMPORTED_LOCATION_DEBUG "${DXSDK_DIR}/lib/${DXSDK_ARCH}/d3dx11d.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${DXSDK_DIR}/include"
        )
    else()
        # Linux / macOS: GLFW + OpenGL 3
        list(APPEND APPLICATION_SOURCES
            Application/source/platform_glfw.cpp
            Application/source/renderer_ogl3.cpp
            Application/source/imgui_impl_glfw.cpp
            Application/source/imgui_impl_glfw.h
            Application/source/imgui_impl_opengl3.cpp
            Application/source/imgui_impl_opengl3.h
            Application/source/imgui_impl_opengl3_loader.h
        )
    endif()

    add_library(Application STATIC ${APPLICATION_SOURCES})

    target_include_directories(Application
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Application/include
        PRIVATE
            ${CMAKE_BINARY_DIR}/Application  # for config.h
            ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image
    )

    target_link_libraries(Application PUBLIC ImGuiNodeEditor)
    if(WIN32)
        target_link_libraries(Application PRIVATE d3d11.lib d3dcompiler.lib d3dx11 dxerr)
    else()
        target_link_libraries(Application PRIVATE glfw OpenGL::GL ${CMAKE_DL_LIBS})
    endif()

    set_target_properties(Application PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        FOLDER "Libraries"
    )

    # ------------------------------------------------------------------
    # BlueprintEditor executable
    # ------------------------------------------------------------------
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

    file(GLOB_RECURSE BLUEPRINT_EDITOR_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/BlueprintEditor/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BlueprintEditor/*.h"
    )

    if(WIN32)
        set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Application/support/Icon.ico")
        if(EXISTS "${APP_ICON}")
            set(ApplicationIcon "${APP_ICON}")
            configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/Application/support/Resource.rc.in
                ${CMAKE_BINARY_DIR}/BlueprintEditor_Resource.rc
            )
            list(APPEND BLUEPRINT_EDITOR_SOURCES ${CMAKE_BINARY_DIR}/BlueprintEditor_Resource.rc)
        endif()

        add_executable(BlueprintEditor WIN32 ${BLUEPRINT_EDITOR_SOURCES})
    else()
        add_executable(BlueprintEditor ${BLUEPRINT_EDITOR_SOURCES})
    endif()

    target_include_directories(BlueprintEditor
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintEditor
            ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintEditor/utilities
    )

    target_link_libraries(BlueprintEditor PRIVATE Application ImGuiColorTextEdit BlueprintRuntime)

    # Phase 3: 若 Runtime 开启了 Lua，Editor 也需要链接 Lua 库（LuaNodeRegistrar.cpp 使用 lua.hpp）
    # LuaNodeRegistrar.cpp 属于 BlueprintEditor，必须显式传递 BLUEPRINT_HAS_LUA 和 Lua include。
    # BLUEPRINT_HAS_LUA 虽然在 BlueprintRuntime PUBLIC，但经过多层静态库传递不保证到达编译单元，
    # 因此此处显式设置，确保 LuaNodeRegistrar.cpp 中的 #ifdef 生效。
    if(BLUEPRINT_LUA)
        target_compile_definitions(BlueprintEditor PRIVATE BLUEPRINT_HAS_LUA)
        if(TARGET lua54)
            # FetchContent 路径
            target_link_libraries(BlueprintEditor PRIVATE lua54)
        elseif(TARGET Lua::lua)
            # CMake 3.23+ find_package imported target
            target_link_libraries(BlueprintEditor PRIVATE Lua::lua)
        elseif(DEFINED LUA_LIBRARIES AND DEFINED LUA_INCLUDE_DIR)
            # find_package(Lua) 经典变量 或 pkg-config 路径
            target_include_directories(BlueprintEditor PRIVATE ${LUA_INCLUDE_DIR})
            target_link_libraries(BlueprintEditor PRIVATE ${LUA_LIBRARIES})
        endif()
    endif()

    set_target_properties(BlueprintEditor PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY           "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${OUTPUT_DIR}/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${OUTPUT_DIR}/Release"
        VS_DEBUGGER_WORKING_DIRECTORY      "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # ------------------------------------------------------------------
    # Resource copy
    # ------------------------------------------------------------------
    # 按配置分别复制到 bin/Debug/ 和 bin/Release/
    foreach(_CFG Debug Release)
        set(_COPY_RESOURCE_COMMANDS_${_CFG}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/${_CFG}/resource
        )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/BlueprintEditor/resource")
            file(GLOB _DATA_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/BlueprintEditor/resource/*")
            foreach(RES ${_DATA_RESOURCES})
                list(APPEND _COPY_RESOURCE_COMMANDS_${_CFG}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RES} ${OUTPUT_DIR}/${_CFG}/resource/
                )
            endforeach()
        endif()
        add_custom_target(copy_resources_${_CFG} ALL
            ${_COPY_RESOURCE_COMMANDS_${_CFG}}
            COMMENT "Copying resources to ${_CFG} output"
        )
    endforeach()

    add_dependencies(BlueprintEditor copy_resources_Debug copy_resources_Release)

    # 兼容别名：examples/CMakeLists.txt 引用 copy_resources
    add_custom_target(copy_resources DEPENDS copy_resources_Debug copy_resources_Release)

    # ------------------------------------------------------------------
    # Install – Editor targets
    # ------------------------------------------------------------------
    include(GNUInstallDirs)

    install(TARGETS ImGuiNodeEditor
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES ${CORE_HEADERS} ${UI_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ImGuiNodeEditor
    )

    install(TARGETS BlueprintEditor
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # ------------------------------------------------------------------
    # Examples
    # ------------------------------------------------------------------
    if(BUILD_EXAMPLES)
        message(STATUS "Building examples...")
        add_subdirectory(examples)
    endif()

endif() # NOT BUILD_RUNTIME_ONLY

# ---------------------------------------------------------------------------
# Unit tests (always available when not cross-compiling)
# ---------------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING
   AND NOT (CMAKE_SYSTEM_NAME STREQUAL "iOS")
   AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Android"))
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "[Tests] Unit tests enabled (run: ctest --test-dir ${CMAKE_BINARY_DIR})")
endif()
