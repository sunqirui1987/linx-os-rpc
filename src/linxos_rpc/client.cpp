/**
 * LinxOS RPC Client Library Implementation
 */

#include "client.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <json/json.h>

// 包含生成的protobuf文件
#include "device.grpc.pb.h"

namespace linxos {
namespace rpc {

/**
 * LinxOS RPC 客户端内部实现类
 */
class LinxOSRPCClientImpl {
public:
    LinxOSRPCClientImpl(const DeviceInfo& device_info, const ConnectionConfig& config)
        : device_info_(device_info)
        , config_(config)
        , status_(ConnectionStatus::DISCONNECTED)
        , running_(false)
        , session_id_("")
    {
    }
    
    ~LinxOSRPCClientImpl() {
        Stop();
        Disconnect();
    }
    
    bool AddTool(const std::string& tool_name, ToolFunction function, const std::string& description) {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        tools_[tool_name] = {function, description};
        std::cout << "[LinxOS RPC] 注册工具: " << tool_name << " - " << description << std::endl;
        return true;
    }
    
    bool RemoveTool(const std::string& tool_name) {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        auto it = tools_.find(tool_name);
        if (it != tools_.end()) {
            tools_.erase(it);
            std::cout << "[LinxOS RPC] 移除工具: " << tool_name << std::endl;
            return true;
        }
        return false;
    }
    
    bool Connect() {
        if (status_ == ConnectionStatus::CONNECTED) {
            return true;
        }
        
        SetStatus(ConnectionStatus::CONNECTING, "正在连接到服务器...");
        
        try {
            // 创建gRPC通道
            grpc::ChannelArguments args;
            args.SetMaxReceiveMessageSize(4 * 1024 * 1024); // 4MB
            args.SetMaxSendMessageSize(4 * 1024 * 1024);    // 4MB
            
            std::shared_ptr<grpc::Channel> channel;
            if (config_.enable_ssl) {
                auto creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
                channel = grpc::CreateCustomChannel(config_.server_address, creds, args);
            } else {
                channel = grpc::CreateCustomChannel(config_.server_address, 
                                                  grpc::InsecureChannelCredentials(), args);
            }
            
            // 创建stub
            stub_ = linxos_device::LinxOSDeviceService::NewStub(channel);
            
            // 等待连接建立
            auto deadline = std::chrono::system_clock::now() + 
                           std::chrono::seconds(config_.connection_timeout_s);
            
            if (!channel->WaitForConnected(deadline)) {
                SetStatus(ConnectionStatus::ERROR, "连接超时");
                return false;
            }
            
            // 注册设备
            if (!RegisterDevice()) {
                SetStatus(ConnectionStatus::ERROR, "设备注册失败");
                return false;
            }
            
            SetStatus(ConnectionStatus::CONNECTED, "连接成功");
            return true;
            
        } catch (const std::exception& e) {
            SetStatus(ConnectionStatus::ERROR, "连接异常: " + std::string(e.what()));
            return false;
        }
    }
    
    void Disconnect() {
        if (status_ == ConnectionStatus::DISCONNECTED) {
            return;
        }
        
        Stop();
        stub_.reset();
        session_id_.clear();
        SetStatus(ConnectionStatus::DISCONNECTED, "已断开连接");
    }
    
    bool Start() {
        if (!stub_ || status_ != ConnectionStatus::CONNECTED) {
            std::cerr << "[LinxOS RPC] 未连接到服务器，无法启动服务" << std::endl;
            return false;
        }
        
        if (running_) {
            return true;
        }
        
        running_ = true;
        
        // 启动心跳线程
        heartbeat_thread_ = std::thread(&LinxOSRPCClientImpl::HeartbeatLoop, this);
        
        std::cout << "[LinxOS RPC] RPC服务已启动" << std::endl;
        return true;
    }
    
    void Stop() {
        if (!running_) {
            return;
        }
        
        running_ = false;
        
        // 等待心跳线程结束
        if (heartbeat_thread_.joinable()) {
            heartbeat_thread_.join();
        }
        
        std::cout << "[LinxOS RPC] RPC服务已停止" << std::endl;
    }
    
    ConnectionStatus GetStatus() const {
        return status_;
    }
    
    void SetStatusCallback(StatusCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        status_callback_ = callback;
    }
    
    const DeviceInfo& GetDeviceInfo() const {
        return device_info_;
    }
    
    const ConnectionConfig& GetConfig() const {
        return config_;
    }
    
    std::vector<std::string> GetRegisteredTools() const {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        std::vector<std::string> tool_names;
        for (const auto& pair : tools_) {
            tool_names.push_back(pair.first);
        }
        return tool_names;
    }
    
    bool IsConnected() const {
        return status_ == ConnectionStatus::CONNECTED;
    }
    
    bool SendHeartbeat() {
        if (!stub_ || !IsConnected()) {
            return false;
        }
        
        try {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            
            linxos_device::HeartbeatRequest request;
            request.set_device_id(device_info_.device_id);
            request.set_session_id(session_id_);
            
            // 添加状态信息
            auto status_info = request.mutable_status_info();
            (*status_info)["cpu_usage"] = "25.5";
            (*status_info)["memory_usage"] = "60.2";
            (*status_info)["temperature"] = "45.8";
            
            linxos_device::HeartbeatResponse response;
            grpc::Status status = stub_->Heartbeat(&context, request, &response);
            
            if (status.ok() && response.status().success()) {
                return true;
            } else {
                std::cerr << "[LinxOS RPC] 心跳失败: " << response.status().message() << std::endl;
                return false;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[LinxOS RPC] 心跳异常: " << e.what() << std::endl;
            return false;
        }
    }
    
    // 处理工具调用
    std::string HandleToolCall(const std::string& tool_name, const std::string& params) {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        
        auto it = tools_.find(tool_name);
        if (it == tools_.end()) {
            Json::Value error_response;
            error_response["success"] = false;
            error_response["message"] = "未找到工具: " + tool_name;
            return error_response.toStyledString();
        }
        
        try {
            return it->second.function(params);
        } catch (const std::exception& e) {
            Json::Value error_response;
            error_response["success"] = false;
            error_response["message"] = "工具执行异常: " + std::string(e.what());
            return error_response.toStyledString();
        }
    }

private:
    struct ToolInfo {
        ToolFunction function;
        std::string description;
    };
    
    DeviceInfo device_info_;
    ConnectionConfig config_;
    std::atomic<ConnectionStatus> status_;
    std::atomic<bool> running_;
    std::string session_id_;
    
    std::unique_ptr<linxos_device::LinxOSDeviceService::Stub> stub_;
    
    std::map<std::string, ToolInfo> tools_;
    mutable std::mutex tools_mutex_;
    
    StatusCallback status_callback_;
    mutable std::mutex callback_mutex_;
    
    std::thread heartbeat_thread_;
    
    void SetStatus(ConnectionStatus status, const std::string& message) {
        status_ = status;
        
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (status_callback_) {
            status_callback_(status, message);
        }
        
        std::cout << "[LinxOS RPC] 状态变更: " << StatusToString(status) << " - " << message << std::endl;
    }
    
    bool RegisterDevice() {
        try {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));
            
            linxos_device::RegisterDeviceRequest request;
            
            // 设置设备信息
            auto device_info = request.mutable_device_info();
            device_info->set_device_id(device_info_.device_id);
            device_info->set_device_name(device_info_.device_name);
            device_info->set_device_type(device_info_.device_type);
            device_info->set_firmware_version(device_info_.firmware_version);
            device_info->set_ip_address(device_info_.ip_address);
            device_info->set_port(device_info_.port);
            
            // 设置设备能力
            auto capabilities = device_info->mutable_capabilities();
            for (const auto& cap : device_info_.capabilities) {
                (*capabilities)[cap.first] = cap.second;
            }
            
            // 设置可用工具
            std::lock_guard<std::mutex> lock(tools_mutex_);
            for (const auto& tool : tools_) {
                request.add_available_tools(tool.first);
            }
            
            linxos_device::RegisterDeviceResponse response;
            grpc::Status status = stub_->RegisterDevice(&context, request, &response);
            
            if (status.ok() && response.status().success()) {
                session_id_ = response.session_id();
                std::cout << "[LinxOS RPC] 设备注册成功，会话ID: " << session_id_ << std::endl;
                return true;
            } else {
                std::cerr << "[LinxOS RPC] 设备注册失败: " << response.status().message() << std::endl;
                return false;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[LinxOS RPC] 设备注册异常: " << e.what() << std::endl;
            return false;
        }
    }
    
    void HeartbeatLoop() {
        std::cout << "[LinxOS RPC] 心跳线程已启动" << std::endl;
        
        while (running_) {
            if (IsConnected()) {
                if (!SendHeartbeat()) {
                    SetStatus(ConnectionStatus::ERROR, "心跳失败");
                    // 尝试重新连接
                    std::this_thread::sleep_for(std::chrono::seconds(config_.retry_interval_s));
                    if (running_) {
                        Connect();
                    }
                }
            }
            
            // 等待下一次心跳
            for (int i = 0; i < config_.heartbeat_interval_s && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        std::cout << "[LinxOS RPC] 心跳线程已退出" << std::endl;
    }
};

// LinxOSRPCClient 公共接口实现

LinxOSRPCClient::LinxOSRPCClient(const DeviceInfo& device_info, const ConnectionConfig& config)
    : impl_(std::make_unique<LinxOSRPCClientImpl>(device_info, config))
{
}

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
    info.device_type = "smart_robot";
    info.firmware_version = firmware_version;
    info.ip_address = "192.168.1.100";  // 默认IP，实际使用时应该获取真实IP
    info.port = 8080;
    
    // 设置设备能力
    info.capabilities["voice"] = "true";
    info.capabilities["display"] = "true";
    info.capabilities["light"] = "true";
    info.capabilities["audio"] = "true";
    info.capabilities["system"] = "true";
    
    return info;
}

ConnectionConfig CreateDefaultConfig(const std::string& server_address) {
    ConnectionConfig config;
    config.server_address = server_address;
    config.heartbeat_interval_s = 30;
    config.connection_timeout_s = 10;
    config.max_retry_count = 5;
    config.retry_interval_s = 5;
    config.enable_ssl = false;
    config.ssl_cert_path = "";
    
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