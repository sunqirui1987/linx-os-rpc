/**
 * @file main.go
 * @brief 设备端 Hello 能力服务实现
 * 
 * 此文件实现了设备端的 Hello 能力服务，模拟真实设备提供 Hello 功能。
 * 作为设备端，接收并响应来自 C++ LiteGRPC 客户端的 Hello 能力调用请求。
 * 
 * 设备端功能：
 * - 实现设备的 Hello 能力，提供 SayHello RPC 方法
 * - 接收来自客户端的 Hello 调用请求
 * - 模拟设备响应客户端的 Hello 能力调用
 * - 支持环境变量配置监听端口
 * - 提供详细的设备端日志输出
 * 
 * 使用方法：
 *   go run main.go                    # 设备端默认监听 50051 端口
 *   PORT=50052 go run main.go         # 设备端监听 50052 端口
 * 
 * 测试场景：
 * - LiteGRPC 客户端与设备端的双向通信测试
 * - 设备端 Hello 能力的功能验证
 * - 中文消息在设备端的处理测试
 * - 设备端并发处理能力测试
 * 
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 */
package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"os"

	"google.golang.org/grpc"
	pb "hello-server/hello"  // 导入生成的 protobuf 代码
)

/**
 * @brief HelloServer 结构体 - 设备端 Hello 能力实现
 * 
 * 实现设备端的 Hello 能力服务，模拟真实设备的 Hello 功能。
 * 作为设备端服务提供者，响应来自客户端的 Hello 能力调用请求。
 * 嵌入 UnimplementedHelloServiceServer 以确保向前兼容性，
 * 当 proto 文件添加新的设备能力方法时，不会破坏现有实现。
 */
type HelloServer struct {
	pb.UnimplementedHelloServiceServer  // 嵌入未实现的服务器，提供向前兼容性
}

/**
 * @brief SayHello RPC 方法实现 - 设备端 Hello 能力响应
 * @param ctx 上下文，用于控制请求的生命周期和取消
 * @param req HelloRequest 请求消息，包含客户端发送的姓名和消息
 * @return HelloResponse 响应消息和可能的错误
 * 
 * 此方法实现了设备端 Hello 能力的核心响应逻辑：
 * 1. 接收并记录来自客户端的 Hello 能力调用请求
 * 2. 模拟设备处理 Hello 请求，构造设备端回复消息
 * 3. 创建设备响应对象并设置处理状态码
 * 4. 记录设备端响应内容并返回给客户端
 * 
 * 设备端特性：
 * - 模拟真实设备的 Hello 能力响应行为
 * - 支持中文和多语言消息在设备端的处理
 * - 提供详细的设备端请求/响应日志
 * - 返回结构化的设备处理状态信息
 * - 处理各种字符编码的设备端消息
 */
func (s *HelloServer) SayHello(ctx context.Context, req *pb.HelloRequest) (*pb.HelloResponse, error) {
	// 记录设备端收到的 Hello 能力调用请求
	log.Printf("设备端收到 Hello 能力调用: name=%s, message=%s", req.GetName(), req.GetMessage())
	
	// 模拟设备端处理 Hello 请求，构造设备回复消息
	reply := fmt.Sprintf("设备端 Hello 响应: Hello %s! 设备已收到你的消息: %s", req.GetName(), req.GetMessage())
	
	// 创建设备端响应对象
	response := &pb.HelloResponse{
		Reply:  reply,
		Status: 0, // 0 表示设备端成功处理
	}
	
	// 记录设备端发送的响应信息
	log.Printf("设备端发送响应: %s", reply)
	return response, nil
}

/**
 * @brief 主函数 - 设备端 Hello 能力服务启动入口
 * 
 * 设备端服务主要功能：
 * 1. 解析环境变量配置设备端监听端口
 * 2. 创建设备端 TCP 监听器
 * 3. 初始化设备端 gRPC 服务器
 * 4. 注册设备端 Hello 能力服务
 * 5. 启动设备端服务并开始监听客户端的 Hello 能力调用
 * 
 * 环境变量支持：
 * - PORT: 指定设备端监听端口（默认：50051）
 * 
 * 使用示例：
 *   go run main.go                    # 设备端监听 50051 端口
 *   PORT=50052 go run main.go         # 设备端监听 50052 端口
 *   PORT=8080 go run main.go          # 设备端监听 8080 端口
 * 
 * 设备端服务特性：
 * - 支持多客户端并发调用设备 Hello 能力
 * - 自动处理客户端连接管理
 * - 提供详细的设备端启动和运行日志
 * - 优雅的设备端错误处理和退出
 * - 模拟真实设备的 Hello 能力响应行为
 */
func main() {
	// 从环境变量获取监听端口，支持灵活配置
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
	// 使用默认配置，支持 HTTP/2 和标准 gRPC 协议
	s := grpc.NewServer()

	// 注册 HelloService 服务实现
	// 将我们的 HelloServer 实例注册到 gRPC 服务器
	pb.RegisterHelloServiceServer(s, &HelloServer{})

	// 输出设备端服务启动信息
	log.Printf("设备端 Hello 能力服务启动，监听端口 %s", address)
	log.Printf("等待 C++ 客户端调用设备 Hello 能力...")
	log.Printf("设备端支持的能力方法: SayHello")

	// 启动设备端服务并开始接受客户端的 Hello 能力调用
	// 此调用会阻塞，直到设备端服务关闭或发生错误
	if err := s.Serve(lis); err != nil {
		log.Fatalf("设备端服务启动失败: %v", err)
	}
}