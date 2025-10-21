#ifndef LITEGRPC_CORE_H
#define LITEGRPC_CORE_H

/**
 * @file core.h
 * @brief LiteGRPC 核心定义和配置
 * @details 定义了 LiteGRPC 框架的核心配置、前向声明和基础数据结构。
 *          这个文件包含了框架的全局配置常量、SSL 凭证选项以及
 *          主要类的前向声明，是整个框架的基础头文件。
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 与标准 gRPC 核心接口完全兼容
 * @note 提供轻量级的 gRPC 实现，专为 LinxOS 优化
 * @note 支持 HTTP/2 协议和 SSL/TLS 加密
 */

#include <string>   // std::string
#include <memory>   // std::shared_ptr, std::unique_ptr
#include <map>      // std::map
#include <vector>   // std::vector

namespace litegrpc {

/* ============================================================================
 * 核心配置结构体
 * ============================================================================ */

/**
 * @struct Config
 * @brief LiteGRPC 全局配置常量
 * @details 定义了框架的默认配置参数，包括超时时间、消息大小限制
 *          和用户代理字符串。这些配置影响整个框架的行为。
 * 
 * @note 所有配置都是编译时常量，确保性能最优
 * @note 配置值经过 LinxOS 环境优化调整
 */
struct Config {
    /**
     * @brief 默认 RPC 调用超时时间（毫秒）
     * @details 30 秒的默认超时时间，适合大多数网络环境。
     *          可以通过 ClientContext 为单个请求覆盖此值。
     */
    static constexpr int DEFAULT_TIMEOUT_MS = 30000;
    
    /**
     * @brief 默认最大消息大小（字节）
     * @details 4MB 的消息大小限制，平衡了内存使用和传输效率。
     *          超过此大小的消息会被拒绝。
     */
    static constexpr int DEFAULT_MAX_MESSAGE_SIZE = 4 * 1024 * 1024; // 4MB
    
    /**
     * @brief 默认用户代理字符串
     * @details 标识 LiteGRPC 客户端的用户代理信息，
     *          会在 HTTP/2 头部中发送给服务器。
     */
    static constexpr const char* DEFAULT_USER_AGENT = "LiteGRPC/1.0";
};

/* ============================================================================
 * 前向声明 - 避免循环依赖
 * ============================================================================ */

/**
 * @class Channel
 * @brief gRPC 通道前向声明
 * @details 表示到 gRPC 服务器的连接通道
 */
class Channel;

/**
 * @class ClientContext
 * @brief 客户端上下文前向声明
 * @details 封装单个 RPC 调用的配置信息
 */
class ClientContext;

/**
 * @class ChannelCredentials
 * @brief 通道凭证前向声明
 * @details 管理通道的安全凭证（SSL/TLS）
 */
class ChannelCredentials;

/**
 * @class ChannelArguments
 * @brief 通道参数前向声明
 * @details 配置通道的各种参数选项
 */
class ChannelArguments;

/**
 * @class StubInterface
 * @brief 存根接口前向声明
 * @details gRPC 服务客户端存根的基础接口
 */
class StubInterface;

/* ============================================================================
 * SSL/TLS 安全配置
 * ============================================================================ */

/**
 * @struct SslCredentialsOptions
 * @brief SSL 凭证配置选项
 * @details 定义了 SSL/TLS 连接所需的证书和密钥信息。
 *          支持客户端证书认证和服务器证书验证。
 * 
 * @note 所有证书和密钥都应该是 PEM 格式
 * @note 空字符串表示使用系统默认配置
 */
struct SslCredentialsOptions {
    /**
     * @brief 根证书（CA 证书）PEM 格式字符串
     * @details 用于验证服务器证书的根证书链。
     *          如果为空，将使用系统默认的根证书。
     */
    std::string pem_root_certs;
    
    /**
     * @brief 客户端私钥 PEM 格式字符串
     * @details 客户端证书对应的私钥，用于双向 SSL 认证。
     *          只有在需要客户端证书认证时才需要设置。
     */
    std::string pem_private_key;
    
    /**
     * @brief 客户端证书链 PEM 格式字符串
     * @details 客户端证书及其证书链，用于双向 SSL 认证。
     *          只有在需要客户端证书认证时才需要设置。
     */
    std::string pem_cert_chain;
};

} // namespace litegrpc

#endif // LITEGRPC_CORE_H