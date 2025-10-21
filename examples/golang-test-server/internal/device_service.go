package internal

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"time"

	pb "golang-test-server/proto"
)

// LinxOSDeviceServer 实现 LinxOS 设备 gRPC 服务
type LinxOSDeviceServer struct {
	pb.UnimplementedLinxOSDeviceServiceServer
	deviceManager *DeviceManager
}

// NewLinxOSDeviceServer 创建新的 LinxOS 设备服务
func NewLinxOSDeviceServer() *LinxOSDeviceServer {
	return &LinxOSDeviceServer{
		deviceManager: NewDeviceManager(),
	}
}

// 辅助函数：创建成功响应状态
func createSuccessStatus(message string) *pb.ResponseStatus {
	return &pb.ResponseStatus{
		Success:   true,
		Message:   message,
		ErrorCode: 0,
	}
}

// 辅助函数：创建错误响应状态
func createErrorStatus(message string, errorCode int32) *pb.ResponseStatus {
	return &pb.ResponseStatus{
		Success:   false,
		Message:   message,
		ErrorCode: errorCode,
	}
}

// 辅助函数：检查设备状态
func (s *LinxOSDeviceServer) checkDevice(deviceId string) (*Device, *pb.ResponseStatus) {
	device, err := s.deviceManager.GetDevice(deviceId)
	if err != nil {
		return nil, createErrorStatus(fmt.Sprintf("Device not found: %s", deviceId), 404)
	}
	
	if !device.Online {
		return nil, createErrorStatus(fmt.Sprintf("Device %s is offline", deviceId), 503)
	}
	
	return device, nil
}

// RegisterDevice 设备注册
func (s *LinxOSDeviceServer) RegisterDevice(ctx context.Context, req *pb.RegisterDeviceRequest) (*pb.RegisterDeviceResponse, error) {
	log.Printf("RegisterDevice called for device %s (%s)", req.DeviceInfo.DeviceId, req.DeviceInfo.DeviceName)
	
	// 注册设备
	device := &Device{
		ID:               req.DeviceInfo.DeviceId,
		Name:             req.DeviceInfo.DeviceName,
		Type:             req.DeviceInfo.DeviceType,
		FirmwareVersion:  req.DeviceInfo.FirmwareVersion,
		IPAddress:        req.DeviceInfo.IpAddress,
		Port:             int(req.DeviceInfo.Port),
		Capabilities:     req.DeviceInfo.Capabilities,
		AvailableTools:   req.AvailableTools,
		Online:           true,
		LastSeen:         time.Now(),
		SessionID:        fmt.Sprintf("session_%d", time.Now().Unix()),
	}
	
	s.deviceManager.RegisterDevice(device)
	
	log.Printf("Device %s registered successfully with %d tools", device.ID, len(req.AvailableTools))
	for _, tool := range req.AvailableTools {
		log.Printf("  - %s", tool)
	}
	
	return &pb.RegisterDeviceResponse{
		Status:            createSuccessStatus("Device registered successfully"),
		SessionId:         device.SessionID,
		HeartbeatInterval: 30, // 30秒心跳间隔
	}, nil
}

// Heartbeat 心跳保持
func (s *LinxOSDeviceServer) Heartbeat(ctx context.Context, req *pb.HeartbeatRequest) (*pb.HeartbeatResponse, error) {
	log.Printf("Heartbeat from device %s (session: %s)", req.DeviceId, req.SessionId)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.HeartbeatResponse{
			Status:    status,
			KeepAlive: false,
		}, nil
	}
	
	// 验证会话ID
	if device.SessionID != req.SessionId {
		return &pb.HeartbeatResponse{
			Status:    createErrorStatus("Invalid session ID", 401),
			KeepAlive: false,
		}, nil
	}
	
	// 更新设备状态
	device.LastSeen = time.Now()
	if req.StatusInfo != nil {
		device.StatusInfo = req.StatusInfo
	}
	
	return &pb.HeartbeatResponse{
		Status:    createSuccessStatus("Heartbeat received"),
		KeepAlive: true,
	}, nil
}

// VoiceSpeak 语音播放
func (s *LinxOSDeviceServer) VoiceSpeak(ctx context.Context, req *pb.VoiceSpeakRequest) (*pb.VoiceSpeakResponse, error) {
	log.Printf("VoiceSpeak called for device %s: text='%s', speed=%.2f, volume=%d", 
		req.DeviceId, req.Text, req.Speed, req.Volume)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.VoiceSpeakResponse{Status: status}, nil
	}
	
	// 调用设备的 voice_speak 工具
	params := map[string]interface{}{
		"text":   req.Text,
		"speed":  req.Speed,
		"volume": req.Volume,
	}
	if req.VoiceType != "" {
		params["voice_type"] = req.VoiceType
	}
	
	result, err := s.callDeviceTool(device, "voice_speak", params)
	if err != nil {
		return &pb.VoiceSpeakResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.VoiceSpeakResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	duration, _ := toolResult["duration"].(float64)
	
	if !success {
		return &pb.VoiceSpeakResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.VoiceSpeakResponse{
		Status:   createSuccessStatus(message),
		Duration: float32(duration),
		AudioId:  fmt.Sprintf("audio_%d", time.Now().Unix()),
	}, nil
}

// VoiceVolume 音量控制
func (s *LinxOSDeviceServer) VoiceVolume(ctx context.Context, req *pb.VoiceVolumeRequest) (*pb.VoiceVolumeResponse, error) {
	log.Printf("VoiceVolume called for device %s: volume=%d", req.DeviceId, req.Volume)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.VoiceVolumeResponse{Status: status}, nil
	}
	
	// 调用设备的 voice_volume 工具
	params := map[string]interface{}{}
	if req.Volume >= 0 {
		params["volume"] = req.Volume
	}
	
	result, err := s.callDeviceTool(device, "voice_volume", params)
	if err != nil {
		return &pb.VoiceVolumeResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.VoiceVolumeResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	volume, _ := toolResult["volume"].(float64)
	
	if !success {
		return &pb.VoiceVolumeResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.VoiceVolumeResponse{
		Status:        createSuccessStatus(message),
		CurrentVolume: int32(volume),
	}, nil
}

// DisplayExpression 表情显示
func (s *LinxOSDeviceServer) DisplayExpression(ctx context.Context, req *pb.DisplayExpressionRequest) (*pb.DisplayExpressionResponse, error) {
	log.Printf("DisplayExpression called for device %s: expression='%s', duration=%d", 
		req.DeviceId, req.Expression, req.Duration)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.DisplayExpressionResponse{Status: status}, nil
	}
	
	// 调用设备的 display_expression 工具
	params := map[string]interface{}{
		"expression": req.Expression,
		"duration":   req.Duration,
	}
	if req.Intensity > 0 {
		params["intensity"] = req.Intensity
	}
	
	result, err := s.callDeviceTool(device, "display_expression", params)
	if err != nil {
		return &pb.DisplayExpressionResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.DisplayExpressionResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.DisplayExpressionResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.DisplayExpressionResponse{
		Status: createSuccessStatus(message),
	}, nil
}

// DisplayText 文本显示
func (s *LinxOSDeviceServer) DisplayText(ctx context.Context, req *pb.DisplayTextRequest) (*pb.DisplayTextResponse, error) {
	log.Printf("DisplayText called for device %s: text='%s', duration=%d", 
		req.DeviceId, req.Text, req.Duration)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.DisplayTextResponse{Status: status}, nil
	}
	
	// 调用设备的 display_text 工具
	params := map[string]interface{}{
		"text":     req.Text,
		"duration": req.Duration,
	}
	if req.FontSize != "" {
		params["font_size"] = req.FontSize
	}
	if req.Color != "" {
		params["color"] = req.Color
	}
	
	result, err := s.callDeviceTool(device, "display_text", params)
	if err != nil {
		return &pb.DisplayTextResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.DisplayTextResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.DisplayTextResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.DisplayTextResponse{
		Status: createSuccessStatus(message),
	}, nil
}

// DisplayBrightness 显示亮度控制
func (s *LinxOSDeviceServer) DisplayBrightness(ctx context.Context, req *pb.DisplayBrightnessRequest) (*pb.DisplayBrightnessResponse, error) {
	log.Printf("DisplayBrightness called for device %s: brightness=%d", req.DeviceId, req.Brightness)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.DisplayBrightnessResponse{Status: status}, nil
	}
	
	// 调用设备的 display_brightness 工具
	params := map[string]interface{}{}
	if req.Brightness >= 0 {
		params["brightness"] = req.Brightness
	}
	
	result, err := s.callDeviceTool(device, "display_brightness", params)
	if err != nil {
		return &pb.DisplayBrightnessResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.DisplayBrightnessResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	brightness, _ := toolResult["brightness"].(float64)
	
	if !success {
		return &pb.DisplayBrightnessResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.DisplayBrightnessResponse{
		Status:            createSuccessStatus(message),
		CurrentBrightness: int32(brightness),
	}, nil
}

// LightControl RGB 灯光控制
func (s *LinxOSDeviceServer) LightControl(ctx context.Context, req *pb.LightControlRequest) (*pb.LightControlResponse, error) {
	log.Printf("LightControl called for device %s: RGB(%d,%d,%d), brightness=%d", 
		req.DeviceId, req.Red, req.Green, req.Blue, req.Brightness)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.LightControlResponse{Status: status}, nil
	}
	
	// 调用设备的 light_control 工具
	params := map[string]interface{}{
		"red":        req.Red,
		"green":      req.Green,
		"blue":       req.Blue,
		"brightness": req.Brightness,
	}
	if req.Duration > 0 {
		params["duration"] = req.Duration
	}
	
	result, err := s.callDeviceTool(device, "light_control", params)
	if err != nil {
		return &pb.LightControlResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.LightControlResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.LightControlResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.LightControlResponse{
		Status: createSuccessStatus(message),
	}, nil
}

// LightMode 灯光模式设置
func (s *LinxOSDeviceServer) LightMode(ctx context.Context, req *pb.LightModeRequest) (*pb.LightModeResponse, error) {
	log.Printf("LightMode called for device %s: mode='%s', speed=%d", 
		req.DeviceId, req.Mode, req.Speed)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.LightModeResponse{Status: status}, nil
	}
	
	// 调用设备的 light_mode 工具
	params := map[string]interface{}{
		"mode":  req.Mode,
		"speed": req.Speed,
	}
	if len(req.Colors) > 0 {
		params["colors"] = req.Colors
	}
	
	result, err := s.callDeviceTool(device, "light_mode", params)
	if err != nil {
		return &pb.LightModeResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.LightModeResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.LightModeResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.LightModeResponse{
		Status: createSuccessStatus(message),
	}, nil
}

// AudioPlay 音频播放
func (s *LinxOSDeviceServer) AudioPlay(ctx context.Context, req *pb.AudioPlayRequest) (*pb.AudioPlayResponse, error) {
	log.Printf("AudioPlay called for device %s: file='%s', volume=%.2f, loop=%t", 
		req.DeviceId, req.FilePath, req.Volume, req.Loop)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.AudioPlayResponse{Status: status}, nil
	}
	
	// 调用设备的 audio_play 工具
	params := map[string]interface{}{
		"file_path": req.FilePath,
		"volume":    req.Volume,
		"loop":      req.Loop,
	}
	
	result, err := s.callDeviceTool(device, "audio_play", params)
	if err != nil {
		return &pb.AudioPlayResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.AudioPlayResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	audioId, _ := toolResult["audio_id"].(string)
	duration, _ := toolResult["duration"].(float64)
	
	if !success {
		return &pb.AudioPlayResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.AudioPlayResponse{
		Status:   createSuccessStatus(message),
		AudioId:  audioId,
		Duration: float32(duration),
	}, nil
}

// AudioRecord 音频录制
func (s *LinxOSDeviceServer) AudioRecord(ctx context.Context, req *pb.AudioRecordRequest) (*pb.AudioRecordResponse, error) {
	log.Printf("AudioRecord called for device %s: duration=%d, format='%s'", 
		req.DeviceId, req.Duration, req.Format)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.AudioRecordResponse{Status: status}, nil
	}
	
	// 调用设备的 audio_record 工具
	params := map[string]interface{}{
		"duration": req.Duration,
	}
	if req.Format != "" {
		params["format"] = req.Format
	}
	if req.SampleRate > 0 {
		params["sample_rate"] = req.SampleRate
	}
	
	result, err := s.callDeviceTool(device, "audio_record", params)
	if err != nil {
		return &pb.AudioRecordResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.AudioRecordResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	recordId, _ := toolResult["record_id"].(string)
	filePath, _ := toolResult["file_path"].(string)
	
	if !success {
		return &pb.AudioRecordResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.AudioRecordResponse{
		Status:   createSuccessStatus(message),
		RecordId: recordId,
		FilePath: filePath,
	}, nil
}

// AudioStop 音频停止
func (s *LinxOSDeviceServer) AudioStop(ctx context.Context, req *pb.AudioStopRequest) (*pb.AudioStopResponse, error) {
	log.Printf("AudioStop called for device %s: audio_id='%s'", req.DeviceId, req.AudioId)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.AudioStopResponse{Status: status}, nil
	}
	
	// 调用设备的 audio_stop 工具
	params := map[string]interface{}{
		"audio_id": req.AudioId,
	}
	
	result, err := s.callDeviceTool(device, "audio_stop", params)
	if err != nil {
		return &pb.AudioStopResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.AudioStopResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.AudioStopResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.AudioStopResponse{
		Status: createSuccessStatus(message),
	}, nil
}

// SystemInfo 系统信息
func (s *LinxOSDeviceServer) SystemInfo(ctx context.Context, req *pb.SystemInfoRequest) (*pb.SystemInfoResponse, error) {
	log.Printf("SystemInfo called for device %s", req.DeviceId)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.SystemInfoResponse{Status: status}, nil
	}
	
	// 调用设备的 system_info 工具
	params := map[string]interface{}{}
	
	result, err := s.callDeviceTool(device, "system_info", params)
	if err != nil {
		return &pb.SystemInfoResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.SystemInfoResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.SystemInfoResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	// 解析系统状态
	systemStatus, _ := toolResult["system_status"].(map[string]interface{})
	cpuUsage, _ := systemStatus["cpu_usage"].(float64)
	memoryUsage, _ := systemStatus["memory_usage"].(float64)
	temperature, _ := systemStatus["temperature"].(float64)
	uptime, _ := systemStatus["uptime"].(float64)
	wifiStatus, _ := systemStatus["wifi_status"].(string)
	freeHeap, _ := systemStatus["free_heap"].(float64)
	
	return &pb.SystemInfoResponse{
		Status:       createSuccessStatus(message),
		CpuUsage:     float32(cpuUsage),
		MemoryUsage:  float32(memoryUsage),
		Temperature:  float32(temperature),
		Uptime:       int64(uptime),
		WifiStatus:   wifiStatus,
		FreeHeap:     int64(freeHeap),
		AdditionalInfo: make(map[string]string), // 可以添加额外信息
	}, nil
}

// SystemRestart 系统重启
func (s *LinxOSDeviceServer) SystemRestart(ctx context.Context, req *pb.SystemRestartRequest) (*pb.SystemRestartResponse, error) {
	log.Printf("SystemRestart called for device %s: delay=%d, reason='%s'", 
		req.DeviceId, req.Delay, req.Reason)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.SystemRestartResponse{Status: status}, nil
	}
	
	// 调用设备的 system_restart 工具
	params := map[string]interface{}{
		"delay": req.Delay,
	}
	if req.Reason != "" {
		params["reason"] = req.Reason
	}
	
	result, err := s.callDeviceTool(device, "system_restart", params)
	if err != nil {
		return &pb.SystemRestartResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.SystemRestartResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	
	if !success {
		return &pb.SystemRestartResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.SystemRestartResponse{
		Status:      createSuccessStatus(message),
		RestartTime: time.Now().Add(time.Duration(req.Delay) * time.Second).Unix(),
	}, nil
}

// SystemWifiReconnect WiFi重连
func (s *LinxOSDeviceServer) SystemWifiReconnect(ctx context.Context, req *pb.SystemWifiReconnectRequest) (*pb.SystemWifiReconnectResponse, error) {
	log.Printf("SystemWifiReconnect called for device %s: ssid='%s'", req.DeviceId, req.Ssid)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.SystemWifiReconnectResponse{Status: status}, nil
	}
	
	// 调用设备的 system_wifi_reconnect 工具
	params := map[string]interface{}{}
	if req.Ssid != "" {
		params["ssid"] = req.Ssid
	}
	if req.Password != "" {
		params["password"] = req.Password
	}
	
	result, err := s.callDeviceTool(device, "system_wifi_reconnect", params)
	if err != nil {
		return &pb.SystemWifiReconnectResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
		}, nil
	}
	
	// 解析结果
	var toolResult map[string]interface{}
	if err := json.Unmarshal([]byte(result), &toolResult); err != nil {
		return &pb.SystemWifiReconnectResponse{
			Status: createErrorStatus("Failed to parse tool result", 500),
		}, nil
	}
	
	success, _ := toolResult["success"].(bool)
	message, _ := toolResult["message"].(string)
	connectionStatus, _ := toolResult["connection_status"].(string)
	
	if !success {
		return &pb.SystemWifiReconnectResponse{
			Status: createErrorStatus(message, 500),
		}, nil
	}
	
	return &pb.SystemWifiReconnectResponse{
		Status:           createSuccessStatus(message),
		ConnectionStatus: connectionStatus,
	}, nil
}

// CallTool 通用工具调用
func (s *LinxOSDeviceServer) CallTool(ctx context.Context, req *pb.ToolCallRequest) (*pb.ToolCallResponse, error) {
	log.Printf("CallTool called for device %s: tool='%s', call_id='%s'", 
		req.DeviceId, req.ToolName, req.CallId)
	
	device, status := s.checkDevice(req.DeviceId)
	if status != nil {
		return &pb.ToolCallResponse{
			Status: status,
			CallId: req.CallId,
		}, nil
	}
	
	// 解析参数
	var params map[string]interface{}
	if req.Parameters != "" {
		if err := json.Unmarshal([]byte(req.Parameters), &params); err != nil {
			return &pb.ToolCallResponse{
				Status: createErrorStatus("Invalid parameters JSON", 400),
				CallId: req.CallId,
			}, nil
		}
	}
	
	// 调用设备工具
	result, err := s.callDeviceTool(device, req.ToolName, params)
	if err != nil {
		return &pb.ToolCallResponse{
			Status: createErrorStatus(fmt.Sprintf("Tool call failed: %v", err), 500),
			CallId: req.CallId,
		}, nil
	}
	
	return &pb.ToolCallResponse{
		Status: createSuccessStatus("Tool called successfully"),
		Result: result,
		CallId: req.CallId,
	}, nil
}

// callDeviceTool 调用设备工具的内部方法
func (s *LinxOSDeviceServer) callDeviceTool(device *Device, toolName string, params map[string]interface{}) (string, error) {
	// 检查工具是否可用
	toolAvailable := false
	for _, tool := range device.AvailableTools {
		if tool == toolName {
			toolAvailable = true
			break
		}
	}
	
	if !toolAvailable {
		return "", fmt.Errorf("tool '%s' not available on device %s", toolName, device.ID)
	}
	
	// 模拟调用设备工具
	// 在实际实现中，这里会通过网络调用设备的具体工具接口
	log.Printf("Calling tool '%s' on device %s (%s:%d)", toolName, device.ID, device.IPAddress, device.Port)
	
	// 模拟处理时间
	time.Sleep(100 * time.Millisecond)
	
	// 模拟工具调用结果
	result := map[string]interface{}{
		"success": true,
		"message": fmt.Sprintf("Tool '%s' executed successfully", toolName),
	}
	
	// 根据不同工具添加特定的返回数据
	switch toolName {
	case "voice_speak":
		result["duration"] = 2.5
	case "voice_volume":
		if volume, ok := params["volume"]; ok {
			result["volume"] = volume
		} else {
			result["volume"] = 80 // 默认音量
		}
	case "system_info":
		result["system_status"] = map[string]interface{}{
			"cpu_usage":    25.5,
			"memory_usage": 60.2,
			"temperature":  45.8,
			"uptime":       3600,
			"wifi_status":  "connected",
			"free_heap":    102400,
		}
	case "audio_play":
		result["audio_id"] = fmt.Sprintf("audio_%d", time.Now().Unix())
		result["duration"] = 30.0
	case "audio_record":
		result["record_id"] = fmt.Sprintf("record_%d", time.Now().Unix())
		result["file_path"] = "/tmp/recording.wav"
	}
	
	resultBytes, _ := json.Marshal(result)
	return string(resultBytes), nil
}