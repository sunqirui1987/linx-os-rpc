# xiaozhi-esp32 LinxOS RPC 集成示例

这个示例展示了如何将 LinxOS RPC 客户端库集成到 xiaozhi-esp32 项目中，实现设备端作为 gRPC 客户端，允许远程服务器调用设备的各种功能。

## 功能特性

### 支持的设备功能
- **语音交互**: 文本转语音播放、音量控制
- **显示控制**: 表情显示、文本显示、亮度调节
- **灯光控制**: RGB 颜色控制、灯光模式设置
- **音频处理**: 音频文件播放、录音控制
- **系统管理**: 系统信息获取、设备重启、WiFi 重连

### 集成特性
- **自动连接管理**: 自动连接、断线重连、心跳保持
- **工具注册机制**: 简单的 AddTool 接口注册设备功能
- **状态监控**: 连接状态回调、错误处理
- **JSON 参数**: 使用 JSON 格式传递参数和返回结果

## 项目结构

```
examples/xiaozhi-integration/
├── xiaozhi_linxos_integration.cpp  # 主集成示例代码
├── CMakeLists.txt                  # 构建配置文件
└── README.md                       # 本文档
```

## 依赖要求

### 系统依赖
- CMake 3.16+
- C++17 编译器 (GCC 7+ 或 Clang 6+)
- pkg-config

### 第三方库
- **gRPC**: gRPC C++ 库
- **Protocol Buffers**: protobuf C++ 库
- **JsonCpp**: JSON 解析库

### 安装依赖 (Ubuntu/Debian)

```bash
# 安装基础工具
sudo apt update
sudo apt install -y cmake build-essential pkg-config

# 安装 gRPC 和 protobuf
sudo apt install -y libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc

# 安装 JsonCpp
sudo apt install -y libjsoncpp-dev
```

### 安装依赖 (macOS)

```bash
# 使用 Homebrew
brew install cmake grpc protobuf jsoncpp
```

## 编译和运行

### 1. 编译项目

```bash
# 进入集成示例目录
cd examples/xiaozhi-integration

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)
```

### 2. 启动测试服务器

在运行集成示例之前，需要先启动 Golang 测试服务器：

```bash
# 在另一个终端中，进入 Golang 服务器目录
cd examples/golang-test-server

# 安装依赖并运行
go mod tidy
go run cmd/server/main.go
```

服务器将在 `localhost:50051` 上监听 gRPC 连接。

### 3. 运行集成示例

```bash
# 在构建目录中运行
./xiaozhi_linxos_integration
```

## 集成到 xiaozhi-esp32

### 1. 添加 LinxOS RPC 库

将 LinxOS RPC 库添加到你的 xiaozhi-esp32 项目中：

```cpp
// 在 application.cc 中包含头文件
#include "linxos_rpc/client.h"
```

### 2. 初始化 RPC 客户端

```cpp
class Application {
private:
    std::unique_ptr<linxos::rpc::LinxOSRPCClient> rpc_client_;
    
public:
    bool Initialize() {
        // 创建设备信息
        auto device_info = linxos::rpc::CreateXiaozhiDeviceInfo("your_device_id", "1.0.0");
        
        // 创建连接配置
        auto config = linxos::rpc::CreateDefaultConfig("your_server:50051");
        
        // 创建 RPC 客户端
        rpc_client_ = std::make_unique<linxos::rpc::LinxOSRPCClient>(device_info, config);
        
        // 注册设备功能
        RegisterDeviceTools();
        
        // 连接并启动
        return rpc_client_->Connect() && rpc_client_->Start();
    }
};
```

### 3. 注册设备功能

```cpp
void RegisterDeviceTools() {
    // 注册语音功能
    rpc_client_->AddTool("voice_speak", [](const std::string& params) -> std::string {
        // 调用你的语音模块
        // 返回 JSON 格式的结果
    });
    
    // 注册显示功能
    rpc_client_->AddTool("display_expression", [](const std::string& params) -> std::string {
        // 调用你的显示模块
        // 返回 JSON 格式的结果
    });
    
    // 注册其他功能...
}
```

### 4. 处理远程调用

当远程服务器调用设备功能时，LinxOS RPC 客户端会：

1. 接收 gRPC 调用请求
2. 根据工具名称找到对应的处理函数
3. 将参数转换为 JSON 格式传递给处理函数
4. 将处理结果返回给远程服务器

## API 参考

### 工具函数签名

所有注册的工具函数都应该遵循以下签名：

```cpp
std::string tool_function(const std::string& json_params);
```

### 参数格式

#### 语音播放 (voice_speak)
```json
{
    "text": "要播放的文本",
    "speed": 1.0,
    "volume": 80
}
```

#### 表情显示 (display_expression)
```json
{
    "expression": "happy",
    "duration": 3000
}
```

#### 灯光控制 (light_control)
```json
{
    "red": 255,
    "green": 0,
    "blue": 0,
    "brightness": 100
}
```

### 返回格式

所有工具函数都应该返回包含以下字段的 JSON：

```json
{
    "success": true,
    "message": "操作成功",
    "data": {
        // 可选的额外数据
    }
}
```

## 测试和调试

### 1. 查看连接状态

集成示例会输出详细的连接状态信息：

```
=== xiaozhi-esp32 LinxOS RPC 集成示例 ===
设备ID: xiaozhi_1703123456
LinxOS RPC 客户端初始化完成
注册设备功能工具...
已注册 8 个工具: voice_speak voice_volume display_expression display_text light_control light_mode audio_play system_info system_restart 
正在连接到远程服务器...
[状态变化] CONNECTED: 连接成功
启动 RPC 服务...
xiaozhi-esp32 设备已就绪，等待远程调用...
```

### 2. 测试远程调用

可以通过 Golang 测试服务器的 Web 界面或 API 来测试设备功能调用。

### 3. 调试技巧

- 检查网络连接和防火墙设置
- 确认服务器地址和端口正确
- 查看 gRPC 日志输出
- 使用 JSON 验证工具检查参数格式

## 常见问题

### Q: 编译时找不到 gRPC 库
A: 确保已正确安装 gRPC 开发包，并且 CMake 能够找到它们。

### Q: 连接服务器失败
A: 检查服务器是否正在运行，网络连接是否正常，防火墙是否阻止了连接。

### Q: 工具调用没有响应
A: 确认工具已正确注册，参数格式正确，处理函数没有异常。

### Q: 如何添加自定义功能
A: 实现符合签名的工具函数，使用 `AddTool` 注册，确保参数和返回值都是有效的 JSON 格式。

## 更多信息

- [LinxOS RPC 产品需求文档](../../.trae/documents/LinxOS-RPC-协议库产品需求文档.md)
- [LinxOS RPC 技术架构文档](../../.trae/documents/LinxOS-RPC-协议库技术架构文档.md)
- [Protocol Buffers 定义](../../proto/linxos_device.proto)