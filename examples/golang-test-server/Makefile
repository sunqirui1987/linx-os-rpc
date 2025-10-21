# Makefile for LinxOS RPC Golang Test Server

.PHONY: all build clean proto run deps

# 默认目标
all: deps proto build

# 安装依赖
deps:
	go mod tidy
	go mod download

# 生成 protobuf 代码
proto:
	@echo "Generating protobuf code..."
	protoc --go_out=. --go_opt=paths=source_relative \
		--go-grpc_out=. --go-grpc_opt=paths=source_relative \
		proto/device.proto

# 编译
build: proto
	@echo "Building server..."
	go build -o bin/server cmd/server/main.go

# 运行服务器
run: build
	@echo "Starting server..."
	./bin/server

# 清理
clean:
	rm -rf bin/
	rm -f proto/*.pb.go

# 安装 protoc 工具 (macOS)
install-protoc:
	@echo "Installing protoc tools..."
	go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
	go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

# 检查 protoc 是否安装
check-protoc:
	@which protoc > /dev/null || (echo "protoc not found. Please install Protocol Buffers compiler." && exit 1)
	@which protoc-gen-go > /dev/null || (echo "protoc-gen-go not found. Run 'make install-protoc' first." && exit 1)
	@which protoc-gen-go-grpc > /dev/null || (echo "protoc-gen-go-grpc not found. Run 'make install-protoc' first." && exit 1)

# 帮助
help:
	@echo "Available targets:"
	@echo "  all           - Install deps, generate proto, and build"
	@echo "  deps          - Install Go dependencies"
	@echo "  proto         - Generate protobuf code"
	@echo "  build         - Build the server"
	@echo "  run           - Build and run the server"
	@echo "  clean         - Clean build artifacts"
	@echo "  install-protoc - Install protoc tools"
	@echo "  check-protoc  - Check if protoc tools are installed"
	@echo "  help          - Show this help"