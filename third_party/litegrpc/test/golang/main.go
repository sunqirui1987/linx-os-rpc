/**
 * @file main.go
 * @brief 云端 Hello 能力调用服务实现
 * 
 * 此文件实现了云端服务，作为RPC调用方主动调用设备端的Hello能力。
 * 正确的架构：云端作为调用方，设备端作为服务提供方。
 * 
 * 云端功能：
 * - 接受设备端连接和注册
 * - 管理已连接的设备列表
 * - 主动调用设备端的Hello能力
 * - 提供设备工具调用接口
 * - 支持环境变量配置监听端口
 * 
 * 使用方法：
 *   go run main.go                    # 云端默认监听 50051 端口
 *   PORT=50052 go run main.go         # 云端监听 50052 端口
 * 
 * 测试场景：
 * - 设备端连接到云端并注册Hello能力
 * - 云端主动调用设备端的Hello能力
 * - 验证"反向gRPC"架构的正确性
 * 
 * @author LiteGRPC Team
 * @date 2024
 * @version 2.0
 */
package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"os"
	"sync"
	"time"

	"google.golang.org/grpc"
	pb "hello-server/hello"  // 导入生成的 protobuf 代码
)

/**
 * @brief Device 结构体 - 已连接设备信息
 */
type Device struct {
	ID             string    `json:"id"`
	Name           string    `json:"name"`
	IPAddress      string    `json:"ip_address"`
	Port           int32     `json:"port"`
	Status         string    `json:"status"`
	LastHeartbeat  time.Time `json:"last_heartbeat"`
	AvailableTools []string  `json:"available_tools"`
}

/**
 * @brief DeviceManager 结构体 - 设备管理器
 */
type DeviceManager struct {
	devices map[string]*Device
	mutex   sync.RWMutex
}

/**
 * @brief NewDeviceManager 创建新的设备管理器
 */
func NewDeviceManager() *DeviceManager {
	return &DeviceManager{
		devices: make(map[string]*Device),
	}
}

/**
 * @brief RegisterDevice 注册设备
 */
func (dm *DeviceManager) RegisterDevice(device *Device) {
	dm.mutex.Lock()
	defer dm.mutex.Unlock()
	dm.devices[device.ID] = device
	log.Printf("设备已注册: %s (%s)", device.ID, device.Name)
}

/**
 * @brief GetDevice 获取设备信息
 */
func (dm *DeviceManager) GetDevice(deviceID string) (*Device, error) {
	dm.mutex.RLock()
	defer dm.mutex.RUnlock()
	device, exists := dm.devices[deviceID]
	if !exists {
		return nil, fmt.Errorf("device not found: %s", deviceID)
	}
	return device, nil
}

/**
 * @brief ListDevices 列出所有设备
 */
func (dm *DeviceManager) ListDevices() []*Device {
	dm.mutex.RLock()
	defer dm.mutex.RUnlock()
	devices := make([]*Device, 0, len(dm.devices))
	for _, device := range dm.devices {
		devices = append(devices, device)
	}
	return devices
}

/**
 * @brief HelloCloudServer 结构体 - 云端Hello调用服务实现
 * 
 * 实现云端服务，作为RPC调用方主动调用设备端的Hello能力。
 * 与传统gRPC不同，这里云端是调用方，设备端是服务提供方。
 */
type HelloCloudServer struct {
	pb.UnimplementedHelloServiceServer
	deviceManager *DeviceManager
}

/**
 * @brief NewHelloCloudServer 创建新的云端Hello服务
 */
func NewHelloCloudServer() *HelloCloudServer {
	return &HelloCloudServer{
		deviceManager: NewDeviceManager(),
	}
}

/**
 * @brief RegisterDevice RPC方法 - 设备注册接口
 * 
 * 设备端连接到云端时调用此方法进行注册，告知云端自己的Hello能力。
 */
func (s *HelloCloudServer) RegisterDevice(ctx context.Context, req *pb.HelloRequest) (*pb.HelloResponse, error) {
	// 解析设备注册信息（使用HelloRequest的字段模拟设备信息）
	deviceID := req.GetName()
	deviceInfo := req.GetMessage()
	
	log.Printf("云端收到设备注册请求: device_id=%s, info=%s", deviceID, deviceInfo)
	
	// 创建设备对象
	device := &Device{
		ID:             deviceID,
		Name:           fmt.Sprintf("Hello设备_%s", deviceID),
		IPAddress:      "127.0.0.1", // 简化实现，实际应从连接中获取
		Port:           50051,
		Status:         "connected",
		LastHeartbeat:  time.Now(),
		AvailableTools: []string{"say_hello"}, // 设备提供Hello能力
	}
	
	// 注册设备
	s.deviceManager.RegisterDevice(device)
	
	// 返回注册成功响应
	reply := fmt.Sprintf("云端确认: 设备 %s 已成功注册Hello能力", deviceID)
	return &pb.HelloResponse{
		Reply:  reply,
		Status: 0, // 0 表示成功
	}, nil
}

/**
 * @brief SayHello RPC方法 - 云端调用设备Hello能力的接口
 * 
 * 此方法被外部调用，云端收到请求后会转发给已注册的设备端。
 * 这里演示了"反向调用"：云端作为中介，调用设备端的Hello能力。
 */
func (s *HelloCloudServer) SayHello(ctx context.Context, req *pb.HelloRequest) (*pb.HelloResponse, error) {
	log.Printf("云端收到Hello调用请求: name=%s, message=%s", req.GetName(), req.GetMessage())
	
	// 获取可用设备列表
	devices := s.deviceManager.ListDevices()
	if len(devices) == 0 {
		return &pb.HelloResponse{
			Reply:  "云端错误: 没有可用的Hello设备",
			Status: 404,
		}, nil
	}
	
	// 选择第一个可用设备（简化实现）
	device := devices[0]
	log.Printf("云端选择设备: %s 来处理Hello请求", device.ID)
	
	// 调用设备端的Hello能力
	result, err := s.callDeviceHello(device, req.GetName(), req.GetMessage())
	if err != nil {
		return &pb.HelloResponse{
			Reply:  fmt.Sprintf("云端调用设备失败: %v", err),
			Status: 500,
		}, nil
	}
	
	log.Printf("云端收到设备响应: %s", result)
	return &pb.HelloResponse{
		Reply:  fmt.Sprintf("云端转发设备响应: %s", result),
		Status: 0,
	}, nil
}

/**
 * @brief callDeviceHello 调用设备Hello能力的内部方法
 * 
 * 模拟云端调用设备端Hello能力的过程。
 * 在实际实现中，这里会通过网络调用设备的具体Hello接口。
 */
func (s *HelloCloudServer) callDeviceHello(device *Device, name, message string) (string, error) {
	log.Printf("云端正在调用设备 %s 的Hello能力...", device.ID)
	
	// 检查设备是否支持Hello能力
	hasHelloTool := false
	for _, tool := range device.AvailableTools {
		if tool == "say_hello" {
			hasHelloTool = true
			break
		}
	}
	
	if !hasHelloTool {
		return "", fmt.Errorf("设备 %s 不支持Hello能力", device.ID)
	}
	
	// 模拟网络调用延迟
	time.Sleep(100 * time.Millisecond)
	
	// 模拟设备端Hello响应
	// 在实际实现中，这里会通过gRPC客户端调用设备端的Hello服务
	deviceResponse := map[string]interface{}{
		"success": true,
		"message": fmt.Sprintf("设备端Hello响应: Hello %s! 设备 %s 已收到你的消息: %s", 
			name, device.ID, message),
		"device_id": device.ID,
		"timestamp": time.Now().Unix(),
	}
	
	responseBytes, _ := json.Marshal(deviceResponse)
	return string(responseBytes), nil
}

/**
 * @brief 主函数 - 云端Hello调用服务启动入口
 * 
 * 云端服务主要功能：
 * 1. 启动gRPC服务器，接受设备端连接
 * 2. 提供设备注册接口
 * 3. 提供Hello能力调用接口（转发给设备端）
 * 4. 管理已连接设备的状态
 * 
 * 环境变量支持：
 * - PORT: 指定云端监听端口（默认：50051）
 * 
 * 使用示例：
 *   go run main.go                    # 云端监听 50051 端口
 *   PORT=50052 go run main.go         # 云端监听 50052 端口
 * 
 * 云端服务特性：
 * - 作为RPC调用方，主动调用设备端Hello能力
 * - 支持多设备连接和管理
 * - 提供设备注册和能力发现
 * - 实现"反向gRPC"架构模式
 */
func main() {
	// 从环境变量获取监听端口
	port := os.Getenv("PORT")
	if port == "" {
		port = "50051"  // 默认端口
	}
	address := ":" + port
	
	// 创建 TCP 监听器
	lis, err := net.Listen("tcp", address)
	if err != nil {
		log.Fatalf("监听失败: %v", err)
	}

	// 创建 gRPC 服务器实例
	s := grpc.NewServer()

	// 注册云端Hello服务实现
	cloudService := NewHelloCloudServer()
	pb.RegisterHelloServiceServer(s, cloudService)

	// 输出云端服务启动信息
	log.Printf("云端Hello调用服务启动，监听端口 %s", address)
	log.Printf("等待设备端连接并注册Hello能力...")
	log.Printf("云端支持的接口:")
	log.Printf("  - RegisterDevice: 设备注册Hello能力")
	log.Printf("  - SayHello: 调用设备端Hello能力")
	log.Printf("架构模式: 云端作为调用方，设备端作为服务提供方")

	// 启动定时任务：演示云端主动调用设备Hello能力
	go func() {
		time.Sleep(5 * time.Second) // 等待设备连接
		for {
			time.Sleep(10 * time.Second)
			devices := cloudService.deviceManager.ListDevices()
			if len(devices) > 0 {
				log.Printf("云端定时任务: 发现 %d 个已连接设备", len(devices))
				// 这里可以添加定时调用设备Hello能力的逻辑
			}
		}
	}()

	// 启动云端服务并开始接受设备端连接
	if err := s.Serve(lis); err != nil {
		log.Fatalf("云端服务启动失败: %v", err)
	}
}