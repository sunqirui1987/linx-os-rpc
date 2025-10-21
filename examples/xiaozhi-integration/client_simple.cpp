/**
 * LinxOS RPC Client Library - 简化版本（不依赖gRPC）
 * 
 * 这个简化版本用于演示目的，不包含实际的gRPC功能
 */

#include "linxos_rpc/client.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace linxos {
namespace rpc {

class LinxOSRPCClientImpl {
public:
    LinxOSRPCClientImpl(const DeviceInfo& device_info, const ConnectionConfig& config) 
        : device_info_(device_info), config_(config) {
        std::cout << "[RPC Client] 初始化设备: " << device_info.device_name 
                  << " (ID: " << device_info.device_id << ")" << std::endl;
    }
    
    bool Connect() {
        std::cout << "[RPC Client] 连接到服务器: " << config_.server_address << std::endl;
        connected_ = true;
        status_ = ConnectionStatus::CONNECTED;
        if (status_callback_) {
            status_callback_(status_, "连接成功");
        }
        return true;
    }
    
    void Disconnect() {
        std::cout << "[RPC Client] 断开连接" << std::endl;
        connected_ = false;
        status_ = ConnectionStatus::DISCONNECTED;
        if (status_callback_) {
            status_callback_(status_, "连接断开");
        }
    }
    
    bool IsConnected() const {
        return connected_;
    }
    
    bool AddTool(const std::string& tool_name, ToolFunction function, const std::string& description) {
        tools_[tool_name] = {description, function};
        std::cout << "[RPC Client] 注册工具: " << tool_name << " - " << description << std::endl;
        return true;
    }
    
    bool RemoveTool(const std::string& tool_name) {
        auto it = tools_.find(tool_name);
        if (it != tools_.end()) {
            tools_.erase(it);
            std::cout << "[RPC Client] 移除工具: " << tool_name << std::endl;
            return true;
        }
        return false;
    }
    
    bool Start() {
        std::cout << "[RPC Client] 启动服务循环" << std::endl;
        service_running_ = true;
        
        // 启动服务线程
        service_thread_ = std::thread([this]() {
            while (service_running_) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                // 模拟接收到工具调用请求
                static int call_count = 0;
                if (call_count < 3) {
                    call_count++;
                    SimulateToolCall();
                }
            }
        });
        
        return true;
    }
    
    void Stop() {
        std::cout << "[RPC Client] 停止服务" << std::endl;
        service_running_ = false;
        if (service_thread_.joinable()) {
            service_thread_.join();
        }
    }
    
    ConnectionStatus GetStatus() const {
        return status_;
    }
    
    void SetStatusCallback(StatusCallback callback) {
        status_callback_ = callback;
        std::cout << "[RPC Client] 设置状态回调" << std::endl;
    }
    
    const DeviceInfo& GetDeviceInfo() const {
        return device_info_;
    }
    
    const ConnectionConfig& GetConfig() const {
        return config_;
    }
    
    std::vector<std::string> GetRegisteredTools() const {
        std::vector<std::string> tool_names;
        for (const auto& tool : tools_) {
            tool_names.push_back(tool.first);
        }
        return tool_names;
    }
    
    bool SendHeartbeat() {
        if (!connected_) return false;
        std::cout << "[RPC Client] 发送心跳包" << std::endl;
        return true;
    }

private:
    void SimulateToolCall() {
        if (tools_.empty()) return;
        
        // 随机选择一个工具进行模拟调用
        auto it = tools_.begin();
        std::advance(it, rand() % tools_.size());
        
        std::string tool_name = it->first;
        ToolFunction function = it->second.function;
        
        std::cout << "[RPC Client] 模拟调用工具: " << tool_name << std::endl;
        
        // 模拟参数
        std::string params = R"({"test": "simulation"})";
        std::string result = function(params);
        
        std::cout << "[RPC Client] 工具调用结果: " << result << std::endl;
    }

private:
    struct ToolInfo {
        std::string description;
        ToolFunction function;
    };
    
    DeviceInfo device_info_;
    ConnectionConfig config_;
    std::map<std::string, ToolInfo> tools_;
    StatusCallback status_callback_;
    ConnectionStatus status_{ConnectionStatus::DISCONNECTED};
    std::atomic<bool> connected_{false};
    std::atomic<bool> service_running_{false};
    std::thread service_thread_;
};

// LinxOSRPCClient 实现
LinxOSRPCClient::LinxOSRPCClient(const DeviceInfo& device_info, const ConnectionConfig& config) 
    : impl_(std::make_unique<LinxOSRPCClientImpl>(device_info, config)) {}

LinxOSRPCClient::~LinxOSRPCClient() = default;

bool LinxOSRPCClient::AddTool(const std::string& tool_name, ToolFunction function, const std::string& description) {
    return impl_->AddTool(tool_name, function, description);
}

bool LinxOSRPCClient::RemoveTool(const std::string& tool_name) {
    return impl_->RemoveTool(tool_name);
}

bool LinxOSRPCClient::Connect() {
    return impl_->Connect();
}

void LinxOSRPCClient::Disconnect() {
    impl_->Disconnect();
}

bool LinxOSRPCClient::Start() {
    return impl_->Start();
}

void LinxOSRPCClient::Stop() {
    impl_->Stop();
}

ConnectionStatus LinxOSRPCClient::GetStatus() const {
    return impl_->GetStatus();
}

void LinxOSRPCClient::SetStatusCallback(StatusCallback callback) {
    impl_->SetStatusCallback(callback);
}

const DeviceInfo& LinxOSRPCClient::GetDeviceInfo() const {
    return impl_->GetDeviceInfo();
}

const ConnectionConfig& LinxOSRPCClient::GetConfig() const {
    return impl_->GetConfig();
}

std::vector<std::string> LinxOSRPCClient::GetRegisteredTools() const {
    return impl_->GetRegisteredTools();
}

bool LinxOSRPCClient::IsConnected() const {
    return impl_->IsConnected();
}

bool LinxOSRPCClient::SendHeartbeat() {
    return impl_->SendHeartbeat();
}

// 辅助函数实现
DeviceInfo CreateXiaozhiDeviceInfo(const std::string& device_id, const std::string& firmware_version) {
    DeviceInfo info;
    info.device_id = device_id;
    info.device_name = "xiaozhi-esp32";
    info.device_type = "smart_speaker";
    info.firmware_version = firmware_version;
    info.ip_address = "192.168.1.100";
    info.port = 8080;
    info.capabilities["voice"] = "true";
    info.capabilities["display"] = "true";
    info.capabilities["light"] = "true";
    info.capabilities["audio"] = "true";
    return info;
}

ConnectionConfig CreateDefaultConfig(const std::string& server_address) {
    ConnectionConfig config;
    config.server_address = server_address;
    config.heartbeat_interval_s = 30;
    config.connection_timeout_s = 10;
    config.max_retry_count = 3;
    config.retry_interval_s = 5;
    config.enable_ssl = false;
    return config;
}

std::string StatusToString(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::DISCONNECTED: return "DISCONNECTED";
        case ConnectionStatus::CONNECTING: return "CONNECTING";
        case ConnectionStatus::CONNECTED: return "CONNECTED";
        case ConnectionStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace rpc
} // namespace linxos