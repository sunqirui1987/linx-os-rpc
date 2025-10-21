# LinxOS RPC Library

LinxOS RPC Library 是一个跨平台的 gRPC 通信库，专为 LinxOS 设备管理和控制而设计。

## 🚀 特性

- **多语言支持**: 支持 C++、Go、Python、Java 等多种编程语言
- **完整的设备管理**: 设备注册、心跳、状态监控
- **丰富的设备控制**: 语音、显示、灯光、音频、系统管理
- **易于集成**: 提供简洁的 API 和完整的示例
- **自动化构建**: 一键生成 proto 代码和构建库
- **LiteGRPC 支持**: 轻量级 gRPC 替代方案，适用于资源受限的嵌入式设备
- **API 兼容性**: LiteGRPC 提供与标准 gRPC 完全兼容的接口

## 📁 项目结构

```
linx-os-rpc/
├── proto/                  # Protocol Buffers 定义文件
│   └── device.proto        # 设备服务协议定义
├── src/                    # C++ 源代码
│   ├── linxos_rpc/         # 核心库
│   │   ├── client.h        # 客户端头文件
│   │   └── client.cpp      # 客户端实现
│   └── CMakeLists.txt      # CMake 构建配置
├── third_party/            # 第三方库
│   └── litegrpc/           # LiteGRPC 轻量级实现
│       ├── litegrpc.h      # LiteGRPC 头文件
│       ├── litegrpc.cpp    # LiteGRPC 实现
│       └── CMakeLists.txt  # LiteGRPC 构建配置
├── scripts/                # 构建脚本
│   └── generate_proto.sh   # Proto 代码生成脚本
├── generated/              # 生成的代码（自动创建）
│   ├── cpp/                # C++ 生成代码
│   ├── go/                 # Go 生成代码
│   ├── python/             # Python 生成代码
│   └── java/               # Java 生成代码
├── examples/               # 示例代码
│   ├── golang-test-server/ # Go 测试服务器
│   ├── xiaozhi-integration/# 小智集成示例
│   └── litegrpc-test/      # LiteGRPC 测试程序
├── docs/                   # 文档
├── cmake/                  # CMake 模块
├── build.sh               # 统一构建脚本
├── CMakeLists.txt         # 主项目构建配置
└── Makefile               # 便捷构建脚本
```

## 🛠️ 快速开始

### 1. 环境要求

#### 基础依赖
- **Protocol Buffers**: `protoc` 编译器
- **CMake**: 3.10 或更高版本
- **Make**: 构建工具

#### 语言特定依赖

**C++:**
- C++17 兼容编译器
- gRPC C++ 库
- JsonCpp 库

**Go:**
- Go 1.16 或更高版本
- protoc-gen-go: `go install google.golang.org/protobuf/cmd/protoc-gen-go@latest`
- protoc-gen-go-grpc: `go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest`

**Python:**
- Python 3.6 或更高版本
- grpcio-tools: `pip install grpcio-tools`

### 2. 安装依赖

#### macOS (使用 Homebrew)
```bash
# 安装基础依赖
brew install protobuf cmake

# 安装 gRPC (可选，用于 C++ gRPC 支持)
brew install grpc

# 安装 Go 依赖
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```

#### Ubuntu/Debian
```bash
# 安装基础依赖
sudo apt-get update
sudo apt-get install -y protobuf-compiler cmake build-essential

# 安装 gRPC
sudo apt-get install -y libgrpc++-dev protobuf-compiler-grpc

# 安装 Go 依赖
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```

### 3. 构建项目

#### 使用 Makefile (推荐)
```bash
# 检查依赖
make check-deps

# 快速开始 - 生成 proto 代码并构建 C++ 库
make quickstart

# 生成所有语言的 proto 代码
make proto

# 构建 C++ 库
make cpp

# 构建 Go 示例
make go

# 构建所有示例
make examples

# 运行测试
make test
```

#### 手动构建
```bash
# 1. 生成 proto 代码
./scripts/generate_proto.sh --all

# 2. 构建 C++ 库
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# 3. 构建 Go 示例
cd examples/golang-test-server
go build -o server ./cmd/server
go build -o client ./cmd/client
```

## 📖 使用指南

### Proto 代码生成

使用 `generate_proto.sh` 脚本生成不同语言的代码：

```bash
# 生成所有语言代码
./scripts/generate_proto.sh

# 只生成 C++ 代码
./scripts/generate_proto.sh --cpp

# 只生成 Go 代码
./scripts/generate_proto.sh --go

# 清理生成的文件
./scripts/generate_proto.sh --clean

# 查看帮助
./scripts/generate_proto.sh --help
```

### C++ 客户端使用

```cpp
#include "linxos_rpc/client.h"

int main() {
    // 创建设备信息
    auto device_info = CreateDeviceInfo(
        "device_001", 
        "TestDevice", 
        "smart_speaker",
        "1.0.0",
        "192.168.1.100"
    );
    
    // 创建连接配置
    auto config = CreateConnectionConfig("localhost:50051");
    
    // 创建客户端
    LinxOSRPCClient client;
    
    // 连接到服务器
    if (client.Connect(config) == ConnectionStatus::CONNECTED) {
        // 注册设备
        client.RegisterDevice(device_info, {"voice_speak", "display_text"});
        
        // 发送语音播放请求
        client.VoiceSpeak("Hello, World!", 1.0, 80, "default");
        
        // 断开连接
        client.Disconnect();
    }
    
    return 0;
}
```

### Go 服务器使用

```go
package main

import (
    "log"
    "net"
    "google.golang.org/grpc"
    pb "path/to/generated/go"
)

func main() {
    lis, err := net.Listen("tcp", ":50051")
    if err != nil {
        log.Fatalf("failed to listen: %v", err)
    }

    s := grpc.NewServer()
    pb.RegisterLinxOSDeviceServiceServer(s, &server{})

    log.Println("Server starting on :50051")
    if err := s.Serve(lis); err != nil {
        log.Fatalf("failed to serve: %v", err)
    }
}
```

## 🧪 运行示例

### Go 测试服务器

```bash
# 启动服务器
cd examples/golang-test-server
./server

# 在另一个终端运行客户端测试
./client
```

### 使用 Makefile 运行测试

```bash
# 自动启动服务器并运行客户端测试
make test
```

## 📚 API 文档

### 设备管理
- `RegisterDevice`: 注册设备到服务器
- `Heartbeat`: 发送心跳保持连接

### 语音交互
- `VoiceSpeak`: 语音播放
- `VoiceVolume`: 音量控制

### 显示控制
- `DisplayExpression`: 表情显示
- `DisplayText`: 文本显示
- `DisplayBrightness`: 亮度控制

### 灯光控制
- `LightControl`: RGB 灯光控制
- `LightMode`: 灯光模式设置

### 音频处理
- `AudioPlay`: 音频播放
- `AudioRecord`: 音频录制
- `AudioStop`: 音频停止

### 系统管理
- `SystemInfo`: 获取系统信息
- `SystemRestart`: 系统重启
- `SystemWifiReconnect`: WiFi 重连

### 通用工具
- `CallTool`: 通用工具调用接口

## 🔧 开发指南

### 添加新的 RPC 方法

1. 在 `proto/device.proto` 中添加新的消息定义和 RPC 方法
2. 重新生成代码: `./scripts/generate_proto.sh`
3. 在 C++ 客户端 (`src/linxos_rpc/client.cpp`) 中实现新方法
4. 在 Go 服务器 (`examples/golang-test-server/internal/device_service.go`) 中实现新方法
5. 更新测试和文档

### 代码格式化

```bash
# 格式化所有代码
make format

# 代码质量检查
make lint
```

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 🆘 故障排除

### 常见问题

**Q: protoc 命令未找到**
A: 请安装 Protocol Buffers: `brew install protobuf` (macOS) 或 `sudo apt-get install protobuf-compiler` (Ubuntu)

**Q: grpc_cpp_plugin 未找到**
A: 请安装 gRPC: `brew install grpc` (macOS) 或 `sudo apt-get install libgrpc++-dev` (Ubuntu)

**Q: Go 代码生成失败**
A: 请安装 Go protobuf 插件:
```bash
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```

**Q: C++ 编译失败**
A: 请确保安装了所需的依赖库 (gRPC, JsonCpp) 和 C++17 兼容编译器

### 获取帮助

- 查看 [文档](docs/)
- 提交 [Issue](https://github.com/your-repo/linx-os-rpc/issues)
- 查看 [示例代码](examples/)

---

**LinxOS RPC Library** - 让设备通信更简单 🚀