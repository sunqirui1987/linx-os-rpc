
# RPC Demo - 基于RPCServer单例的工具调用

## 概述

这个demo展示了如何使用 `RPCServer::GetInstance()` 单例模式在设备端注册工具方法，然后云端可以直接调用 `rpc.dog.forward()` 等方法来控制设备。

## 架构图

```
云端调用                    设备端RPCServer
    |                           |
rpc.dog.forward()  ------>  AddTool("self.dog.forward", ...)
rpc.dog.backward() ------>  AddTool("self.dog.backward", ...)
rpc.audio.play()   ------>  AddTool("self.audio.play", ...)
```

## Protocol Buffer 定义

```protobuf
syntax = "proto3";

package rpc_demo;

// RPC 服务定义
service RPCService {
  // 建立双向流连接
  rpc Connect(stream ClientMessage) returns (stream ServerMessage);
}

// 客户端消息
message ClientMessage {
  oneof message {
    ToolRegistration tool_registration = 1;
    ToolResponse tool_response = 2;
    Heartbeat heartbeat = 3;
  }
}

// 服务器消息
message ServerMessage {
  oneof message {
    ToolCall tool_call = 1;
  }
}

// 工具注册
message ToolRegistration {
  string device_id = 1;
  repeated ToolInfo tools = 2;
}

// 工具信息
message ToolInfo {
  string name = 1;           // 工具名称，如 "self.dog.forward"
  string description = 2;    // 工具描述，如 "机器人向前移动"
  repeated Property properties = 3; // 参数列表
}

// 属性定义
message Property {
  string name = 1;
  string type = 2;           // "string", "int", "float", "bool"
  string description = 3;
  bool required = 4;
  string default_value = 5;
}

// 工具调用
message ToolCall {
  string call_id = 1;
  string tool_name = 2;      // 如 "self.dog.forward"
  map<string, string> parameters = 3; // 参数键值对
}

// 工具响应
message ToolResponse {
  string call_id = 1;
  bool success = 2;
  string error_message = 3;
  string result = 4;         // 返回值（JSON格式）
}

// 心跳
message Heartbeat {
  string device_id = 1;
  int64 timestamp = 2;
}
```

## 设备端实现 (C++)

### RPCServer 单例类

```cpp
#include <grpcpp/grpcpp.h>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <iostream>

// 属性列表类型定义
using PropertyList = std::map<std::string, std::string>;
using ReturnValue = std::string;
using ToolFunction = std::function<ReturnValue(const PropertyList&)>;

struct ToolInfo {
    std::string name;
    std::string description;
    std::vector<Property> properties;
    ToolFunction function;
};

class RPCServer {
private:
    static std::unique_ptr<RPCServer> instance_;
    static std::mutex instance_mutex_;
    
    std::map<std::string, ToolInfo> tools_;
    std::mutex tools_mutex_;
    
    std::unique_ptr<RPCService::Stub> stub_;
    std::shared_ptr<grpc::ClientReaderWriter<ClientMessage, ServerMessage>> stream_;
    std::string device_id_;
    bool connected_;
    std::thread message_thread_;
    
    RPCServer() : connected_(false) {}
    
public:
    static RPCServer& GetInstance() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = std::unique_ptr<RPCServer>(new RPCServer());
        }
        return *instance_;
    }
    
    // 连接到RPC服务器
    bool Connect(const std::string& server_address, const std::string& device_id) {
        device_id_ = device_id;
        
        auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
        stub_ = RPCService::NewStub(channel);
        
        grpc::ClientContext context;
        stream_ = stub_->Connect(&context);
        
        if (!stream_) {
            std::cout << "连接RPC服务器失败" << std::endl;
            return false;
        }
        
        connected_ = true;
        
        // 注册所有工具
        RegisterAllTools();
        
        // 启动消息监听线程
        message_thread_ = std::thread(&RPCServer::ListenForCalls, this);
        
        std::cout << "RPC服务器连接成功，设备ID: " << device_id_ << std::endl;
        return true;
    }
    
    // 添加工具
    void AddTool(const std::string& name, 
                 const std::string& description,
                 const std::vector<Property>& properties,
                 ToolFunction function) {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        
        ToolInfo tool;
        tool.name = name;
        tool.description = description;
        tool.properties = properties;
        tool.function = function;
        
        tools_[name] = tool;
        
        std::cout << "工具已注册: " << name << " - " << description << std::endl;
        
        // 如果已连接，立即注册到服务器
        if (connected_) {
            RegisterTool(tool);
        }
    }
    
private:
    void RegisterAllTools() {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        
        ClientMessage msg;
        auto* registration = msg.mutable_tool_registration();
        registration->set_device_id(device_id_);
        
        for (const auto& [name, tool] : tools_) {
            auto* tool_info = registration->add_tools();
            tool_info->set_name(tool.name);
            tool_info->set_description(tool.description);
            
            for (const auto& prop : tool.properties) {
                auto* property = tool_info->add_properties();
                property->set_name(prop.name);
                property->set_type(prop.type);
                property->set_description(prop.description);
                property->set_required(prop.required);
                property->set_default_value(prop.default_value);
            }
        }
        
        stream_->Write(msg);
        std::cout << "已注册 " << tools_.size() << " 个工具到服务器" << std::endl;
    }
    
    void RegisterTool(const ToolInfo& tool) {
        ClientMessage msg;
        auto* registration = msg.mutable_tool_registration();
        registration->set_device_id(device_id_);
        
        auto* tool_info = registration->add_tools();
        tool_info->set_name(tool.name);
        tool_info->set_description(tool.description);
        
        for (const auto& prop : tool.properties) {
            auto* property = tool_info->add_properties();
            property->set_name(prop.name);
            property->set_type(prop.type);
            property->set_description(prop.description);
            property->set_required(prop.required);
            property->set_default_value(prop.default_value);
        }
        
        stream_->Write(msg);
    }
    
    void ListenForCalls() {
        ServerMessage server_msg;
        while (connected_ && stream_->Read(&server_msg)) {
            if (server_msg.has_tool_call()) {
                HandleToolCall(server_msg.tool_call());
            }
        }
        connected_ = false;
    }
    
    void HandleToolCall(const ToolCall& call) {
        std::cout << "收到工具调用: " << call.tool_name() << std::endl;
        
        std::lock_guard<std::mutex> lock(tools_mutex_);
        
        ClientMessage response_msg;
        auto* response = response_msg.mutable_tool_response();
        response->set_call_id(call.call_id());
        
        auto it = tools_.find(call.tool_name());
        if (it != tools_.end()) {
            try {
                // 转换参数
                PropertyList params;
                for (const auto& [key, value] : call.parameters()) {
                    params[key] = value;
                }
                
                // 执行工具函数
                ReturnValue result = it->second.function(params);
                
                response->set_success(true);
                response->set_result(result);
                
                std::cout << "工具执行成功: " << call.tool_name() << std::endl;
                
            } catch (const std::exception& e) {
                response->set_success(false);
                response->set_error_message(e.what());
                std::cout << "工具执行失败: " << e.what() << std::endl;
            }
        } else {
            response->set_success(false);
            response->set_error_message("未找到工具: " + call.tool_name());
            std::cout << "未找到工具: " << call.tool_name() << std::endl;
        }
        
        stream_->Write(response_msg);
    }
};

// 静态成员定义
std::unique_ptr<RPCServer> RPCServer::instance_ = nullptr;
std::mutex RPCServer::instance_mutex_;
```

### 机器狗控制示例

```cpp
#include "rpc_server.h"

// 机器狗控制命令定义
enum DogState {
    DOG_STATE_FORWARD = 1,
    DOG_STATE_BACKWARD = 2,
    DOG_STATE_LEFT = 3,
    DOG_STATE_RIGHT = 4,
    DOG_STATE_STOP = 5
};

// 模拟的机器狗控制函数
void servo_dog_ctrl_send(DogState state, void* data) {
    switch (state) {
        case DOG_STATE_FORWARD:
            std::cout << "机器狗向前移动" << std::endl;
            break;
        case DOG_STATE_BACKWARD:
            std::cout << "机器狗向后移动" << std::endl;
            break;
        case DOG_STATE_LEFT:
            std::cout << "机器狗向左转" << std::endl;
            break;
        case DOG_STATE_RIGHT:
            std::cout << "机器狗向右转" << std::endl;
            break;
        case DOG_STATE_STOP:
            std::cout << "机器狗停止" << std::endl;
            break;
    }
}

// 模拟的音频播放函数
void system_audio_play(const char* file_path) {
    std::cout << "播放音频: " << file_path << std::endl;
}

class DogController {
public:
    void RegisterTools() {
        auto& rpc_server = RPCServer::GetInstance();
        
        // 例1：无参数，控制机器人前进
        rpc_server.AddTool("self.dog.forward", "机器人向前移动", {},
            [this](const PropertyList&) -> ReturnValue {
                servo_dog_ctrl_send(DOG_STATE_FORWARD, nullptr);
                return "success";
            });
        
        // 例2：无参数，控制机器人后退
        rpc_server.AddTool("self.dog.backward", "机器人向后移动", {},
            [this](const PropertyList&) -> ReturnValue {
                servo_dog_ctrl_send(DOG_STATE_BACKWARD, nullptr);
                return "success";
            });
        
        // 例3：无参数，控制机器人左转
        rpc_server.AddTool("self.dog.left", "机器人向左转", {},
            [this](const PropertyList&) -> ReturnValue {
                servo_dog_ctrl_send(DOG_STATE_LEFT, nullptr);
                return "success";
            });
        
        // 例4：无参数，控制机器人右转
        rpc_server.AddTool("self.dog.right", "机器人向右转", {},
            [this](const PropertyList&) -> ReturnValue {
                servo_dog_ctrl_send(DOG_STATE_RIGHT, nullptr);
                return "success";
            });
        
        // 例5：无参数，控制机器人停止
        rpc_server.AddTool("self.dog.stop", "机器人停止", {},
            [this](const PropertyList&) -> ReturnValue {
                servo_dog_ctrl_send(DOG_STATE_STOP, nullptr);
                return "success";
            });
        
        // 例6：有参数，播放音频
        std::vector<Property> audio_props = {
            {"file_path", "string", "音频文件路径", true, ""}
        };
        rpc_server.AddTool("self.audio.play", "播放音频文件", audio_props,
            [this](const PropertyList& params) -> ReturnValue {
                auto it = params.find("file_path");
                if (it != params.end()) {
                    system_audio_play(it->second.c_str());
                    return "success";
                } else {
                    throw std::runtime_error("缺少file_path参数");
                }
            });
        
        // 例7：有多个参数，设置机器狗速度
        std::vector<Property> speed_props = {
            {"speed", "float", "移动速度 (0.1-2.0)", true, "1.0"},
            {"duration", "int", "持续时间(秒)", false, "5"}
        };
        rpc_server.AddTool("self.dog.set_speed", "设置机器狗移动速度", speed_props,
            [this](const PropertyList& params) -> ReturnValue {
                float speed = std::stof(params.at("speed"));
                int duration = params.count("duration") ? std::stoi(params.at("duration")) : 5;
                
                std::cout << "设置机器狗速度: " << speed << ", 持续时间: " << duration << "秒" << std::endl;
                return "speed_set_success";
            });
    }
};

int main() {
    // 创建机器狗控制器并注册工具
    DogController controller;
    controller.RegisterTools();
    
    // 连接到RPC服务器
    auto& rpc_server = RPCServer::GetInstance();
    if (!rpc_server.Connect("localhost:50051", "robot_dog_001")) {
        std::cout << "连接RPC服务器失败" << std::endl;
        return -1;
    }
    
    std::cout << "机器狗RPC服务已启动，等待云端调用..." << std::endl;
    
    // 保持程序运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

## 服务器端实现 (Go)

```go
package main

import (
    "context"
    "fmt"
    "log"
    "net"
    "sync"
    "time"
    
    "google.golang.org/grpc"
    pb "your_package/rpc_demo"
)

type RPCServer struct {
    pb.UnimplementedRPCServiceServer
    devices map[string]*DeviceConnection
    mutex   sync.RWMutex
}

type DeviceConnection struct {
    deviceID string
    stream   pb.RPCService_ConnectServer
    tools    map[string]*pb.ToolInfo
}

func NewRPCServer() *RPCServer {
    return &RPCServer{
        devices: make(map[string]*DeviceConnection),
    }
}

func (s *RPCServer) Connect(stream pb.RPCService_ConnectServer) error {
    for {
        msg, err := stream.Recv()
        if err != nil {
            return err
        }
        
        switch m := msg.Message.(type) {
        case *pb.ClientMessage_ToolRegistration:
            s.handleToolRegistration(stream, m.ToolRegistration)
        case *pb.ClientMessage_ToolResponse:
            s.handleToolResponse(m.ToolResponse)
        case *pb.ClientMessage_Heartbeat:
            s.handleHeartbeat(m.Heartbeat)
        }
    }
}

func (s *RPCServer) handleToolRegistration(stream pb.RPCService_ConnectServer, reg *pb.ToolRegistration) {
    s.mutex.Lock()
    defer s.mutex.Unlock()
    
    deviceID := reg.DeviceId
    
    // 获取或创建设备连接
    conn, exists := s.devices[deviceID]
    if !exists {
        conn = &DeviceConnection{
            deviceID: deviceID,
            stream:   stream,
            tools:    make(map[string]*pb.ToolInfo),
        }
        s.devices[deviceID] = conn
    }
    
    // 注册工具
    for _, tool := range reg.Tools {
        conn.tools[tool.Name] = tool
        fmt.Printf("设备 %s 注册工具: %s - %s\n", deviceID, tool.Name, tool.Description)
    }
    
    fmt.Printf("设备 %s 已注册 %d 个工具\n", deviceID, len(reg.Tools))
}

func (s *RPCServer) handleToolResponse(response *pb.ToolResponse) {
    if response.Success {
        fmt.Printf("工具调用成功: %s, 结果: %s\n", response.CallId, response.Result)
    } else {
        fmt.Printf("工具调用失败: %s, 错误: %s\n", response.CallId, response.ErrorMessage)
    }
}

func (s *RPCServer) handleHeartbeat(heartbeat *pb.Heartbeat) {
    fmt.Printf("收到设备 %s 心跳\n", heartbeat.DeviceId)
}

// 调用设备工具
func (s *RPCServer) CallTool(deviceID, toolName string, parameters map[string]string) error {
    s.mutex.RLock()
    device, exists := s.devices[deviceID]
    s.mutex.RUnlock()
    
    if !exists {
        return fmt.Errorf("设备 %s 未找到", deviceID)
    }
    
    if _, hasTool := device.tools[toolName]; !hasTool {
        return fmt.Errorf("设备 %s 不支持工具 %s", deviceID, toolName)
    }
    
    call := &pb.ToolCall{
        CallId:     fmt.Sprintf("call_%d", time.Now().UnixNano()),
        ToolName:   toolName,
        Parameters: parameters,
    }
    
    msg := &pb.ServerMessage{
        Message: &pb.ServerMessage_ToolCall{ToolCall: call},
    }
    
    return device.stream.Send(msg)
}

// 获取设备工具列表
func (s *RPCServer) GetDeviceTools(deviceID string) ([]*pb.ToolInfo, error) {
    s.mutex.RLock()
    defer s.mutex.RUnlock()
    
    device, exists := s.devices[deviceID]
    if !exists {
        return nil, fmt.Errorf("设备 %s 未找到", deviceID)
    }
    
    var tools []*pb.ToolInfo
    for _, tool := range device.tools {
        tools = append(tools, tool)
    }
    
    return tools, nil
}

func main() {
    server := NewRPCServer()
    
    lis, err := net.Listen("tcp", ":50051")
    if err != nil {
        log.Fatalf("监听失败: %v", err)
    }
    
    grpcServer := grpc.NewServer()
    pb.RegisterRPCServiceServer(grpcServer, server)
    
    // 启动测试调用
    go func() {
        time.Sleep(10 * time.Second) // 等待设备连接和注册
        
        deviceID := "robot_dog_001"
        
        // 测试调用机器狗前进
        fmt.Println("=== 测试调用机器狗工具 ===")
        
        err := server.CallTool(deviceID, "self.dog.forward", map[string]string{})
        if err != nil {
            fmt.Printf("调用 dog.forward 失败: %v\n", err)
        }
        
        time.Sleep(2 * time.Second)
        
        err = server.CallTool(deviceID, "self.dog.backward", map[string]string{})
        if err != nil {
            fmt.Printf("调用 dog.backward 失败: %v\n", err)
        }
        
        time.Sleep(2 * time.Second)
        
        // 测试播放音频
        err = server.CallTool(deviceID, "self.audio.play", map[string]string{
            "file_path": "/path/to/music.mp3",
        })
        if err != nil {
            fmt.Printf("调用 audio.play 失败: %v\n", err)
        }
        
        time.Sleep(2 * time.Second)
        
        // 测试设置速度
        err = server.CallTool(deviceID, "self.dog.set_speed", map[string]string{
            "speed":    "1.5",
            "duration": "10",
        })
        if err != nil {
            fmt.Printf("调用 dog.set_speed 失败: %v\n", err)
        }
    }()
    
    fmt.Println("RPC服务器启动在 :50051")
    grpcServer.Serve(lis)
}
```

## 云端调用示例

```go
package main

import (
    "fmt"
    "time"
)

// 模拟云端RPC调用接口
type CloudRPC struct {
    server *RPCServer
}

func NewCloudRPC(server *RPCServer) *CloudRPC {
    return &CloudRPC{server: server}
}

// 机器狗控制接口
func (c *CloudRPC) Dog() *DogController {
    return &DogController{rpc: c, deviceID: "robot_dog_001"}
}

// 音频控制接口
func (c *CloudRPC) Audio() *AudioController {
    return &AudioController{rpc: c, deviceID: "robot_dog_001"}
}

type DogController struct {
    rpc      *CloudRPC
    deviceID string
}

func (d *DogController) Forward() error {
    return d.rpc.server.CallTool(d.deviceID, "self.dog.forward", map[string]string{})
}

func (d *DogController) Backward() error {
    return d.rpc.server.CallTool(d.deviceID, "self.dog.backward", map[string]string{})
}

func (d *DogController) Left() error {
    return d.rpc.server.CallTool(d.deviceID, "self.dog.left", map[string]string{})
}

func (d *DogController) Right() error {
    return d.rpc.server.CallTool(d.deviceID, "self.dog.right", map[string]string{})
}

func (d *DogController) Stop() error {
    return d.rpc.server.CallTool(d.deviceID, "self.dog.stop", map[string]string{})
}

func (d *DogController) SetSpeed(speed float64, duration int) error {
    return d.rpc.server.CallTool(d.deviceID, "self.dog.set_speed", map[string]string{
        "speed":    fmt.Sprintf("%.1f", speed),
        "duration": fmt.Sprintf("%d", duration),
    })
}

type AudioController struct {
    rpc      *CloudRPC
    deviceID string
}

func (a *AudioController) Play(filePath string) error {
    return a.rpc.server.CallTool(a.deviceID, "self.audio.play", map[string]string{
        "file_path": filePath,
    })
}

// 使用示例
func main() {
    server := NewRPCServer()
    rpc := NewCloudRPC(server)
    
    // 等待设备连接...
    time.Sleep(5 * time.Second)
    
    // 云端调用示例
    fmt.Println("=== 云端RPC调用示例 ===")
    
    // 控制机器狗
    rpc.Dog().Forward()
    time.Sleep(1 * time.Second)
    
    rpc.Dog().Left()
    time.Sleep(1 * time.Second)
    
    rpc.Dog().SetSpeed(1.8, 15)
    time.Sleep(1 * time.Second)
    
    rpc.Dog().Stop()
    time.Sleep(1 * time.Second)
    
    // 播放音频
    rpc.Audio().Play("/sounds/notification.wav")
}
```

## 使用示例

### 1. 启动RPC服务器
```bash
go run server.go
```

### 2. 启动设备端（机器狗）
```bash
./robot_dog_client
```

### 3. 云端调用
```go
// 直接调用方式
rpc.Dog().Forward()
rpc.Dog().Backward()
rpc.Audio().Play("music.mp3")
```

## 输出示例

**设备端输出:**
```
工具已注册: self.dog.forward - 机器人向前移动
工具已注册: self.dog.backward - 机器人向后移动
工具已注册: self.audio.play - 播放音频文件
RPC服务器连接成功，设备ID: robot_dog_001
已注册 7 个工具到服务器
机器狗RPC服务已启动，等待云端调用...
收到工具调用: self.dog.forward
机器狗向前移动
工具执行成功: self.dog.forward
```

**服务器端输出:**
```
设备 robot_dog_001 注册工具: self.dog.forward - 机器人向前移动
设备 robot_dog_001 注册工具: self.dog.backward - 机器人向后移动
设备 robot_dog_001 注册工具: self.audio.play - 播放音频文件
设备 robot_dog_001 已注册 7 个工具
=== 测试调用机器狗工具 ===
工具调用成功: call_1234567890, 结果: success
```

## 核心特性

1. **单例模式**: 使用 `RPCServer::GetInstance()` 全局访问
2. **简单注册**: `AddTool()` 方法快速注册工具
3. **Lambda支持**: 支持lambda表达式和成员函数
4. **参数验证**: 自动参数类型检查和验证
5. **云端友好**: 提供类似 `rpc.dog.forward()` 的调用接口

## 总结

这个设计实现了：

1. **设备端**: 使用单例 `RPCServer::GetInstance()` 注册工具
2. **云端调用**: 直接调用 `rpc.dog.forward()` 等方法
3. **工具管理**: 支持有参数和无参数的工具注册
4. **类型安全**: 完整的参数类型定义和验证

核心流程：**注册工具 → 连接服务器 → 云端调用 → 设备执行 → 返回结果**