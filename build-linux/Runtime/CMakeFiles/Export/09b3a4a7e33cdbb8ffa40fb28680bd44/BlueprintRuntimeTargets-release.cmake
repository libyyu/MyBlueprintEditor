#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "BlueprintRuntime::BlueprintRuntime" for configuration "Release"
set_property(TARGET BlueprintRuntime::BlueprintRuntime APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(BlueprintRuntime::BlueprintRuntime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib64/libBlueprintRuntime.a"
  )

list(APPEND _cmake_import_check_targets BlueprintRuntime::BlueprintRuntime )
list(APPEND _cmake_import_check_files_for_BlueprintRuntime::BlueprintRuntime "${_IMPORT_PREFIX}/lib64/libBlueprintRuntime.a" )

# Import target "BlueprintRuntime::crude_json" for configuration "Release"
set_property(TARGET BlueprintRuntime::crude_json APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(BlueprintRuntime::crude_json PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib64/libcrude_json.a"
  )

list(APPEND _cmake_import_check_targets BlueprintRuntime::crude_json )
list(APPEND _cmake_import_check_files_for_BlueprintRuntime::crude_json "${_IMPORT_PREFIX}/lib64/libcrude_json.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
