# LinxOS RPC 项目主构建文件
cmake_minimum_required(VERSION 3.10)

project(linxos-rpc VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 查找依赖
find_package(PkgConfig REQUIRED)

# 检查protobuf
pkg_check_modules(PROTOBUF_PKG REQUIRED protobuf)
if(PROTOBUF_PKG_FOUND)
    message(STATUS "Found protobuf: ${PROTOBUF_PKG_VERSION}")
else()
    message(FATAL_ERROR "protobuf not found")
endif()

# 检查gRPC（可选）
pkg_check_modules(GRPC_PKG grpc++)
if(GRPC_PKG_FOUND)
    message(STATUS "Found gRPC: ${GRPC_PKG_VERSION}")
    set(HAVE_GRPC TRUE)
else()
    message(WARNING "gRPC not found, will use LiteGRPC as alternative")
    set(HAVE_GRPC FALSE)
endif()

# 全局包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/generated/cpp)
include_directories(${PROTOBUF_PKG_INCLUDE_DIRS})

# 全局链接目录
link_directories(${PROTOBUF_PKG_LIBRARY_DIRS})
if(HAVE_GRPC)
    link_directories(${GRPC_PKG_LIBRARY_DIRS})
endif()

# 生成protobuf文件
set(PROTO_FILES
    proto/device.proto
)

set(PROTO_SOURCES)
set(PROTO_HEADERS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_SRC "${CMAKE_SOURCE_DIR}/generated/cpp/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${CMAKE_SOURCE_DIR}/generated/cpp/${PROTO_NAME}.pb.h")
    
    list(APPEND PROTO_SOURCES ${PROTO_SRC})
    list(APPEND PROTO_HEADERS ${PROTO_HDR})
    
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/generated/cpp
        COMMAND protoc --cpp_out=${CMAKE_SOURCE_DIR}/generated/cpp 
                       --proto_path=${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
        DEPENDS ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
        COMMENT "Generating protobuf files for ${PROTO_FILE}"
    )
endforeach()

# 如果有gRPC，也生成gRPC文件
if(HAVE_GRPC)
    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
        set(GRPC_SRC "${CMAKE_SOURCE_DIR}/generated/cpp/${PROTO_NAME}.grpc.pb.cc")
        set(GRPC_HDR "${CMAKE_SOURCE_DIR}/generated/cpp/${PROTO_NAME}.grpc.pb.h")
        
        list(APPEND PROTO_SOURCES ${GRPC_SRC})
        list(APPEND PROTO_HEADERS ${GRPC_HDR})
        
        add_custom_command(
            OUTPUT ${GRPC_SRC} ${GRPC_HDR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/generated/cpp
            COMMAND protoc --grpc_out=${CMAKE_SOURCE_DIR}/generated/cpp 
                           --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
                           --proto_path=${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
            DEPENDS ${CMAKE_SOURCE_DIR}/${PROTO_FILE}
            COMMENT "Generating gRPC files for ${PROTO_FILE}"
        )
    endforeach()
endif()

# 创建protobuf目标
add_custom_target(generate_proto ALL DEPENDS ${PROTO_SOURCES} ${PROTO_HEADERS})

# 添加子目录
add_subdirectory(third_party/litegrpc)

# 添加linxos_rpc库
if(EXISTS "${CMAKE_SOURCE_DIR}/src/linxos_rpc")
    add_subdirectory(src/linxos_rpc)
endif()

# 添加示例程序
# litegrpc-test目录不存在，暂时注释掉
# add_subdirectory(examples/litegrpc-test)

# 如果xiaozhi-integration存在，也添加它
if(EXISTS "${CMAKE_SOURCE_DIR}/examples/xiaozhi-integration")
    add_subdirectory(examples/xiaozhi-integration)
endif()

# 打印配置信息
message(STATUS "=== LinxOS RPC 构建配置 ===")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "Protobuf版本: ${PROTOBUF_PKG_VERSION}")
if(HAVE_GRPC)
    message(STATUS "gRPC版本: ${GRPC_PKG_VERSION}")
    message(STATUS "gRPC支持: 启用")
else()
    message(STATUS "gRPC支持: 禁用 (将使用LiteGRPC)")
endif()
message(STATUS "LiteGRPC: 启用")
message(STATUS "==========================")