package internal

import (
	"fmt"
	"log"
	"sync"
	"time"

	pb "golang-test-server/proto"
)

// DeviceManager 管理所有连接的设备
type DeviceManager struct {
	devices map[string]*Device
	mutex   sync.RWMutex
}

// Device 设备信息（更新为支持 LinxOS RPC 协议）
type Device struct {
	ID               string
	Name             string
	Type             string
	FirmwareVersion  string
	IPAddress        string
	Port             int
	Capabilities     map[string]string
	AvailableTools   []string
	Online           bool
	LastSeen         time.Time
	SessionID        string
	StatusInfo       map[string]string
}

// DeviceInfo 保持向后兼容的别名
type DeviceInfo = Device

// NewDeviceManager 创建新的设备管理器
func NewDeviceManager() *DeviceManager {
	dm := &DeviceManager{
		devices: make(map[string]*Device),
	}
	
	// 添加一些模拟设备用于演示
	dm.addMockDevices()
	
	return dm
}

// addMockDevices 添加模拟设备
func (dm *DeviceManager) addMockDevices() {
	mockDevices := []*Device{
		{
			ID:              "xiaozhi-001",
			Name:            "小智设备-客厅",
			Type:            "xiaozhi-esp32",
			FirmwareVersion: "1.0.0",
			IPAddress:       "192.168.1.100",
			Port:            8080,
			Capabilities: map[string]string{
				"voice":   "enabled",
				"display": "enabled",
				"light":   "enabled",
				"audio":   "enabled",
				"system":  "enabled",
			},
			AvailableTools: []string{
				"voice_speak", "voice_volume",
				"display_expression", "display_text", "display_brightness",
				"light_control", "light_mode",
				"audio_play", "audio_record", "audio_stop",
				"system_info", "system_restart", "system_wifi_reconnect",
			},
			Online:    true,
			LastSeen:  time.Now(),
			SessionID: "session_mock_001",
		},
		{
			ID:              "xiaozhi-002",
			Name:            "小智设备-卧室",
			Type:            "xiaozhi-esp32",
			FirmwareVersion: "1.0.0",
			IPAddress:       "192.168.1.101",
			Port:            8080,
			Capabilities: map[string]string{
				"voice":   "enabled",
				"display": "enabled",
				"light":   "enabled",
				"audio":   "enabled",
				"system":  "enabled",
			},
			AvailableTools: []string{
				"voice_speak", "voice_volume",
				"display_expression", "display_text", "display_brightness",
				"light_control", "light_mode",
				"audio_play", "audio_record", "audio_stop",
				"system_info", "system_restart", "system_wifi_reconnect",
			},
			Online:    true,
			LastSeen:  time.Now(),
			SessionID: "session_mock_002",
		},
	}
	
	for _, device := range mockDevices {
		dm.devices[device.ID] = device
	}
	
	log.Printf("Added %d mock devices", len(mockDevices))
}

// GetDevice 获取设备信息
func (dm *DeviceManager) GetDevice(deviceID string) (*Device, error) {
	dm.mutex.RLock()
	defer dm.mutex.RUnlock()
	
	device, exists := dm.devices[deviceID]
	if !exists {
		return nil, fmt.Errorf("device %s not found", deviceID)
	}
	
	return device, nil
}

// RegisterDevice 注册新设备
func (dm *DeviceManager) RegisterDevice(device *Device) {
	dm.mutex.Lock()
	defer dm.mutex.Unlock()
	
	dm.devices[device.ID] = device
	log.Printf("Device %s registered successfully", device.ID)
}

// ListDevices 获取所有设备列表
func (dm *DeviceManager) ListDevices() []*Device {
	dm.mutex.RLock()
	defer dm.mutex.RUnlock()
	
	devices := make([]*Device, 0, len(dm.devices))
	for _, device := range dm.devices {
		devices = append(devices, device)
	}
	
	return devices
}

// UpdateDeviceStatus 更新设备状态
func (dm *DeviceManager) UpdateDeviceStatus(deviceID string, online bool) {
	dm.mutex.Lock()
	defer dm.mutex.Unlock()
	
	if device, exists := dm.devices[deviceID]; exists {
		device.Online = online
		device.LastSeen = time.Now()
	}
}

// ToProtoDeviceInfo 转换为 protobuf 格式
func (d *Device) ToProtoDeviceInfo() *pb.DeviceInfo {
	return &pb.DeviceInfo{
		DeviceId:        d.ID,
		DeviceName:      d.Name,
		DeviceType:      d.Type,
		FirmwareVersion: d.FirmwareVersion,
		IpAddress:       d.IPAddress,
		Port:            int32(d.Port),
		Capabilities:    d.Capabilities,
		LastHeartbeat:   d.LastSeen.Unix(),
	}
}