cmake_minimum_required(VERSION 3.14)
project(litegrpc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)

# Build nanopb
set(nanopb_BUILD_RUNTIME ON CACHE BOOL "Build nanopb runtime")
set(nanopb_BUILD_GENERATOR OFF CACHE BOOL "Don't build nanopb generator")
add_subdirectory(../nanopb nanopb EXCLUDE_FROM_ALL)

# Build nghttp2
set(ENABLE_LIB_ONLY ON CACHE BOOL "Build only nghttp2 library")
set(ENABLE_STATIC_LIB ON CACHE BOOL "Build static library")
set(ENABLE_SHARED_LIB OFF CACHE BOOL "Don't build shared library")
add_subdirectory(../nghttp2 nghttp2 EXCLUDE_FROM_ALL)

# Collect source files
file(GLOB_RECURSE LITEGRPC_SOURCES
    "src/core/*.cpp"
    "src/http2/*.cpp"
    "src/protobuf/*.cpp"
    "src/client/*.cpp"
)

# Create the litegrpc library
add_library(litegrpc STATIC ${LITEGRPC_SOURCES})

# Include directories
target_include_directories(litegrpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(litegrpc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../nanopb
    ${CMAKE_CURRENT_SOURCE_DIR}/../nghttp2/lib/includes
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(litegrpc
    PRIVATE
    protobuf-nanopb-static
    nghttp2_static
    ${OPENSSL_LIBRARIES}
)

# Set target properties
set_target_properties(litegrpc PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Install rules
install(TARGETS litegrpc
    EXPORT litegrpcTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT litegrpcTargets
    FILE litegrpcTargets.cmake
    NAMESPACE litegrpc::
    DESTINATION lib/cmake/litegrpc
)