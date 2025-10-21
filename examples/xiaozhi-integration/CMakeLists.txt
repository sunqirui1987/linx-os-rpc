# xiaozhi-esp32 LinxOS RPC 集成示例 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(xiaozhi_linxos_integration)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖包
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
pkg_check_modules(PROTOBUF_PKG protobuf)

# 尝试查找 gRPC，如果找不到则禁用 gRPC 功能
find_package(gRPC QUIET)
if(NOT gRPC_FOUND)
    message(WARNING "gRPC not found! Building without gRPC support.")
    set(ENABLE_GRPC OFF)
else()
    set(ENABLE_GRPC ON)
    message(STATUS "gRPC found, enabling gRPC support")
endif()

# 查找 jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../../src
    ${CMAKE_SOURCE_DIR}/../../proto
    ${CMAKE_SOURCE_DIR}/../../generated/cpp
    ${Protobuf_INCLUDE_DIRS}
)

# 生成 protobuf 文件
set(PROTO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../proto")
set(PROTO_FILES "${PROTO_PATH}/device.proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

# 生成 protobuf C++ 文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# 生成 gRPC C++ 文件（如果启用）
if(ENABLE_GRPC)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    if(NOT GRPC_CPP_PLUGIN)
        message(WARNING "grpc_cpp_plugin not found! Disabling gRPC functionality.")
        set(ENABLE_GRPC OFF)
        set(GRPC_SRCS "")
        set(GRPC_HDRS "")
    else()
        set(GRPC_SRCS "${GENERATED_PROTOBUF_PATH}/device.grpc.pb.cc")
        set(GRPC_HDRS "${GENERATED_PROTOBUF_PATH}/device.grpc.pb.h")

        add_custom_command(
            OUTPUT ${GRPC_SRCS} ${GRPC_HDRS}
            COMMAND protobuf::protoc
            ARGS --grpc_out=${GENERATED_PROTOBUF_PATH}
                 --cpp_out=${GENERATED_PROTOBUF_PATH}
                 --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
                 -I${PROTO_PATH}
                 ${PROTO_FILES}
            DEPENDS ${PROTO_FILES}
            COMMENT "Generating gRPC C++ files"
        )
    endif()
else()
    set(GRPC_SRCS "")
    set(GRPC_HDRS "")
endif()

# 源文件
set(SOURCES
    xiaozhi_linxos_integration.cpp
    client_simple.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# 添加库目录
if(JSONCPP_LIBRARY_DIRS)
    link_directories(${JSONCPP_LIBRARY_DIRS})
endif()
if(PROTOBUF_PKG_LIBRARY_DIRS)
    link_directories(${PROTOBUF_PKG_LIBRARY_DIRS})
endif()

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
set(LINK_LIBRARIES
    ${PROTOBUF_PKG_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
)

if(ENABLE_GRPC)
    list(APPEND LINK_LIBRARIES gRPC::grpc++)
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -O2
)

# 包含生成的文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GENERATED_PROTOBUF_PATH}
    ${JSONCPP_INCLUDE_DIRS}
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Protobuf version: ${Protobuf_VERSION}")
message(STATUS "gRPC found: ${ENABLE_GRPC}")
message(STATUS "JsonCpp libraries: ${JSONCPP_LIBRARIES}")
message(STATUS "JsonCpp library dirs: ${JSONCPP_LIBRARY_DIRS}")
message(STATUS "JsonCpp include dirs: ${JSONCPP_INCLUDE_DIRS}")