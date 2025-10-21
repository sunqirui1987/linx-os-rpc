/**
 * @file test_client.go
 * @brief Go 客户端测试程序
 * @details 用于测试设备端 Hello 服务器的 Go 客户端实现
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
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	pb "hello-server/hello"
)

func main() {
	// 连接到设备端服务器
	serverAddr := "localhost:50052"
	fmt.Printf("=== Go 客户端启动 ===\n")
	fmt.Printf("连接到设备端服务器: %s\n", serverAddr)

	// 建立连接
	conn, err := grpc.Dial(serverAddr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("连接失败: %v", err)
	}
	defer conn.Close()

	// 创建客户端
	client := pb.NewHelloServiceClient(conn)
	fmt.Printf("Go 客户端已创建，连接到: %s\n", serverAddr)

	// 测试连接
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	fmt.Printf("\n=== 开始设备端 Hello 能力调用测试 ===\n")

	// 发送 Hello 请求
	request := &pb.HelloRequest{
		Name:    "Go客户端",
		Message: "你好，设备端！",
	}

	fmt.Printf("\n--- 调用设备端 Hello 能力 ---\n")
	fmt.Printf("发送请求:\n")
	fmt.Printf("  Name: %s\n", request.Name)
	fmt.Printf("  Message: %s\n", request.Message)

	response, err := client.SayHello(ctx, request)
	if err != nil {
		log.Printf("设备端 Hello 能力调用失败: %v", err)
		return
	}

	fmt.Printf("\n收到设备端响应:\n")
	fmt.Printf("  Reply: %s\n", response.Reply)
	fmt.Printf("  Status: %d\n", response.Status)

	if response.Status == 0 {
		fmt.Printf("\n✅ 设备端 Hello 能力调用成功！\n")
	} else {
		fmt.Printf("\n❌ 设备端返回错误状态: %d\n", response.Status)
	}

	fmt.Printf("\n=== Go 客户端测试完成 ===\n")
}