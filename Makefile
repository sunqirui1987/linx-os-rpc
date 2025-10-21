# LinxOS RPC Library Makefile
# 提供便捷的构建和管理命令

.PHONY: all clean proto cpp go examples test install help

# 默认目标
all: proto cpp

# 项目目录
PROJECT_ROOT := $(shell pwd)
BUILD_DIR := $(PROJECT_ROOT)/build
GENERATED_DIR := $(PROJECT_ROOT)/generated
SCRIPTS_DIR := $(PROJECT_ROOT)/scripts

# 颜色输出
BLUE := \033[0;34m
GREEN := \033[0;32m
YELLOW := \033[1;33m
RED := \033[0;31m
NC := \033[0m

# 日志函数
define log_info
	@echo -e "$(BLUE)[INFO]$(NC) $(1)"
endef

define log_success
	@echo -e "$(GREEN)[SUCCESS]$(NC) $(1)"
endef

define log_warning
	@echo -e "$(YELLOW)[WARNING]$(NC) $(1)"
endef

define log_error
	@echo -e "$(RED)[ERROR]$(NC) $(1)"
endef

# 帮助信息
help:
	@echo "LinxOS RPC Library Build System"
	@echo ""
	@echo "可用目标:"
	@echo "  all        - 生成 proto 文件并构建 C++ 库 (默认)"
	@echo "  proto      - 生成所有语言的 proto 代码"
	@echo "  proto-cpp  - 只生成 C++ proto 代码"
	@echo "  proto-go   - 只生成 Go proto 代码"
	@echo "  cpp        - 构建 C++ 库"
	@echo "  go         - 构建 Go 示例"
	@echo "  examples   - 构建所有示例"
	@echo "  test       - 运行测试"
	@echo "  clean      - 清理构建文件"
	@echo "  clean-all  - 清理所有生成的文件"
	@echo "  install    - 安装库文件"
	@echo "  help       - 显示此帮助信息"
	@echo ""
	@echo "示例:"
	@echo "  make proto     # 生成所有 proto 代码"
	@echo "  make cpp       # 构建 C++ 库"
	@echo "  make examples  # 构建示例"

# 生成 proto 代码
proto:
	$(call log_info,"生成 proto 代码...")
	@$(SCRIPTS_DIR)/generate_proto.sh --all
	$(call log_success,"Proto 代码生成完成")

proto-cpp:
	$(call log_info,"生成 C++ proto 代码...")
	@$(SCRIPTS_DIR)/generate_proto.sh --cpp
	$(call log_success,"C++ Proto 代码生成完成")

proto-go:
	$(call log_info,"生成 Go proto 代码...")
	@$(SCRIPTS_DIR)/generate_proto.sh --go
	$(call log_success,"Go Proto 代码生成完成")

# 构建 C++ 库
cpp: proto-cpp
	$(call log_info,"构建 C++ 库...")
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Release
	@cd $(BUILD_DIR) && make -j$(shell nproc 2>/dev/null || echo 4)
	$(call log_success,"C++ 库构建完成")

# 构建 Go 示例
go: proto-go
	$(call log_info,"构建 Go 示例...")
	@cd examples/golang-test-server && go build -o server ./cmd/server
	@cd examples/golang-test-server && go build -o client ./cmd/client
	$(call log_success,"Go 示例构建完成")

# 构建所有示例
examples: cpp go
	$(call log_info,"构建所有示例...")
	@if [ -d examples/xiaozhi-integration ]; then \
		cd examples/xiaozhi-integration && mkdir -p build && cd build && cmake .. && make; \
	fi
	$(call log_success,"所有示例构建完成")

# 运行测试
test: examples
	$(call log_info,"运行测试...")
	@echo "启动 Go 服务器进行测试..."
	@cd examples/golang-test-server && timeout 10s ./server &
	@sleep 2
	@cd examples/golang-test-server && ./client
	@pkill -f "./server" || true
	$(call log_success,"测试完成")

# 清理构建文件
clean:
	$(call log_info,"清理构建文件...")
	@rm -rf $(BUILD_DIR)
	@rm -f examples/golang-test-server/server
	@rm -f examples/golang-test-server/client
	@if [ -d examples/xiaozhi-integration/build ]; then \
		rm -rf examples/xiaozhi-integration/build; \
	fi
	$(call log_success,"构建文件清理完成")

# 清理所有生成的文件
clean-all: clean
	$(call log_info,"清理所有生成的文件...")
	@$(SCRIPTS_DIR)/generate_proto.sh --clean
	$(call log_success,"所有文件清理完成")

# 安装库文件
install: cpp
	$(call log_info,"安装库文件...")
	@cd $(BUILD_DIR) && make install
	$(call log_success,"库文件安装完成")

# 开发模式 - 监听文件变化并自动重新构建
dev:
	$(call log_info,"启动开发模式...")
	@echo "监听文件变化，按 Ctrl+C 退出"
	@while true; do \
		inotifywait -r -e modify,create,delete src/ proto/ 2>/dev/null || \
		(sleep 2); \
		make cpp; \
		sleep 1; \
	done

# 格式化代码
format:
	$(call log_info,"格式化代码...")
	@find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@find examples/ -name "*.go" | xargs gofmt -w
	$(call log_success,"代码格式化完成")

# 检查代码质量
lint:
	$(call log_info,"检查代码质量...")
	@find src/ -name "*.cpp" -o -name "*.h" | xargs clang-tidy
	@cd examples/golang-test-server && go vet ./...
	$(call log_success,"代码质量检查完成")

# 生成文档
docs:
	$(call log_info,"生成文档...")
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen 未安装，跳过 C++ 文档生成"
	@cd examples/golang-test-server && go doc ./... > ../../docs/go-api.md
	$(call log_success,"文档生成完成")

# 检查依赖
check-deps:
	$(call log_info,"检查依赖...")
	@echo "检查 protoc..."
	@which protoc > /dev/null || ($(call log_error,"protoc 未安装") && exit 1)
	@echo "检查 cmake..."
	@which cmake > /dev/null || ($(call log_error,"cmake 未安装") && exit 1)
	@echo "检查 make..."
	@which make > /dev/null || ($(call log_error,"make 未安装") && exit 1)
	@echo "检查 go..."
	@which go > /dev/null || ($(call log_warning,"go 未安装，将跳过 Go 相关构建"))
	$(call log_success,"依赖检查完成")

# 显示项目信息
info:
	@echo "LinxOS RPC Library"
	@echo "=================="
	@echo "项目根目录: $(PROJECT_ROOT)"
	@echo "构建目录:   $(BUILD_DIR)"
	@echo "生成目录:   $(GENERATED_DIR)"
	@echo "脚本目录:   $(SCRIPTS_DIR)"
	@echo ""
	@echo "目录结构:"
	@tree -L 2 . 2>/dev/null || ls -la

# 快速开始
quickstart: check-deps all
	$(call log_success,"快速开始完成！")
	@echo ""
	@echo "下一步:"
	@echo "1. 运行测试: make test"
	@echo "2. 查看示例: ls examples/"
	@echo "3. 查看文档: ls docs/"