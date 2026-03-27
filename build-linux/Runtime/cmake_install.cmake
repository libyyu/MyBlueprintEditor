# Install script for directory: /root/.openclaw/workspace/MyBlueprintEditor/Runtime

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE STATIC_LIBRARY FILES "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Runtime/libBlueprintRuntime.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE STATIC_LIBRARY FILES "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Utils/Json/libcrude_json.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/BlueprintRuntime" TYPE FILE FILES
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BlueprintExport.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BlueprintCAPI.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/Types.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/NodeDefinition.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BlueprintData.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BlueprintExporter.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BlueprintRunner.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/FrameTimerManager.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/MainThreadDispatcher.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BuiltinNodeDefs.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/BuiltinHandlers.h"
    "/root/.openclaw/workspace/MyBlueprintEditor/Runtime/FileSystem.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/crude_json" TYPE FILE FILES "/root/.openclaw/workspace/MyBlueprintEditor/Utils/Json/crude_json.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime/BlueprintRuntimeTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime/BlueprintRuntimeTargets.cmake"
         "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Runtime/CMakeFiles/Export/09b3a4a7e33cdbb8ffa40fb28680bd44/BlueprintRuntimeTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime/BlueprintRuntimeTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime/BlueprintRuntimeTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime" TYPE FILE FILES "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Runtime/CMakeFiles/Export/09b3a4a7e33cdbb8ffa40fb28680bd44/BlueprintRuntimeTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime" TYPE FILE FILES "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Runtime/CMakeFiles/Export/09b3a4a7e33cdbb8ffa40fb28680bd44/BlueprintRuntimeTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/cmake/BlueprintRuntime" TYPE FILE FILES
    "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Runtime/BlueprintRuntimeConfig.cmake"
    "/root/.openclaw/workspace/MyBlueprintEditor/build-linux/Runtime/BlueprintRuntimeConfigVersion.cmake"
    )
endif()

