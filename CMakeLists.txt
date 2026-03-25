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

project(MyBlueprintEditor VERSION 0.9.1 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
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

# ImGui math operators (global, needed by Editor and Application)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)

# ---------------------------------------------------------------------------
# Project root dir (referenced by Runtime/CMakeLists.txt for Utils sources)
# ---------------------------------------------------------------------------
set(BLUEPRINT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------------
# BlueprintRuntime  (always built)
# ---------------------------------------------------------------------------
add_subdirectory(Runtime)

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
    target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
    set_property(TARGET imgui PROPERTY FOLDER "Libraries")

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

    target_include_directories(ImGuiNodeEditor
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/Editor/Core
            ${CMAKE_CURRENT_SOURCE_DIR}/Editor
            ${CMAKE_CURRENT_SOURCE_DIR}/UI
    )

    target_link_libraries(ImGuiNodeEditor PUBLIC imgui BlueprintRuntime)

    set_target_properties(ImGuiNodeEditor PROPERTIES
        CXX_STANDARD 14
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
        Application/source/platform_win32.cpp
        Application/source/renderer.h
        Application/source/renderer_dx11.cpp
    )

    if(WIN32)
        list(APPEND APPLICATION_SOURCES
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
    endif()

    set_target_properties(Application PROPERTIES
        CXX_STANDARD 14
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

    target_link_libraries(BlueprintEditor PRIVATE Application)

    set_target_properties(BlueprintEditor PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY           "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${OUTPUT_DIR}"
        VS_DEBUGGER_WORKING_DIRECTORY      "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # ------------------------------------------------------------------
    # Resource copy
    # ------------------------------------------------------------------
    set(_COPY_RESOURCE_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}/data
    )

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
        file(GLOB _DATA_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
        foreach(RES ${_DATA_RESOURCES})
            list(APPEND _COPY_RESOURCE_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RES} ${OUTPUT_DIR}/data/
            )
        endforeach()
    endif()

    add_custom_target(copy_resources ALL
        ${_COPY_RESOURCE_COMMANDS}
        COMMENT "Copying resources to output directory"
    )

    add_dependencies(BlueprintEditor copy_resources)

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
