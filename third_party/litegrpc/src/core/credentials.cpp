/**
 * @file credentials.cpp
 * @brief LiteGRPC 凭证和通道参数实现文件
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件实现了 LiteGRPC 的凭证管理和通道参数配置功能。
 * 提供了与标准 gRPC 完全兼容的凭证和通道参数接口实现。
 * 
 * 实现功能：
 * - ChannelArguments 类：管理通道配置参数
 * - 预定义的标准 gRPC 参数常量
 * - 参数的设置和获取方法（整数、字符串、指针类型）
 * - 凭证工厂函数：创建不安全和 SSL 凭证
 * - 与标准 gRPC 参数名称完全兼容
 */

#include "litegrpc/credentials.h"

namespace litegrpc {

// ChannelArguments 实现

/**
 * @brief 标准 gRPC 通道参数常量定义
 * 
 * 这些常量与标准 gRPC 的参数名称完全一致，确保兼容性。
 * 用于配置连接保活、HTTP/2 设置和连接管理等功能。
 */
const std::string ChannelArguments::GRPC_ARG_KEEPALIVE_TIME_MS = "grpc.keepalive_time_ms";                                           ///< 保活时间间隔（毫秒）
const std::string ChannelArguments::GRPC_ARG_KEEPALIVE_TIMEOUT_MS = "grpc.keepalive_timeout_ms";                                   ///< 保活超时时间（毫秒）
const std::string ChannelArguments::GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS = "grpc.keepalive_permit_without_calls";               ///< 是否允许无调用时发送保活
const std::string ChannelArguments::GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA = "grpc.http2.max_pings_without_data";                   ///< HTTP/2 无数据时最大 ping 数量
const std::string ChannelArguments::GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS = "grpc.http2.min_sent_ping_interval_without_data_ms";     ///< HTTP/2 无数据时最小发送 ping 间隔（毫秒）
const std::string ChannelArguments::GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS = "grpc.http2.min_recv_ping_interval_without_data_ms";     ///< HTTP/2 无数据时最小接收 ping 间隔（毫秒）
const std::string ChannelArguments::GRPC_ARG_MAX_CONNECTION_IDLE_MS = "grpc.max_connection_idle_ms";                               ///< 最大连接空闲时间（毫秒）
const std::string ChannelArguments::GRPC_ARG_MAX_CONNECTION_AGE_MS = "grpc.max_connection_age_ms";                                 ///< 最大连接存活时间（毫秒）
const std::string ChannelArguments::GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS = "grpc.max_connection_age_grace_ms";                     ///< 连接存活宽限时间（毫秒）

/**
 * @brief 设置整数类型参数
 * @param key 参数键名
 * @param value 参数值
 * 
 * 设置指定键的整数类型参数值。如果键已存在，则覆盖原有值。
 */
void ChannelArguments::SetInt(const std::string& key, int value) {
    int_args_[key] = value;
}

/**
 * @brief 设置字符串类型参数
 * @param key 参数键名
 * @param value 参数值
 * 
 * 设置指定键的字符串类型参数值。如果键已存在，则覆盖原有值。
 */
void ChannelArguments::SetString(const std::string& key, const std::string& value) {
    string_args_[key] = value;
}

/**
 * @brief 设置指针类型参数
 * @param key 参数键名
 * @param value 参数值（指针）
 * 
 * 设置指定键的指针类型参数值。如果键已存在，则覆盖原有值。
 * 注意：调用者负责确保指针的生命周期管理。
 */
void ChannelArguments::SetPointer(const std::string& key, void* value) {
    pointer_args_[key] = value;
}

/**
 * @brief 获取整数类型参数
 * @param key 参数键名
 * @param value 用于存储参数值的指针
 * @return 如果找到参数返回 true，否则返回 false
 * 
 * 查找并获取指定键的整数类型参数值。
 */
bool ChannelArguments::GetInt(const std::string& key, int* value) const {
    auto it = int_args_.find(key);
    if (it != int_args_.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

/**
 * @brief 获取字符串类型参数
 * @param key 参数键名
 * @param value 用于存储参数值的指针
 * @return 如果找到参数返回 true，否则返回 false
 * 
 * 查找并获取指定键的字符串类型参数值。
 */
bool ChannelArguments::GetString(const std::string& key, std::string* value) const {
    auto it = string_args_.find(key);
    if (it != string_args_.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

/**
 * @brief 获取指针类型参数
 * @param key 参数键名
 * @param value 用于存储参数值的指针的指针
 * @return 如果找到参数返回 true，否则返回 false
 * 
 * 查找并获取指定键的指针类型参数值。
 */
bool ChannelArguments::GetPointer(const std::string& key, void** value) const {
    auto it = pointer_args_.find(key);
    if (it != pointer_args_.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

// 凭证工厂函数

/**
 * @brief 创建不安全的通道凭证
 * @return 不安全通道凭证的共享指针
 * 
 * 创建一个不使用 TLS 加密的通道凭证。
 * 适用于开发环境或内部网络通信，不建议在生产环境中使用。
 */
std::shared_ptr<ChannelCredentials> InsecureChannelCredentials() {
    return std::make_shared<InsecureChannelCredentialsImpl>();
}

/**
 * @brief 创建 SSL/TLS 通道凭证
 * @param options SSL 凭证配置选项
 * @return SSL 通道凭证的共享指针
 * 
 * 创建一个使用 SSL/TLS 加密的通道凭证。
 * 适用于生产环境，提供安全的网络通信。
 */
std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options) {
    return std::make_shared<SslChannelCredentialsImpl>(options);
}

} // namespace litegrpc