#!/bin/bash

# xiaozhi-esp32 LinxOS RPC 集成示例构建脚本
# 这个脚本用于构建 xiaozhi 集成示例

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EXAMPLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${EXAMPLE_DIR}/build"

echo -e "${BLUE}LinxOS RPC xiaozhi 集成示例构建脚本${NC}"
echo "项目根目录: ${PROJECT_ROOT}"
echo "示例目录: ${EXAMPLE_DIR}"
echo "构建目录: ${BUILD_DIR}"

# 函数：打印带颜色的消息
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_status "检查构建依赖..."
    
    # 检查 cmake
    if ! command -v cmake &> /dev/null; then
        print_error "cmake 未安装，请先安装 cmake"
        exit 1
    fi
    
    # 检查 protoc
    if ! command -v protoc &> /dev/null; then
        print_error "protoc 未安装，请先安装 Protocol Buffers"
        exit 1
    fi
    
    # 检查 pkg-config
    if ! command -v pkg-config &> /dev/null; then
        print_error "pkg-config 未安装，请先安装 pkg-config"
        exit 1
    fi
    
    # 检查 jsoncpp
    if ! pkg-config --exists jsoncpp; then
        print_warning "jsoncpp 未找到，尝试使用系统安装的版本"
    fi
    
    print_status "依赖检查完成"
}

# 生成 proto 代码
generate_proto() {
    print_status "生成 protobuf 代码..."
    
    cd "${PROJECT_ROOT}"
    
    # 使用项目的 proto 生成脚本
    if [ -f "scripts/generate_proto.sh" ]; then
        bash scripts/generate_proto.sh --cpp
    else
        print_error "找不到 proto 生成脚本"
        exit 1
    fi
    
    print_status "protobuf 代码生成完成"
}

# 构建项目
build_project() {
    print_status "开始构建项目..."
    
    # 创建构建目录
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    # 配置 CMake
    print_status "配置 CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=17
    
    # 编译
    print_status "编译项目..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    print_status "构建完成"
}

# 运行测试
run_test() {
    print_status "运行集成测试..."
    
    cd "${BUILD_DIR}"
    
    if [ -f "./xiaozhi_linxos_integration" ]; then
        print_status "启动 xiaozhi 集成示例..."
        ./xiaozhi_linxos_integration
    else
        print_error "找不到可执行文件"
        exit 1
    fi
}

# 清理构建文件
clean() {
    print_status "清理构建文件..."
    
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        print_status "构建文件已清理"
    else
        print_status "没有需要清理的文件"
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  build     构建项目 (默认)"
    echo "  test      运行测试"
    echo "  clean     清理构建文件"
    echo "  help      显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 build    # 构建项目"
    echo "  $0 test     # 运行测试"
    echo "  $0 clean    # 清理构建文件"
}

# 主函数
main() {
    case "${1:-build}" in
        "build")
            check_dependencies
            generate_proto
            build_project
            ;;
        "test")
            run_test
            ;;
        "clean")
            clean
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"