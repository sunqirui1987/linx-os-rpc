#ifndef LITEGRPC_CREDENTIALS_H
#define LITEGRPC_CREDENTIALS_H

/**
 * @file credentials.h
 * @brief LiteGRPC 凭证和通道参数管理
 * @details 定义了 gRPC 通道的安全凭证（SSL/TLS、不安全连接）和
 *          通道配置参数。提供了与标准 gRPC 完全兼容的凭证接口，
 *          支持各种连接安全模式和高级通道配置选项。
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 与标准 gRPC ChannelCredentials 和 ChannelArguments 完全兼容
 * @note 支持 SSL/TLS 加密和客户端证书认证
 * @note 提供丰富的 HTTP/2 连接参数配置
 */

#include <string>   // std::string
#include <memory>   // std::shared_ptr
#include "litegrpc/core.h"  // SslCredentialsOptions

namespace litegrpc {

/* ============================================================================
 * 通道凭证基类和实现
 * ============================================================================ */

/**
 * @class ChannelCredentials
 * @brief 通道凭证抽象基类
 * @details 定义了 gRPC 通道安全凭证的接口。凭证决定了客户端
 *          与服务器之间的连接安全模式（加密/非加密）。
 * 
 * @note 这是一个抽象基类，不能直接实例化
 * @note 使用工厂函数创建具体的凭证实例
 */
class ChannelCredentials {
public:
    /**
     * @brief 虚析构函数
     * @details 确保派生类能够正确析构
     */
    virtual ~ChannelCredentials() = default;
    
    /**
     * @brief 检查连接是否安全
     * @return true 如果使用加密连接，false 如果是明文连接
     * 
     * @details 用于判断当前凭证是否提供传输层安全保护
     */
    virtual bool IsSecure() const = 0;
    
    /**
     * @brief 获取凭证类型
     * @return 凭证类型字符串（如 "ssl", "insecure"）
     * 
     * @details 返回凭证的类型标识，用于调试和日志记录
     */
    virtual std::string GetType() const = 0;
};

/**
 * @class InsecureChannelCredentialsImpl
 * @brief 不安全（明文）通道凭证实现
 * @details 提供不加密的 HTTP/2 连接。数据以明文传输，
 *          适用于内网环境或开发测试场景。
 * 
 * @warning 生产环境中应谨慎使用，存在数据泄露风险
 * @note 与标准 gRPC InsecureChannelCredentials 兼容
 */
class InsecureChannelCredentialsImpl : public ChannelCredentials {
public:
    /**
     * @brief 检查连接是否安全
     * @return false 明文连接不安全
     */
    bool IsSecure() const override { return false; }
    
    /**
     * @brief 获取凭证类型
     * @return "insecure" 表示明文连接
     */
    std::string GetType() const override { return "insecure"; }
};

/**
 * @class SslChannelCredentialsImpl
 * @brief SSL/TLS 安全通道凭证实现
 * @details 提供基于 SSL/TLS 的加密连接。支持服务器证书验证
 *          和可选的客户端证书认证（双向 SSL）。
 * 
 * @note 与标准 gRPC SslCredentials 兼容
 * @note 支持自定义根证书和客户端证书
 */
class SslChannelCredentialsImpl : public ChannelCredentials {
public:
    /**
     * @brief 构造函数
     * @param options SSL 凭证配置选项
     * 
     * @details 使用指定的 SSL 配置创建安全凭证
     */
    explicit SslChannelCredentialsImpl(const SslCredentialsOptions& options)
        : options_(options) {}
    
    /**
     * @brief 检查连接是否安全
     * @return true SSL/TLS 连接是安全的
     */
    bool IsSecure() const override { return true; }
    
    /**
     * @brief 获取凭证类型
     * @return "ssl" 表示 SSL/TLS 加密连接
     */
    std::string GetType() const override { return "ssl"; }
    
    /**
     * @brief 获取 SSL 配置选项
     * @return SSL 凭证配置的常量引用
     */
    const SslCredentialsOptions& GetOptions() const { return options_; }
    
private:
    SslCredentialsOptions options_;  ///< SSL 凭证配置
};

/* ============================================================================
 * 通道参数配置
 * ============================================================================ */

/**
 * @class ChannelArguments
 * @brief 通道参数配置类
 * @details 管理 gRPC 通道的各种配置参数，包括连接保活、
 *          HTTP/2 设置、超时配置等。提供类型安全的参数设置接口。
 * 
 * @note 与标准 gRPC ChannelArguments 完全兼容
 * @note 支持整数、字符串和指针类型的参数
 */
class ChannelArguments {
public:
    /**
     * @brief 默认构造函数
     * @details 创建空的参数配置
     */
    ChannelArguments() = default;
    
    /* ========================================================================
     * 参数设置接口
     * ======================================================================== */
    
    /**
     * @brief 设置整数类型参数
     * @param key 参数键名
     * @param value 参数值
     */
    void SetInt(const std::string& key, int value);
    
    /**
     * @brief 设置字符串类型参数
     * @param key 参数键名
     * @param value 参数值
     */
    void SetString(const std::string& key, const std::string& value);
    
    /**
     * @brief 设置指针类型参数
     * @param key 参数键名
     * @param value 参数值
     */
    void SetPointer(const std::string& key, void* value);
    
    /* ========================================================================
     * 参数获取接口
     * ======================================================================== */
    
    /**
     * @brief 获取整数类型参数
     * @param key 参数键名
     * @param value 输出参数值的指针
     * @return true 如果参数存在，false 否则
     */
    bool GetInt(const std::string& key, int* value) const;
    
    /**
     * @brief 获取字符串类型参数
     * @param key 参数键名
     * @param value 输出参数值的指针
     * @return true 如果参数存在，false 否则
     */
    bool GetString(const std::string& key, std::string* value) const;
    
    /**
     * @brief 获取指针类型参数
     * @param key 参数键名
     * @param value 输出参数值的指针
     * @return true 如果参数存在，false 否则
     */
    bool GetPointer(const std::string& key, void** value) const;
    
    /* ========================================================================
     * 标准 gRPC 参数键常量 - 与官方 gRPC 完全兼容
     * ======================================================================== */
    
    /** @brief 连接保活时间间隔（毫秒） */
    static const std::string GRPC_ARG_KEEPALIVE_TIME_MS;
    
    /** @brief 连接保活超时时间（毫秒） */
    static const std::string GRPC_ARG_KEEPALIVE_TIMEOUT_MS;
    
    /** @brief 是否允许在没有活跃调用时发送保活包 */
    static const std::string GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS;
    
    /** @brief HTTP/2 无数据时最大 ping 数量 */
    static const std::string GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA;
    
    /** @brief HTTP/2 无数据时最小发送 ping 间隔（毫秒） */
    static const std::string GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS;
    
    /** @brief HTTP/2 无数据时最小接收 ping 间隔（毫秒） */
    static const std::string GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS;
    
    /** @brief 连接最大空闲时间（毫秒） */
    static const std::string GRPC_ARG_MAX_CONNECTION_IDLE_MS;
    
    /** @brief 连接最大生存时间（毫秒） */
    static const std::string GRPC_ARG_MAX_CONNECTION_AGE_MS;
    
    /** @brief 连接生存时间宽限期（毫秒） */
    static const std::string GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS;
    
private:
    /* ========================================================================
     * 私有成员变量 - 参数存储
     * ======================================================================== */
    
    std::map<std::string, int> int_args_;           ///< 整数类型参数
    std::map<std::string, std::string> string_args_; ///< 字符串类型参数
    std::map<std::string, void*> pointer_args_;     ///< 指针类型参数
};

/* ============================================================================
 * 凭证工厂函数 - 与标准 gRPC 完全兼容
 * ============================================================================ */

/**
 * @brief 创建不安全（明文）通道凭证
 * @return 不安全凭证的共享指针
 * 
 * @details 创建用于明文 HTTP/2 连接的凭证。
 *          适用于内网环境或开发测试。
 * 
 * @warning 生产环境应谨慎使用
 * @note 与 grpc::InsecureChannelCredentials() 兼容
 */
std::shared_ptr<ChannelCredentials> InsecureChannelCredentials();

/**
 * @brief 创建 SSL/TLS 安全通道凭证
 * @param options SSL 凭证配置选项
 * @return SSL 凭证的共享指针
 * 
 * @details 创建用于加密 HTTPS 连接的凭证。
 *          支持服务器证书验证和客户端证书认证。
 * 
 * @note 与 grpc::SslCredentials() 兼容
 */
std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options);

} // namespace litegrpc

#endif // LITEGRPC_CREDENTIALS_H