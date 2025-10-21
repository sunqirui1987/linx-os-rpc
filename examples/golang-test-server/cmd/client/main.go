package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	pb "golang-test-server/proto"
)

var (
	addr     = flag.String("addr", "localhost:50051", "the address to connect to")
	deviceID = flag.String("device", "test-client-001", "device ID for testing")
)

func main() {
	flag.Parse()

	// 连接到服务器
	conn, err := grpc.NewClient(*addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()

	client := pb.NewLinxOSDeviceServiceClient(conn)
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	// 测试设备注册
	fmt.Println("=== Testing Device Registration ===")
	registerResp, err := testDeviceRegistration(ctx, client)
	if err != nil {
		log.Fatalf("Device registration failed: %v", err)
	}
	fmt.Printf("Registration successful. Session ID: %s\n", registerResp.SessionId)

	// 测试心跳
	fmt.Println("\n=== Testing Heartbeat ===")
	if err := testHeartbeat(ctx, client, registerResp.SessionId); err != nil {
		log.Printf("Heartbeat failed: %v", err)
	}

	// 测试语音功能
	fmt.Println("\n=== Testing Voice Functions ===")
	if err := testVoiceFunctions(ctx, client); err != nil {
		log.Printf("Voice functions test failed: %v", err)
	}

	// 测试显示功能
	fmt.Println("\n=== Testing Display Functions ===")
	if err := testDisplayFunctions(ctx, client); err != nil {
		log.Printf("Display functions test failed: %v", err)
	}

	// 测试灯光功能
	fmt.Println("\n=== Testing Light Functions ===")
	if err := testLightFunctions(ctx, client); err != nil {
		log.Printf("Light functions test failed: %v", err)
	}

	// 测试音频功能
	fmt.Println("\n=== Testing Audio Functions ===")
	if err := testAudioFunctions(ctx, client); err != nil {
		log.Printf("Audio functions test failed: %v", err)
	}

	// 测试系统功能
	fmt.Println("\n=== Testing System Functions ===")
	if err := testSystemFunctions(ctx, client); err != nil {
		log.Printf("System functions test failed: %v", err)
	}

	// 测试通用工具调用
	fmt.Println("\n=== Testing Generic Tool Call ===")
	if err := testGenericToolCall(ctx, client); err != nil {
		log.Printf("Generic tool call test failed: %v", err)
	}

	fmt.Println("\n=== All tests completed ===")
}

func testDeviceRegistration(ctx context.Context, client pb.LinxOSDeviceServiceClient) (*pb.RegisterDeviceResponse, error) {
	req := &pb.RegisterDeviceRequest{
		DeviceInfo: &pb.DeviceInfo{
			DeviceId:        *deviceID,
			DeviceName:      "测试客户端设备",
			DeviceType:      "test-client",
			FirmwareVersion: "1.0.0-test",
			IpAddress:       "127.0.0.1",
			Port:            8080,
			Capabilities: map[string]string{
				"voice":   "enabled",
				"display": "enabled",
				"light":   "enabled",
				"audio":   "enabled",
				"system":  "enabled",
			},
		},
		AvailableTools: []string{
			"voice_speak", "voice_volume",
			"display_expression", "display_text", "display_brightness",
			"light_control", "light_mode",
			"audio_play", "audio_record", "audio_stop",
			"system_info", "system_restart", "system_wifi_reconnect",
		},
	}

	resp, err := client.RegisterDevice(ctx, req)
	if err != nil {
		return nil, err
	}

	if !resp.Status.Success {
		return nil, fmt.Errorf("registration failed: %s", resp.Status.Message)
	}

	return resp, nil
}

func testHeartbeat(ctx context.Context, client pb.LinxOSDeviceServiceClient, sessionID string) error {
	req := &pb.HeartbeatRequest{
		DeviceId:  *deviceID,
		SessionId: sessionID,
		StatusInfo: map[string]string{
			"cpu_usage":    "25.5",
			"memory_usage": "60.2",
			"temperature":  "45.8",
		},
	}

	resp, err := client.Heartbeat(ctx, req)
	if err != nil {
		return err
	}

	if !resp.Status.Success {
		return fmt.Errorf("heartbeat failed: %s", resp.Status.Message)
	}

	fmt.Printf("Heartbeat successful. Keep alive: %v\n", resp.KeepAlive)
	return nil
}

func testVoiceFunctions(ctx context.Context, client pb.LinxOSDeviceServiceClient) error {
	// 测试语音播放
	speakReq := &pb.VoiceSpeakRequest{
		DeviceId:  *deviceID,
		Text:      "你好，这是一个测试消息",
		Speed:     1.0,
		Volume:    80,
		VoiceType: "female",
	}

	speakResp, err := client.VoiceSpeak(ctx, speakReq)
	if err != nil {
		return fmt.Errorf("voice speak failed: %v", err)
	}

	if !speakResp.Status.Success {
		return fmt.Errorf("voice speak failed: %s", speakResp.Status.Message)
	}

	fmt.Printf("Voice speak successful. Duration: %.2fs, Audio ID: %s\n", 
		speakResp.Duration, speakResp.AudioId)

	// 测试音量控制
	volumeReq := &pb.VoiceVolumeRequest{
		DeviceId: *deviceID,
		Volume:   90,
	}

	volumeResp, err := client.VoiceVolume(ctx, volumeReq)
	if err != nil {
		return fmt.Errorf("voice volume failed: %v", err)
	}

	if !volumeResp.Status.Success {
		return fmt.Errorf("voice volume failed: %s", volumeResp.Status.Message)
	}

	fmt.Printf("Voice volume set to: %d\n", volumeResp.CurrentVolume)
	return nil
}

func testDisplayFunctions(ctx context.Context, client pb.LinxOSDeviceServiceClient) error {
	// 测试显示表情
	exprReq := &pb.DisplayExpressionRequest{
		DeviceId:   *deviceID,
		Expression: "happy",
		Duration:   5.0,
	}

	exprResp, err := client.DisplayExpression(ctx, exprReq)
	if err != nil {
		return fmt.Errorf("display expression failed: %v", err)
	}

	if !exprResp.Status.Success {
		return fmt.Errorf("display expression failed: %s", exprResp.Status.Message)
	}

	fmt.Printf("Display expression successful\n")

	// 测试显示文本
	textReq := &pb.DisplayTextRequest{
		DeviceId: *deviceID,
		Text:     "Hello, LinxOS!",
		Duration: 3,
		FontSize: "16",
		Color:    "blue",
	}

	textResp, err := client.DisplayText(ctx, textReq)
	if err != nil {
		return fmt.Errorf("display text failed: %v", err)
	}

	if !textResp.Status.Success {
		return fmt.Errorf("display text failed: %s", textResp.Status.Message)
	}

	fmt.Printf("Display text successful\n")

	// 测试亮度控制
	brightnessReq := &pb.DisplayBrightnessRequest{
		DeviceId:   *deviceID,
		Brightness: 75,
	}

	brightnessResp, err := client.DisplayBrightness(ctx, brightnessReq)
	if err != nil {
		return fmt.Errorf("display brightness failed: %v", err)
	}

	if !brightnessResp.Status.Success {
		return fmt.Errorf("display brightness failed: %s", brightnessResp.Status.Message)
	}

	fmt.Printf("Display brightness set to: %d\n", brightnessResp.CurrentBrightness)
	return nil
}

func testLightFunctions(ctx context.Context, client pb.LinxOSDeviceServiceClient) error {
	// 测试灯光控制
	lightReq := &pb.LightControlRequest{
		DeviceId:   *deviceID,
		Red:        255,
		Green:      100,
		Blue:       50,
		Brightness: 80,
		Duration:   5,
	}

	lightResp, err := client.LightControl(ctx, lightReq)
	if err != nil {
		return fmt.Errorf("light control failed: %v", err)
	}

	if !lightResp.Status.Success {
		return fmt.Errorf("light control failed: %s", lightResp.Status.Message)
	}

	fmt.Printf("Light control successful\n")

	// 测试灯光模式
	modeReq := &pb.LightModeRequest{
		DeviceId: *deviceID,
		Mode:     "rainbow",
		Speed:    5,
		Colors:   []int32{255, 0, 0, 0, 255, 0, 0, 0, 255},
	}

	modeResp, err := client.LightMode(ctx, modeReq)
	if err != nil {
		return fmt.Errorf("light mode failed: %v", err)
	}

	if !modeResp.Status.Success {
		return fmt.Errorf("light mode failed: %s", modeResp.Status.Message)
	}

	fmt.Printf("Light mode set successfully\n")
	return nil
}

func testAudioFunctions(ctx context.Context, client pb.LinxOSDeviceServiceClient) error {
	// 测试音频播放
	playReq := &pb.AudioPlayRequest{
		DeviceId: *deviceID,
		FilePath: "/tmp/test.mp3",
		Volume:   0.7,
		Loop:     false,
	}

	playResp, err := client.AudioPlay(ctx, playReq)
	if err != nil {
		return fmt.Errorf("audio play failed: %v", err)
	}

	if !playResp.Status.Success {
		return fmt.Errorf("audio play failed: %s", playResp.Status.Message)
	}

	fmt.Printf("Audio play successful. Audio ID: %s, Duration: %.2fs\n", 
		playResp.AudioId, playResp.Duration)

	// 测试音频录制
	recordReq := &pb.AudioRecordRequest{
		DeviceId:   *deviceID,
		Duration:   5,
		SampleRate: 44100,
		Format:     "wav",
	}

	recordResp, err := client.AudioRecord(ctx, recordReq)
	if err != nil {
		return fmt.Errorf("audio record failed: %v", err)
	}

	if !recordResp.Status.Success {
		return fmt.Errorf("audio record failed: %s", recordResp.Status.Message)
	}

	fmt.Printf("Audio record successful. Record ID: %s, File: %s\n", 
		recordResp.RecordId, recordResp.FilePath)

	// 测试音频停止
	stopReq := &pb.AudioStopRequest{
		DeviceId: *deviceID,
		AudioId:  playResp.AudioId,
	}

	stopResp, err := client.AudioStop(ctx, stopReq)
	if err != nil {
		return fmt.Errorf("audio stop failed: %v", err)
	}

	if !stopResp.Status.Success {
		return fmt.Errorf("audio stop failed: %s", stopResp.Status.Message)
	}

	fmt.Printf("Audio stop successful\n")
	return nil
}

func testSystemFunctions(ctx context.Context, client pb.LinxOSDeviceServiceClient) error {
	// 测试系统信息
	infoReq := &pb.SystemInfoRequest{
		DeviceId: *deviceID,
	}

	infoResp, err := client.SystemInfo(ctx, infoReq)
	if err != nil {
		return fmt.Errorf("system info failed: %v", err)
	}

	if !infoResp.Status.Success {
		return fmt.Errorf("system info failed: %s", infoResp.Status.Message)
	}

	fmt.Printf("System info successful. CPU: %.1f%%, Memory: %.1f%%, Temp: %.1f°C\n", 
		infoResp.CpuUsage, infoResp.MemoryUsage, infoResp.Temperature)

	// 测试WiFi重连
	wifiReq := &pb.SystemWifiReconnectRequest{
		DeviceId: *deviceID,
		Ssid:     "TestNetwork",
		Password: "testpassword",
	}

	wifiResp, err := client.SystemWifiReconnect(ctx, wifiReq)
	if err != nil {
		return fmt.Errorf("wifi reconnect failed: %v", err)
	}

	if !wifiResp.Status.Success {
		return fmt.Errorf("wifi reconnect failed: %s", wifiResp.Status.Message)
	}

	fmt.Printf("WiFi reconnect successful. Status: %s\n", 
		wifiResp.ConnectionStatus)

	return nil
}

func testGenericToolCall(ctx context.Context, client pb.LinxOSDeviceServiceClient) error {
	req := &pb.ToolCallRequest{
		DeviceId:   *deviceID,
		ToolName:   "system_info",
		Parameters: `{"detailed": true}`,
		CallId:     "test_call_001",
	}

	resp, err := client.CallTool(ctx, req)
	if err != nil {
		return fmt.Errorf("tool call failed: %v", err)
	}

	if !resp.Status.Success {
		return fmt.Errorf("tool call failed: %s", resp.Status.Message)
	}

	fmt.Printf("Generic tool call successful. Call ID: %s\n", resp.CallId)
	fmt.Printf("Result: %s\n", resp.Result)
	return nil
}