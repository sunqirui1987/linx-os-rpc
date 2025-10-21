#!/bin/bash

# LinxOS RPC 项目构建脚本
# 提供统一的构建入口和使用指南

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助信息
show_help() {
    echo "LinxOS RPC 项目构建脚本"
    echo ""
    echo "用法: $0 [选项] [目标]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示此帮助信息"
    echo "  -c, --clean         清理构建目录"
    echo "  -d, --debug         调试模式构建"
    echo "  -r, --release       发布模式构建 (默认)"
    echo "  -j, --jobs N        并行构建任务数 (默认: CPU核心数)"
    echo "  --with-grpc         强制使用标准gRPC (如果可用)"
    echo "  --litegrpc-only     仅使用LiteGRPC"
    echo ""
    echo "目标:"
    echo "  all                 构建所有目标 (默认)"
    echo "  litegrpc            仅构建LiteGRPC库"
    echo "  litegrpc-test       构建LiteGRPC测试程序"
    echo "  xiaozhi-integration 构建小智集成示例"
    echo "  proto               仅生成protobuf文件"
    echo ""
    echo "示例:"
    echo "  $0                  # 默认构建所有目标"
    echo "  $0 -c               # 清理构建目录"
    echo "  $0 -d litegrpc-test # 调试模式构建测试程序"
    echo "  $0 --litegrpc-only  # 仅使用LiteGRPC构建"
}

# 检查依赖
check_dependencies() {
    print_info "检查构建依赖..."
    
    # 检查cmake
    if ! command -v cmake &> /dev/null; then
        print_error "cmake 未找到，请安装 cmake"
        exit 1
    fi
    
    # 检查protoc
    if ! command -v protoc &> /dev/null; then
        print_error "protoc 未找到，请安装 protobuf"
        exit 1
    fi
    
    # 检查pkg-config
    if ! command -v pkg-config &> /dev/null; then
        print_error "pkg-config 未找到，请安装 pkg-config"
        exit 1
    fi
    
    # 检查protobuf
    if ! pkg-config --exists protobuf; then
        print_error "protobuf 开发包未找到，请安装 protobuf-devel"
        exit 1
    fi
    
    # 检查gRPC (可选)
    if pkg-config --exists grpc++; then
        print_info "找到 gRPC: $(pkg-config --modversion grpc++)"
        HAVE_GRPC=true
    else
        print_warning "未找到 gRPC，将使用 LiteGRPC 作为替代"
        HAVE_GRPC=false
    fi
    
    print_success "依赖检查完成"
}

# 清理构建目录
clean_build() {
    print_info "清理构建目录..."
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        print_success "构建目录已清理"
    else
        print_info "构建目录不存在，无需清理"
    fi
}

# 创建构建目录
create_build_dir() {
    if [ ! -d "${BUILD_DIR}" ]; then
        mkdir -p "${BUILD_DIR}"
        print_info "创建构建目录: ${BUILD_DIR}"
    fi
}

# 配置项目
configure_project() {
    print_info "配置项目..."
    
    cd "${BUILD_DIR}"
    
    local cmake_args=()
    cmake_args+=("-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
    
    if [ "${FORCE_GRPC}" = true ] && [ "${HAVE_GRPC}" = true ]; then
        cmake_args+=("-DFORCE_GRPC=ON")
        print_info "强制使用标准 gRPC"
    elif [ "${LITEGRPC_ONLY}" = true ]; then
        cmake_args+=("-DLITEGRPC_ONLY=ON")
        print_info "仅使用 LiteGRPC"
    fi
    
    cmake "${cmake_args[@]}" "${PROJECT_ROOT}"
    
    print_success "项目配置完成"
}

# 构建项目
build_project() {
    print_info "开始构建..."
    
    cd "${BUILD_DIR}"
    
    if [ -n "${TARGET}" ] && [ "${TARGET}" != "all" ]; then
        make -j"${JOBS}" "${TARGET}"
        print_success "目标 ${TARGET} 构建完成"
    else
        make -j"${JOBS}"
        print_success "所有目标构建完成"
    fi
}

# 运行测试
run_tests() {
    print_info "运行测试..."
    
    cd "${BUILD_DIR}"
    
    # 检查是否有测试程序
    if [ -f "examples/litegrpc-test/litegrpc-test" ]; then
        print_info "运行 LiteGRPC 测试..."
        ./examples/litegrpc-test/litegrpc-test localhost:50051 || true
    fi
    
    print_success "测试完成"
}

# 显示构建结果
show_results() {
    print_info "构建结果:"
    
    cd "${BUILD_DIR}"
    
    echo ""
    echo "可执行文件:"
    find . -name "*.a" -o -name "*-test" -o -name "*-integration" | while read -r file; do
        if [ -f "$file" ]; then
            echo "  $(realpath "$file")"
        fi
    done
    
    echo ""
    echo "库文件:"
    find . -name "*.a" -o -name "*.so" | while read -r file; do
        if [ -f "$file" ]; then
            echo "  $(realpath "$file")"
        fi
    done
    
    echo ""
    print_success "构建完成！"
    echo ""
    echo "使用方法:"
    echo "  # 运行 LiteGRPC 测试:"
    echo "  ${BUILD_DIR}/examples/litegrpc-test/litegrpc-test [server_address]"
    echo ""
    echo "  # 运行小智集成示例 (如果构建了):"
    echo "  ${BUILD_DIR}/examples/xiaozhi-integration/xiaozhi-integration"
}

# 默认参数
BUILD_TYPE="Release"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
TARGET=""
CLEAN=false
FORCE_GRPC=false
LITEGRPC_ONLY=false
RUN_TESTS=false

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --with-grpc)
            FORCE_GRPC=true
            shift
            ;;
        --litegrpc-only)
            LITEGRPC_ONLY=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        all|litegrpc|litegrpc-test|xiaozhi-integration|proto)
            TARGET="$1"
            shift
            ;;
        *)
            print_error "未知参数: $1"
            show_help
            exit 1
            ;;
    esac
done

# 主流程
main() {
    print_info "LinxOS RPC 项目构建开始"
    print_info "构建类型: ${BUILD_TYPE}"
    print_info "并行任务数: ${JOBS}"
    
    if [ "${CLEAN}" = true ]; then
        clean_build
        exit 0
    fi
    
    check_dependencies
    create_build_dir
    configure_project
    build_project
    
    if [ "${RUN_TESTS}" = true ]; then
        run_tests
    fi
    
    show_results
}

# 运行主流程
main "$@"