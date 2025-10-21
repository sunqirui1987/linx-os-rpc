cmake_minimum_required(VERSION 3.16)
project(hello_rpc_client)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖包
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

# 查找 jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 设置包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../nanopb)
include_directories(${JSONCPP_INCLUDE_DIRS})

# 添加 litegrpc 库路径
set(LITEGRPC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${LITEGRPC_ROOT}/include)

# 查找 litegrpc 源文件
file(GLOB_RECURSE LITEGRPC_SOURCES 
    ${LITEGRPC_ROOT}/src/*.cpp
    ${LITEGRPC_ROOT}/src/*.c
)

# 查找 nanopb 源文件
set(NANOPB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../nanopb)
set(NANOPB_SOURCES
    ${NANOPB_ROOT}/pb_common.c
    ${NANOPB_ROOT}/pb_decode.c
    ${NANOPB_ROOT}/pb_encode.c
)

# 查找 nghttp2 库 (用于 HTTP/2 支持)
pkg_check_modules(NGHTTP2 libnghttp2)

if(NGHTTP2_FOUND)
    message(STATUS "Found nghttp2: ${NGHTTP2_LIBRARIES}")
    include_directories(${NGHTTP2_INCLUDE_DIRS})
    link_directories(${NGHTTP2_LIBRARY_DIRS})
else()
    message(WARNING "nghttp2 not found, using system HTTP/2 implementation")
endif()

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# 定义可执行文件：Hello 客户端（简化版本）
add_executable(hello_client
    hello_client.cpp
)

# 链接库（简化版本）
target_link_libraries(hello_client
    pthread
)

# 如果找到了 nghttp2，链接它
if(NGHTTP2_FOUND)
    target_link_libraries(hello_client ${NGHTTP2_LIBRARIES})
    target_compile_definitions(hello_client PRIVATE HAVE_NGHTTP2)
endif()

# 安装目标
install(TARGETS hello_client
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "=== Hello RPC Client Build Configuration ===")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "Protobuf_VERSION: ${Protobuf_VERSION}")
message(STATUS "Protobuf_LIBRARIES: ${Protobuf_LIBRARIES}")
message(STATUS "LITEGRPC_ROOT: ${LITEGRPC_ROOT}")
message(STATUS "NANOPB_ROOT: ${NANOPB_ROOT}")
message(STATUS "NGHTTP2_FOUND: ${NGHTTP2_FOUND}")
if(NGHTTP2_FOUND)
    message(STATUS "NGHTTP2_INCLUDE_DIR: ${NGHTTP2_INCLUDE_DIR}")
    message(STATUS "NGHTTP2_LIBRARY: ${NGHTTP2_LIBRARY}")
endif()
message(STATUS "===========================================")