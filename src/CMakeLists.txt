cmake_minimum_required(VERSION 3.16)

# LinxOS RPC 核心库
project(linxos_rpc_core VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖包
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# 查找gRPC和protobuf
find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)

# 查找JsonCpp
pkg_check_modules(JSONCPP jsoncpp REQUIRED)

# 设置protobuf和gRPC路径
set(PROTO_PATH "${CMAKE_SOURCE_DIR}/../examples/golang-test-server/proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated")

# 创建生成目录
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

# protobuf文件
set(PROTO_FILES
    ${PROTO_PATH}/device.proto
)

# 生成protobuf和gRPC文件
set(PROTO_SRCS "${GENERATED_PROTOBUF_PATH}/device.pb.cc")
set(PROTO_HDRS "${GENERATED_PROTOBUF_PATH}/device.pb.h")
set(GRPC_SRCS "${GENERATED_PROTOBUF_PATH}/device.grpc.pb.cc")
set(GRPC_HDRS "${GENERATED_PROTOBUF_PATH}/device.grpc.pb.h")

# 自定义命令生成protobuf文件
add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND protobuf::protoc
    ARGS --grpc_out=${GENERATED_PROTOBUF_PATH}
         --cpp_out=${GENERATED_PROTOBUF_PATH}
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         -I${PROTO_PATH}
         ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Generating gRPC C++ files"
)

# 核心库源文件
set(LINXOS_RPC_SOURCES
    linxos_rpc/client.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# 核心库头文件
set(LINXOS_RPC_HEADERS
    linxos_rpc/client.h
    ${PROTO_HDRS}
    ${GRPC_HDRS}
)

# 创建静态库
add_library(linxos_rpc_static STATIC ${LINXOS_RPC_SOURCES})

# 创建动态库
add_library(linxos_rpc_shared SHARED ${LINXOS_RPC_SOURCES})

# 设置库的别名
add_library(linxos_rpc::static ALIAS linxos_rpc_static)
add_library(linxos_rpc::shared ALIAS linxos_rpc_shared)

# 设置库的属性
set_target_properties(linxos_rpc_static PROPERTIES
    OUTPUT_NAME linxos_rpc
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties(linxos_rpc_shared PROPERTIES
    OUTPUT_NAME linxos_rpc
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 包含目录
target_include_directories(linxos_rpc_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${GENERATED_PROTOBUF_PATH}>
    $<INSTALL_INTERFACE:include>
    ${JSONCPP_INCLUDE_DIRS}
)

target_include_directories(linxos_rpc_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${GENERATED_PROTOBUF_PATH}>
    $<INSTALL_INTERFACE:include>
    ${JSONCPP_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(linxos_rpc_static PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

target_link_libraries(linxos_rpc_shared PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

# 编译选项
target_compile_options(linxos_rpc_static PRIVATE
    -Wall
    -Wextra
    -O2
)

target_compile_options(linxos_rpc_shared PRIVATE
    -Wall
    -Wextra
    -O2
)

# 安装规则
install(TARGETS linxos_rpc_static linxos_rpc_shared
    EXPORT linxos_rpc_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(FILES linxos_rpc/client.h
    DESTINATION include/linxos_rpc
)

# 安装生成的protobuf头文件
install(FILES ${PROTO_HDRS} ${GRPC_HDRS}
    DESTINATION include/linxos_rpc
)

# 导出目标
install(EXPORT linxos_rpc_targets
    FILE linxos_rpc_targets.cmake
    NAMESPACE linxos_rpc::
    DESTINATION lib/cmake/linxos_rpc
)

# 创建配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/linxos_rpc_config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/linxos_rpc_config.cmake"
    INSTALL_DESTINATION lib/cmake/linxos_rpc
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/linxos_rpc_config_version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/linxos_rpc_config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/linxos_rpc_config_version.cmake"
    DESTINATION lib/cmake/linxos_rpc
)

# 打印配置信息
message(STATUS "LinxOS RPC Core Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Protobuf version: ${Protobuf_VERSION}")
message(STATUS "  gRPC found: ${gRPC_FOUND}")
message(STATUS "  JsonCpp libraries: ${JSONCPP_LIBRARIES}")
message(STATUS "  Generated files path: ${GENERATED_PROTOBUF_PATH}")