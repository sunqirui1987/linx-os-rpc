# LinxOS RPC Golang 测试服务器

这是一个简单的 Golang gRPC 服务器 demo，用于测试调用 xiaozhi-esp32 设备的各种能力。

## 功能特性

- 🎵 **语音播放**: 控制设备播放语音内容
- 📺 **显示控制**: 控制设备显示内容和表情
- 💡 **灯光控制**: 控制设备灯光颜色、亮度和模式
- 📊 **设备管理**: 查询设备状态和设备列表
- 🔄 **实时通信**: 基于 gRPC 的高效通信

## 项目结构

```
golang-test-server/
├── cmd/
│   ├── server/          # 服务器主程序
│   └── client/          # 测试客户端
├── internal/            # 内部实现
│   ├── device_manager.go    # 设备管理器
│   └── device_service.go    # gRPC 服务实现
├── proto/               # Protocol Buffers 定义
│   └── device.proto
├── go.mod              # Go 模块定义
├── Makefile           # 构建脚本
└── README.md          # 说明文档
```

## 快速开始

### 一键演示

如果你想快速体验这个 demo，可以直接运行：

```bash
./demo.sh
```

这个脚本会自动编译项目、启动服务器、运行客户端测试，并展示所有功能。

### 1. 环境要求

- Go 1.21+
- Protocol Buffers 编译器 (protoc)

### 2. 安装依赖

```bash
# 安装 protoc 工具
make install-protoc

# 安装 Go 依赖
make deps
```

### 3. 编译和运行

```bash
# 生成 protobuf 代码并编译
make build

# 运行服务器
make run
```

或者分步执行：

```bash
# 生成 protobuf 代码
make proto

# 编译服务器
go build -o bin/server cmd/server/main.go

# 运行服务器
./bin/server
```

### 4. 测试服务器

在另一个终端运行测试客户端：

```bash
# 编译客户端
go build -o bin/client cmd/client/main.go

# 运行测试
./bin/client

# 或者指定服务器地址和设备ID
./bin/client -addr localhost:50051 -device xiaozhi-001
```

## API 接口

### 设备控制服务 (DeviceControlService)

#### 1. 语音播放 (PlayVoice)
```protobuf
rpc PlayVoice(VoicePlayRequest) returns (VoicePlayResponse);
```

**请求参数:**
- `device_id`: 设备ID
- `text`: 要播放的文本
- `voice_type`: 语音类型 ("female", "male")
- `volume`: 音量 (0.0-1.0)

#### 2. 显示控制 (ControlDisplay)
```protobuf
rpc ControlDisplay(DisplayRequest) returns (DisplayResponse);
```

**请求参数:**
- `device_id`: 设备ID
- `content`: 显示内容
- `expression`: 表情类型 ("happy", "sad", "normal")
- `duration`: 显示时长（秒）

#### 3. 灯光控制 (ControlLight)
```protobuf
rpc ControlLight(LightControlRequest) returns (LightControlResponse);
```

**请求参数:**
- `device_id`: 设备ID
- `color`: 颜色值 (如 "#FF0000")
- `brightness`: 亮度 (0-100)
- `mode`: 模式 ("solid", "blink", "fade")

#### 4. 设备状态查询 (GetDeviceStatus)
```protobuf
rpc GetDeviceStatus(DeviceStatusRequest) returns (DeviceStatusResponse);
```

#### 5. 设备列表 (ListDevices)
```protobuf
rpc ListDevices(ListDevicesRequest) returns (ListDevicesResponse);
```

## 使用示例

### Go 客户端示例

```go
package main

import (
    "context"
    "log"
    "google.golang.org/grpc"
    pb "github.com/linx-os/rpc/examples/golang-test-server/proto"
)

func main() {
    // 连接服务器
    conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure())
    if err != nil {
        log.Fatal(err)
    }
    defer conn.Close()

    client := pb.NewDeviceControlServiceClient(conn)

    // 语音播放
    resp, err := client.PlayVoice(context.Background(), &pb.VoicePlayRequest{
        DeviceId:  "xiaozhi-001",
        Text:      "你好世界",
        VoiceType: "female",
        Volume:    0.8,
    })
    if err != nil {
        log.Fatal(err)
    }
    log.Printf("结果: %t, 消息: %s", resp.Success, resp.Message)
}
```

### 使用 grpcurl 测试

```bash
# 安装 grpcurl
go install github.com/fullstorydev/grpcurl/cmd/grpcurl@latest

# 查看服务列表
grpcurl -plaintext localhost:50051 list

# 语音播放测试
grpcurl -plaintext -d '{
  "device_id": "xiaozhi-001",
  "text": "你好，小智！",
  "voice_type": "female",
  "volume": 0.8
}' localhost:50051 device.DeviceControlService/PlayVoice

# 获取设备列表
grpcurl -plaintext localhost:50051 device.DeviceControlService/ListDevices
```

## 配置

### 环境变量

- `PORT`: 服务器端口 (默认: `:50051`)

### 模拟设备

服务器启动时会自动创建两个模拟设备：
- `xiaozhi-001`: 小智设备-客厅 (192.168.1.100:8080)
- `xiaozhi-002`: 小智设备-卧室 (192.168.1.101:8080)

## 开发说明

### 修改 Protocol Buffers

1. 编辑 `proto/device.proto` 文件
2. 运行 `make proto` 重新生成代码
3. 更新相应的服务实现

### 添加新功能

1. 在 `proto/device.proto` 中定义新的 RPC 方法
2. 在 `internal/device_service.go` 中实现方法
3. 重新编译和测试

## 故障排除

### 常见问题

1. **protoc 未找到**
   ```bash
   # macOS
   brew install protobuf
   
   # 或者使用 make 命令安装 Go 工具
   make install-protoc
   ```

2. **端口被占用**
   ```bash
   # 修改端口
   PORT=:50052 ./bin/server
   ```

3. **连接失败**
   - 检查服务器是否正在运行
   - 确认端口号是否正确
   - 检查防火墙设置

## 许可证

MIT License