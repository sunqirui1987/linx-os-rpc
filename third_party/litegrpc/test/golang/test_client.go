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
	pb "test/hello"
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

	// 测试连