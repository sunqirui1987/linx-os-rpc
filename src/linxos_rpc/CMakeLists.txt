# LinxOS RPC 客户端库
cmake_minimum_required(VERSION 3.10)

project(linxos_rpc VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF_PKG REQUIRED protobuf)

# 检查是否有gRPC
pkg_check_modules(GRPC_PKG grpc++)
if(GRPC_PKG_FOUND)
    set(HAVE_GRPC TRUE)
    message(STATUS "Found gRPC for linxos_rpc: ${GRPC_PKG_VERSION}")
else()
    set(HAVE_GRPC FALSE)
    message(STATUS "gRPC not found for linxos_rpc, will use LiteGRPC")
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/generated/cpp)
include_directories(${CMAKE_SOURCE_DIR}/third_party/litegrpc)
include_directories(${PROTOBUF_PKG_INCLUDE_DIRS})

if(HAVE_GRPC)
    include_directories(${GRPC_PKG_INCLUDE_DIRS})
endif()

# 链接目录
link_directories(${PROTOBUF_PKG_LIBRARY_DIRS})
if(HAVE_GRPC)
    link_directories(${GRPC_PKG_LIBRARY_DIRS})
endif()

# 添加protobuf生成的源文件
set(PROTO_SOURCES
    ${CMAKE_SOURCE_DIR}/generated/cpp/device.pb.cc
)

# 如果有gRPC，添加gRPC生成的源文件
if(HAVE_GRPC)
    list(APPEND PROTO_SOURCES
        ${CMAKE_SOURCE_DIR}/generated/cpp/device.grpc.pb.cc
    )
endif()

# 创建linxos_rpc静态库
add_library(linxos_rpc STATIC
    client.cpp
    ${PROTO_SOURCES}
)

# 设置编译选项
target_compile_options(linxos_rpc PRIVATE ${PROTOBUF_PKG_CFLAGS_OTHER})
if(HAVE_GRPC)
    target_compile_options(linxos_rpc PRIVATE ${GRPC_PKG_CFLAGS_OTHER})
endif()

# 链接库
set(LINK_LIBRARIES ${PROTOBUF_PKG_LIBRARIES})

if(HAVE_GRPC)
    list(APPEND LINK_LIBRARIES ${GRPC_PKG_LIBRARIES})
else()
    # 使用LiteGRPC
    list(APPEND LINK_LIBRARIES litegrpc)
    add_dependencies(linxos_rpc litegrpc)
endif()

target_link_libraries(linxos_rpc ${LINK_LIBRARIES})

# 设置头文件安装
target_include_directories(linxos_rpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 安装规则
install(TARGETS linxos_rpc
    EXPORT linxos_rpcTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES client.h
    DESTINATION include/linxos_rpc
)

# 导出配置
install(EXPORT linxos_rpcTargets
    FILE linxos_rpcTargets.cmake
    NAMESPACE linxos_rpc::
    DESTINATION lib/cmake/linxos_rpc
)