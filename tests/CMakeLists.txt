cmake_minimum_required(VERSION 3.12)
project(BlueprintRuntimeTests CXX)

add_executable(runtime_test runtime_test.cpp)

target_link_libraries(runtime_test PRIVATE BlueprintRuntime)

target_include_directories(runtime_test PRIVATE
    ${CMAKE_SOURCE_DIR}/Runtime
    ${CMAKE_SOURCE_DIR}/Utils
    ${CMAKE_SOURCE_DIR}/Utils/Json
)

# 把测试蓝图 JSON 拷贝到构建目录，方便运行时读取
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/flow_test.json
    ${CMAKE_CURRENT_BINARY_DIR}/flow_test.json
    COPYONLY
)
