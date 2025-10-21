package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"os"

	"google.golang.org/grpc"
	pb "hello-server/hello"
)

// HelloServer 实现 HelloService
type HelloServer struct {
	pb.UnimplementedHelloServiceServer
}

// SayHello 实现 SayHello RPC 方法
func (s *HelloServer) SayHello(ctx context.Context, req *pb.HelloRequest) (*pb.HelloResponse, error) {
	log.Printf("收到请求: name=%s, message=%s", req.GetName(), req.GetMessage())
	
	// 构造响应
	reply := fmt.Sprintf("Hello %s! 收到你的消息: %s", req.GetName(), req.GetMessage())
	
	response := &pb.HelloResponse{
		Reply:  reply,
		Status: 0, // 0 表示成功
	}
	
	log.Printf("发送响应: %s", reply)
	return response, nil
}

func main() {
	// 监听端口，支持环境变量配置
	port := os.Getenv("PORT")
	if port == "" {
		port = "50051"
	}
	address := ":" + port
	lis, err := net.Listen("tcp", address)
	if err != nil {
		log.Fatalf("监听失败: %v", err)
	}

	// 创建 gRPC 服务器
	s := grpc.NewServer()

	// 注册 HelloService
	pb.RegisterHelloServiceServer(s, &HelloServer{})

	log.Printf("Hello gRPC 服务器启动，监听端口 %s", address)
	log.Printf("等待 C++ 客户端连接...")

	// 启动服务器
	if err := s.Serve(lis); err != nil {
		log.Fatalf("服务器启动失败: %v", err)
	}
}