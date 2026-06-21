cmake_minimum_required(VERSION 3.10)
project(lua_socket)

message(STATUS "Configuring lua_socket... ${CMAKE_SOURCE_DIR}")
# 设置 C++11（或更高版本）
set(CMAKE_CXX_STANDARD 11)      # 可选：11, 14, 17, 20, 23
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 必须支持指定版本
#set(CMAKE_CXX_EXTENSIONS OFF)    # 禁用编译器扩展（如 GNU 的 -std=gnu++11）

file(GLOB luasocket_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)

if(TARGET lua54)
    # Use the FetchContent Lua 5.4 target directly — most reliable
    set(_LUASOCKET_LUA_TARGET lua54)
    get_target_property(_LUASOCKET_LUA_INC lua54 SOURCE_DIR)
    # SOURCE_DIR of the imported target is the FetchContent src dir
    # The actual headers are in <src>/src
    set(_LUASOCKET_LUA_INC "${CMAKE_BINARY_DIR}/_deps/lua54-src/src")
    if(NOT EXISTS "${_LUASOCKET_LUA_INC}/lua.h")
        # Fallback: try interface include directories
        get_target_property(_LUASOCKET_LUA_INC lua54 INTERFACE_INCLUDE_DIRECTORIES)
    endif()
elseif(LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/lua.h")
    set(_LUASOCKET_LUA_INC "${LUA_INCLUDE_DIR}")
    set(_LUASOCKET_LUA_TARGET "${LUA_LIBRARIES}")
else()
    find_package(Lua QUIET)
    if(NOT LUA_FOUND)
        message(WARNING "[luasocket] Lua not found — luasocket targets will not be built.")
        return()
    endif()
    set(_LUASOCKET_LUA_INC "${LUA_INCLUDE_DIR}")
    set(_LUASOCKET_LUA_TARGET "${LUA_LIBRARIES}")
endif()

message(STATUS "[luasocket] Using Lua include: ${_LUASOCKET_LUA_INC}")

add_library(luasocket_s STATIC ${luasocket_SOURCES})
#target_link_libraries(luasocket_s PRIVATE lua::lua)
set_target_properties(luasocket_s PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(luasocket_s PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${_LUASOCKET_LUA_INC}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(luasocket_s
    PRIVATE ${_LUASOCKET_LUA_TARGET}
)

target_compile_definitions(luasocket_s 
    PUBLIC
    LUASOCKET_STATIC
    WITH_LUASOCKET 
    WITH_LUASOCKET_STATIC
)
if(MSVC)
    target_compile_options(luasocket_s PRIVATE /W0 /utf-8)
else()
    target_compile_options(luasocket_s PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-parameter
    )
endif()
set_target_properties(luasocket_s PROPERTIES FOLDER "external")
if(WIN32)
    target_compile_definitions(luasocket_s PRIVATE 
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
    target_link_libraries(luasocket_s PUBLIC ws2_32)
endif()
# ---------------------------------------------------------------------------
# Shared library  (luasocket) — loadable as  require "luasocket"  from Lua
# Skipped on Emscripten (no shared libs) and iOS (no dylib plugins)
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN AND NOT (CMAKE_SYSTEM_NAME STREQUAL "iOS"))
    add_library(luasocket SHARED ${luasocket_SOURCES})
    #target_link_libraries(luasocket PRIVATE lua::lua)
    target_include_directories(luasocket PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${_LUASOCKET_LUA_INC}
    )

    target_link_libraries(luasocket
        PRIVATE ${_LUASOCKET_LUA_TARGET}
    )
    target_compile_definitions(luasocket 
        PUBLIC
        WITH_LUASOCKET 
        WITH_LUASOCKET_SHARED
    )
    target_compile_definitions(luasocket 
        PRIVATE
        LUASOCKET_SHARED
    )
    if(MSVC)
        target_compile_options(luasocket PRIVATE /W0 /utf-8)
    else()
        target_compile_options(luasocket PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-parameter
        )
    endif()
    if(WIN32)
        target_compile_definitions(luasocket PRIVATE 
            _WINSOCK_DEPRECATED_NO_WARNINGS
        )
        target_link_libraries(luasocket PRIVATE ws2_32)
    endif()

    # Output to  bin/{Debug,Release}/lua.plugins/
    set_target_properties(luasocket PROPERTIES
        FOLDER "external"
        LIBRARY_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin/lua.plugins"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug/lua.plugins"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release/lua.plugins"
        RUNTIME_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin/lua.plugins"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug/lua.plugins"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release/lua.plugins"
        PREFIX ""   # no "lib" prefix on Linux — Lua expects luasocket.so not libluasocket.so
    )

    message(STATUS "[luasocket] Shared library target added (luasocket)")
endif()
