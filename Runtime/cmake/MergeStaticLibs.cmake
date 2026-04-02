# MergeStaticLibs.cmake
# Called by the BlueprintBundle custom target on Linux / Android NDK.
#
# Expected variables (passed via -D):
#   OUTPUT    – path of the output .a
#   INPUTS    – semicolon-separated list of input .a files
#   MRI_FILE  – path for the temporary MRI script
#   AR        – path to ar (CMAKE_AR)

cmake_minimum_required(VERSION 3.12)

# Build MRI script content
set(_mri "CREATE ${OUTPUT}\n")
foreach(_lib IN LISTS INPUTS)
    string(APPEND _mri "ADDLIB ${_lib}\n")
endforeach()
string(APPEND _mri "SAVE\nEND\n")

# Write the MRI file
file(WRITE "${MRI_FILE}" "${_mri}")

# Run ar with the MRI script
execute_process(
    COMMAND ${AR} -M
    INPUT_FILE "${MRI_FILE}"
    RESULT_VARIABLE _ar_result
    ERROR_VARIABLE  _ar_err
)

if(NOT _ar_result EQUAL 0)
    message(FATAL_ERROR "ar merge failed: ${_ar_err}")
endif()

message(STATUS "BlueprintBundle merged → ${OUTPUT}")
