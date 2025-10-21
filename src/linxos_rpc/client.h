/**
 * LinxOS RPC Client Library
 * 
 * 这个库提供了一个简单易用的接口，用于将设备端功能注册到远程gRPC服务器，
 * 并允许远程服务器调用设备的各种功能。
 * 
 * 主要特性：
 * - 自动连接管理和断线重连
 * - 简单的工具注册机制
 * - JSON格式的参数传递
 * - 状态监控和错误处理
 * - 心跳保持连接
 */

#ifndef LINXOS_RPC_CLIENT_H
#define LINXOS_RPC_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace linxos {
namespace rpc {

// 前向声明
class LinxOSRPCClientImpl;

/**
 * 连接状态枚举
 */
enum class ConnectionStatus {
    DISCONNECTED,   // 未连接
    CONNECTING,     // 连接中
    CONNECTED,      // 已连接
    ERROR          // 连接错误
};

/**
 * 设备信息结构
 */
struct DeviceInfo {
    std::string device_id;          // 设备ID
    std::string device_name;        // 设备名称
    std::string device_type;        // 设备类型
    std::string firmware_version;   // 固件版本
    std::string ip_address;         // IP地址
    int port;                       // 端口
    std::map<std::string, std::string> capabilities; // 设备能力
};

/**
 * 连接配置结构
 */
struct ConnectionConfig {
    std::string server_address;     // 服务器地址 (host:port)
    int heartbeat_interval_s;       // 心跳间隔(秒)
    int connection_timeout_s;       // 连接超时(秒)
    int max_retry_count;           // 最大重试次数
    int retry_interval_s;          // 重试间隔(秒)
    bool enable_ssl;               // 是否启用SSL
    std::string ssl_cert_path;     // SSL证书路径
};

/**
 * 工具函数类型定义
 * 参数：JSON格式的参数字符串
 * 返回：JSON格式的结果字符串
 */
using ToolFunction = std::function<std::string(const std::string& params)>;

/**
 * 状态回调函数类型定义
 * 参数1：连接状态
 * 参数2：状态消息
 */
using StatusCallback = std::function<void(ConnectionStatus status, const std::string& message)>;

/**
 * LinxOS RPC 客户端主类
 * 
 * 这个类提供了设备端与远程gRPC服务器通信的完整功能。
 * 使用方法：
 * 1. 创建设备信息和连接配置
 * 2. 创建LinxOSRPCClient实例
 * 3. 注册设备功能工具
 * 4. 连接到服务器并启动服务
 */
class LinxOSRPCClient {
public:
    /**
     * 构造函数
     * @param device_info 设备信息
     * @param config 连接配置
     */
    LinxOSRPCClient(const DeviceInfo& device_info, const ConnectionConfig& config);
    
    /**
     * 析构函数
     */
    ~LinxOSRPCClient();
    
    /**
     * 注册工具函数
     * @param tool_name 工具名称，如 "voice_speak", "display_expression"
     * @param function 工具处理函数
     * @param description 工具描述（可选）
     * @return 是否注册成功
     */
    bool AddTool(const std::string& tool_name, 
                 ToolFunction function, 
                 const std::string& description = "");
    
    /**
     * 移除工具函数
     * @param tool_name 工具名称
     * @return 是否移除成功
     */
    bool RemoveTool(const std::string& tool_name);
    
    /**
     * 连接到远程服务器
     * @return 是否连接成功
     */
    bool Connect();
    
    /**
     * 断开与远程服务器的连接
     */
    void Disconnect();
    
    /**
     * 启动RPC服务
     * @return 是否启动成功
     */
    bool Start();
    
    /**
     * 停止RPC服务
     */
    void Stop();
    
    /**
     * 获取当前连接状态
     * @return 连接状态
     */
    ConnectionStatus GetStatus() const;
    
    /**
     * 设置状态回调函数
     * @param callback 状态回调函数
     */
    void SetStatusCallback(StatusCallback callback);
    
    /**
     * 获取设备信息
     * @return 设备信息
     */
    const DeviceInfo& GetDeviceInfo() const;
    
    /**
     * 获取连接配置
     * @return 连接配置
     */
    const ConnectionConfig& GetConfig() const;
    
    /**
     * 获取已注册的工具列表
     * @return 工具名称列表
     */
    std::vector<std::string> GetRegisteredTools() const;
    
    /**
     * 检查是否已连接
     * @return 是否已连接
     */
    bool IsConnected() const;
    
    /**
     * 发送心跳
     * @return 是否发送成功
     */
    bool SendHeartbeat();

private:
    std::unique_ptr<LinxOSRPCClientImpl> impl_;
};

/**
 * 辅助函数：创建xiaozhi设备信息
 * @param device_id 设备ID
 * @param firmware_version 固件版本
 * @return 设备信息
 */
DeviceInfo CreateXiaozhiDeviceInfo(const std::string& device_id, 
                                   const std::string& firmware_version = "1.0.0");

/**
 * 辅助函数：创建默认连接配置
 * @param server_address 服务器地址
 * @return 连接配置
 */
ConnectionConfig CreateDefaultConfig(const std::string& server_address);

/**
 * 辅助函数：将连接状态转换为字符串
 * @param status 连接状态
 * @return 状态字符串
 */
std::string StatusToString(ConnectionStatus status);

} // namespace rpc
} // namespace linxos

#endif // LINXOS_RPC_CLIENT_H