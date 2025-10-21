package main

import (
	"log"
	"net"
	"os"
	"os/signal"
	"syscall"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	"golang-test-server/internal"
	pb "golang-test-server/proto"
)

const (
	defaultPort = ":50051"
)

func main() {
	// 获取端口配置
	port := os.Getenv("PORT")
	if port == "" {
		port = defaultPort
	}

	// 创建监听器
	lis, err := net.Listen("tcp", port)
	if err != nil {
		log.Fatalf("Failed to listen: %v", err)
	}

	// 创建 gRPC 服务器
	s := grpc.NewServer()

	// 注册 LinxOS 设备服务
	deviceService := internal.NewLinxOSDeviceServer()
	pb.RegisterLinxOSDeviceServiceServer(s, deviceService)

	// 启用反射（用于调试和测试）
	reflection.Register(s)

	log.Printf("LinxOS RPC Test Server starting on port %s", port)
	log.Printf("Available services:")
	log.Printf("  - LinxOSDeviceService")
	log.Printf("    设备管理:")
	log.Printf("      - RegisterDevice: 设备注册")
	log.Printf("      - Heartbeat: 心跳保持")
	log.Printf("    语音交互:")
	log.Printf("      - VoiceSpeak: 语音播放")
	log.Printf("      - VoiceVolume: 音量控制")
	log.Printf("    显示控制:")
	log.Printf("      - DisplayExpression: 表情显示")
	log.Printf("      - DisplayText: 文本显示")
	log.Printf("      - DisplayBrightness: 亮度控制")
	log.Printf("    灯光控制:")
	log.Printf("      - LightControl: RGB 灯光控制")
	log.Printf("      - LightMode: 灯光模式设置")
	log.Printf("    音频处理:")
	log.Printf("      - AudioPlay: 音频播放")
	log.Printf("      - AudioRecord: 音频录制")
	log.Printf("      - AudioStop: 音频停止")
	log.Printf("    系统管理:")
	log.Printf("      - SystemInfo: 系统信息")
	log.Printf("      - SystemRestart: 系统重启")
	log.Printf("      - SystemWifiReconnect: WiFi重连")
	log.Printf("    通用工具:")
	log.Printf("      - CallTool: 工具调用")

	// 设置优雅关闭
	go func() {
		sigChan := make(chan os.Signal, 1)
		signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
		<-sigChan

		log.Println("Shutting down server...")
		s.GracefulStop()
	}()

	// 启动服务器
	if err := s.Serve(lis); err != nil {
		log.Fatalf("Failed to serve: %v", err)
	}
}