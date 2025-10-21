#!/bin/bash

# LinxOS RPC Proto Generation Script
# 支持生成 C++, Go, Python, Java 等多种语言的代码

set -e

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 目录定义
PROTO_DIR="$PROJECT_ROOT/proto"
GENERATED_DIR="$PROJECT_ROOT/generated"

# 输出目录
CPP_OUT_DIR="$GENERATED_DIR/cpp"
GO_OUT_DIR="$GENERATED_DIR/go"
PYTHON_OUT_DIR="$GENERATED_DIR/python"
JAVA_OUT_DIR="$GENERATED_DIR/java"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    log_info "检查依赖..."
    
    # 检查 protoc
    if ! command -v protoc &> /dev/null; then
        log_error "protoc 未安装，请先安装 Protocol Buffers"
        exit 1
    fi
    
    # 检查 grpc_cpp_plugin
    if ! command -v grpc_cpp_plugin &> /dev/null; then
        log_warning "grpc_cpp_plugin 未安装，将跳过 C++ gRPC 代码生成"
        SKIP_CPP_GRPC=true
    fi
    
    # 检查 protoc-gen-go
    if ! command -v protoc-gen-go &> /dev/null; then
        log_warning "protoc-gen-go 未安装，将跳过 Go 代码生成"
        SKIP_GO=true
    fi
    
    # 检查 protoc-gen-go-grpc
    if ! command -v protoc-gen-go-grpc &> /dev/null; then
        log_warning "protoc-gen-go-grpc 未安装，将跳过 Go gRPC 代码生成"
        SKIP_GO_GRPC=true
    fi
    
    log_success "依赖检查完成"
}

# 创建输出目录
create_output_dirs() {
    log_info "创建输出目录..."
    
    mkdir -p "$CPP_OUT_DIR"
    mkdir -p "$GO_OUT_DIR"
    mkdir -p "$PYTHON_OUT_DIR"
    mkdir -p "$JAVA_OUT_DIR"
    
    log_success "输出目录创建完成"
}

# 生成 C++ 代码
generate_cpp() {
    log_info "生成 C++ 代码..."
    
    # 先生成基本的 protobuf C++ 代码
    protoc \
        --proto_path="$PROTO_DIR" \
        --cpp_out="$CPP_OUT_DIR" \
        "$PROTO_DIR/device.proto"
    
    if [ $? -ne 0 ]; then
        log_error "C++ protobuf 代码生成失败"
        return 1
    fi
    
    # 如果有 gRPC 插件，则生成 gRPC 代码
    if [ "$SKIP_CPP_GRPC" != true ]; then
        protoc \
            --proto_path="$PROTO_DIR" \
            --grpc_out="$CPP_OUT_DIR" \
            --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
            "$PROTO_DIR/device.proto"
        
        if [ $? -eq 0 ]; then
            log_success "C++ 代码（包含 gRPC）生成完成: $CPP_OUT_DIR"
        else
            log_error "C++ gRPC 代码生成失败"
            return 1
        fi
    else
        log_success "C++ protobuf 代码生成完成: $CPP_OUT_DIR"
        log_warning "跳过 gRPC 代码生成（缺少 grpc_cpp_plugin）"
    fi
}

# 生成 Go 代码
generate_go() {
    if [ "$SKIP_GO" = true ] || [ "$SKIP_GO_GRPC" = true ]; then
        log_warning "跳过 Go 代码生成"
        return 0
    fi
    
    log_info "生成 Go 代码..."
    
    protoc \
        --proto_path="$PROTO_DIR" \
        --go_out="$GO_OUT_DIR" \
        --go_opt=paths=source_relative \
        --go-grpc_out="$GO_OUT_DIR" \
        --go-grpc_opt=paths=source_relative \
        "$PROTO_DIR/device.proto"
    
    if [ $? -eq 0 ]; then
        log_success "Go 代码生成完成: $GO_OUT_DIR"
    else
        log_error "Go 代码生成失败"
        return 1
    fi
}

# 生成 Python 代码
generate_python() {
    log_info "生成 Python 代码..."
    
    python -m grpc_tools.protoc \
        --proto_path="$PROTO_DIR" \
        --python_out="$PYTHON_OUT_DIR" \
        --grpc_python_out="$PYTHON_OUT_DIR" \
        "$PROTO_DIR/device.proto" 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Python 代码生成完成: $PYTHON_OUT_DIR"
    else
        log_warning "Python 代码生成失败，可能需要安装 grpcio-tools: pip install grpcio-tools"
    fi
}

# 生成 Java 代码
generate_java() {
    log_info "生成 Java 代码..."
    
    protoc \
        --proto_path="$PROTO_DIR" \
        --java_out="$JAVA_OUT_DIR" \
        --grpc_java_out="$JAVA_OUT_DIR" \
        --plugin=protoc-gen-grpc-java=$(which protoc-gen-grpc-java) \
        "$PROTO_DIR/device.proto" 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Java 代码生成完成: $JAVA_OUT_DIR"
    else
        log_warning "Java 代码生成失败，可能需要安装 protoc-gen-grpc-java"
    fi
}

# 清理旧文件
clean_generated() {
    log_info "清理旧的生成文件..."
    
    if [ -d "$GENERATED_DIR" ]; then
        rm -rf "$GENERATED_DIR"
        log_success "清理完成"
    fi
}

# 显示帮助信息
show_help() {
    echo "LinxOS RPC Proto Generation Script"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help     显示帮助信息"
    echo "  -c, --clean    清理生成的文件"
    echo "  --cpp          只生成 C++ 代码"
    echo "  --go           只生成 Go 代码"
    echo "  --python       只生成 Python 代码"
    echo "  --java         只生成 Java 代码"
    echo "  --all          生成所有语言代码 (默认)"
    echo ""
    echo "示例:"
    echo "  $0                # 生成所有语言代码"
    echo "  $0 --cpp         # 只生成 C++ 代码"
    echo "  $0 --clean       # 清理生成的文件"
}

# 主函数
main() {
    local generate_cpp_flag=false
    local generate_go_flag=false
    local generate_python_flag=false
    local generate_java_flag=false
    local generate_all=true
    local clean_flag=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_flag=true
                shift
                ;;
            --cpp)
                generate_cpp_flag=true
                generate_all=false
                shift
                ;;
            --go)
                generate_go_flag=true
                generate_all=false
                shift
                ;;
            --python)
                generate_python_flag=true
                generate_all=false
                shift
                ;;
            --java)
                generate_java_flag=true
                generate_all=false
                shift
                ;;
            --all)
                generate_all=true
                shift
                ;;
            *)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 执行清理
    if [ "$clean_flag" = true ]; then
        clean_generated
        exit 0
    fi
    
    # 检查 proto 文件是否存在
    if [ ! -f "$PROTO_DIR/device.proto" ]; then
        log_error "Proto 文件不存在: $PROTO_DIR/device.proto"
        exit 1
    fi
    
    log_info "开始生成 Proto 代码..."
    log_info "Proto 文件: $PROTO_DIR/device.proto"
    log_info "输出目录: $GENERATED_DIR"
    
    # 检查依赖
    check_dependencies
    
    # 创建输出目录
    create_output_dirs
    
    # 生成代码
    local success_count=0
    local total_count=0
    
    if [ "$generate_all" = true ] || [ "$generate_cpp_flag" = true ]; then
        total_count=$((total_count + 1))
        if generate_cpp; then
            success_count=$((success_count + 1))
        fi
    fi
    
    if [ "$generate_all" = true ] || [ "$generate_go_flag" = true ]; then
        total_count=$((total_count + 1))
        if generate_go; then
            success_count=$((success_count + 1))
        fi
    fi
    
    if [ "$generate_all" = true ] || [ "$generate_python_flag" = true ]; then
        total_count=$((total_count + 1))
        if generate_python; then
            success_count=$((success_count + 1))
        fi
    fi
    
    if [ "$generate_all" = true ] || [ "$generate_java_flag" = true ]; then
        total_count=$((total_count + 1))
        if generate_java; then
            success_count=$((success_count + 1))
        fi
    fi
    
    # 显示结果
    echo ""
    log_info "代码生成完成: $success_count/$total_count 成功"
    
    if [ $success_count -eq $total_count ]; then
        log_success "所有代码生成成功！"
        exit 0
    else
        log_warning "部分代码生成失败，请检查依赖和错误信息"
        exit 1
    fi
}

# 运行主函数
main "$@"