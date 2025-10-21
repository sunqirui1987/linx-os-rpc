/**
 * @file client_context.cpp
 * @brief LiteGRPC 客户端上下文实现文件
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件实现了 LiteGRPC 客户端上下文类的具体功能，用于管理 RPC 调用的上下文信息。
 * 提供了与标准 gRPC ClientContext 完全兼容的接口实现。
 * 
 * 实现功能：
 * - 元数据管理：添加和获取请求元数据
 * - 超时管理：设置和检查调用截止时间
 * - 权威名称管理：设置目标服务的权威名称
 * - 压缩算法管理：配置请求压缩算法
 * - 用户代理管理：设置客户端用户代理信息
 * - 上下文重置：清理所有上下文信息
 * - 超时检查：判断调用是否已过期
 */

#include "litegrpc/client_context.h"
#include <chrono>

namespace litegrpc {

/**
 * @brief 添加元数据键值对
 * @param key 元数据键
 * @param value 元数据值
 * 
 * 将指定的键值对添加到请求元数据中。如果键已存在，则覆盖原有值。
 * 元数据会在 RPC 调用时发送给服务端。
 */
void ClientContext::AddMetadata(const std::string& key, const std::string& value) {
    metadata_[key] = value;
}

/**
 * @brief 获取所有元数据
 * @return 包含所有元数据键值对的映射表的常量引用
 * 
 * 返回当前上下文中存储的所有元数据信息。
 */
const std::map<std::string, std::string>& ClientContext::GetMetadata() const {
    return metadata_;
}

/**
 * @brief 设置调用截止时间
 * @param deadline 截止时间点
 * 
 * 设置 RPC 调用的截止时间。如果调用在此时间点之前未完成，
 * 将被取消并返回 DEADLINE_EXCEEDED 错误。
 */
void ClientContext::set_deadline(const std::chrono::system_clock::time_point& deadline) {
    deadline_ = deadline;
    has_deadline_ = true;
}

/**
 * @brief 获取截止时间
 * @return 当前设置的截止时间点
 * 
 * 返回之前通过 set_deadline() 设置的截止时间。
 * 如果未设置截止时间，返回值未定义。
 */
std::chrono::system_clock::time_point ClientContext::deadline() const {
    return deadline_;
}

/**
 * @brief 检查是否设置了截止时间
 * @return 如果设置了截止时间返回 true，否则返回 false
 * 
 * 用于判断当前上下文是否配置了调用超时。
 */
bool ClientContext::has_deadline() const {
    return has_deadline_;
}

/**
 * @brief 设置权威名称
 * @param authority 目标服务的权威名称
 * 
 * 设置 RPC 调用的目标权威名称，通常用于指定服务的主机名或域名。
 * 这在负载均衡或服务发现场景中特别有用。
 */
void ClientContext::set_authority(const std::string& authority) {
    authority_ = authority;
}

/**
 * @brief 获取权威名称
 * @return 当前设置的权威名称
 * 
 * 返回之前通过 set_authority() 设置的权威名称。
 */
const std::string& ClientContext::authority() const {
    return authority_;
}

/**
 * @brief 设置压缩算法
 * @param algorithm 压缩算法名称
 * 
 * 设置 RPC 调用使用的压缩算法，用于减少网络传输的数据量。
 * 常见的算法包括 "gzip"、"deflate" 等。
 */
void ClientContext::set_compression_algorithm(const std::string& algorithm) {
    compression_algorithm_ = algorithm;
}

/**
 * @brief 获取压缩算法
 * @return 当前设置的压缩算法名称
 * 
 * 返回之前通过 set_compression_algorithm() 设置的压缩算法。
 */
const std::string& ClientContext::compression_algorithm() const {
    return compression_algorithm_;
}

/**
 * @brief 设置用户代理前缀
 * @param user_agent_prefix 用户代理字符串前缀
 * 
 * 设置客户端的用户代理信息前缀，用于标识客户端应用程序。
 * 完整的用户代理字符串会包含此前缀和 gRPC 版本信息。
 */
void ClientContext::set_user_agent_prefix(const std::string& user_agent_prefix) {
    user_agent_prefix_ = user_agent_prefix;
}

/**
 * @brief 获取用户代理前缀
 * @return 当前设置的用户代理前缀
 * 
 * 返回之前通过 set_user_agent_prefix() 设置的用户代理前缀。
 */
const std::string& ClientContext::user_agent_prefix() const {
    return user_agent_prefix_;
}

/**
 * @brief 重置上下文
 * 
 * 清除所有上下文信息，包括元数据、截止时间、权威名称、
 * 压缩算法和用户代理前缀。将上下文恢复到初始状态。
 * 
 * 此方法通常在重用 ClientContext 对象进行多次 RPC 调用时使用。
 */
void ClientContext::Reset() {
    metadata_.clear();
    has_deadline_ = false;
    authority_.clear();
    compression_algorithm_.clear();
    user_agent_prefix_.clear();
}

/**
 * @brief 检查调用是否已过期
 * @return 如果调用已过期返回 true，否则返回 false
 * 
 * 检查当前时间是否已超过设置的截止时间。
 * 如果未设置截止时间，则永远不会过期，返回 false。
 */
bool ClientContext::IsExpired() const {
    if (!has_deadline_) {
        return false;
    }
    return std::chrono::system_clock::now() > deadline_;
}

/**
 * @brief 获取剩余超时时间（毫秒）
 * @return 剩余超时时间（毫秒），-1 表示无超时，0 表示已过期
 * 
 * 计算从当前时间到截止时间的剩余毫秒数。
 * 返回值说明：
 * - -1：未设置截止时间，无超时限制
 * - 0：已过期
 * - >0：剩余的毫秒数
 */
int ClientContext::GetTimeoutMs() const {
    if (!has_deadline_) {
        return -1; // 无超时
    }
    
    auto now = std::chrono::system_clock::now();
    if (deadline_ <= now) {
        return 0; // 已过期
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - now);
    return static_cast<int>(duration.count());
}

} // namespace litegrpc