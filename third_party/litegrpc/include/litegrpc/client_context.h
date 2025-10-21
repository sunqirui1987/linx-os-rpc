#ifndef LITEGRPC_CLIENT_CONTEXT_H
#define LITEGRPC_CLIENT_CONTEXT_H

/**
 * @file client_context.h
 * @brief LiteGRPC 客户端上下文定义
 * @details 定义了 gRPC 客户端请求的上下文信息，包括元数据、超时设置、
 *          压缩算法、用户代理等配置。ClientContext 是每个 RPC 调用的
 *          配置载体，允许对单个请求进行精细控制。
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 与标准 gRPC ClientContext 完全兼容
 * @note 每个 RPC 调用都应该使用独立的 ClientContext 实例
 * @note 不支持拷贝和移动，确保上下文的唯一性
 */

#include <string>   // std::string
#include <map>      // std::map
#include <chrono>   // std::chrono::system_clock

namespace litegrpc {

/**
 * @class ClientContext
 * @brief gRPC 客户端请求上下文
 * @details 封装了单个 RPC 调用的所有配置信息，包括：
 *          - 请求元数据（HTTP 头）
 *          - 超时设置
 *          - 服务器权威名称
 *          - 压缩算法
 *          - 用户代理信息
 * 
 * @note 每个 RPC 调用都需要一个独立的 ClientContext 实例
 * @note 上下文对象不可复制，确保请求的唯一性和线程安全
 */
class ClientContext {
public:
    /**
     * @brief 默认构造函数
     * @details 创建一个空的客户端上下文，所有配置都使用默认值
     */
    ClientContext() = default;
    
    /**
     * @brief 析构函数
     * @details 清理上下文资源
     */
    ~ClientContext() = default;
    
    /* ========================================================================
     * 禁用拷贝和移动操作 - 确保上下文唯一性
     * ======================================================================== */
    
    /**
     * @brief 禁用拷贝构造函数
     * @details 防止意外的上下文复制，确保每个请求都有独立的上下文
     */
    ClientContext(const ClientContext&) = delete;
    
    /**
     * @brief 禁用拷贝赋值操作符
     */
    ClientContext& operator=(const ClientContext&) = delete;
    
    /**
     * @brief 禁用移动构造函数
     */
    ClientContext(ClientContext&&) = delete;
    
    /**
     * @brief 禁用移动赋值操作符
     */
    ClientContext& operator=(ClientContext&&) = delete;
    
    /* ========================================================================
     * 元数据管理 - HTTP 头信息
     * ======================================================================== */
    
    /**
     * @brief 添加请求元数据
     * @param key 元数据键名
     * @param value 元数据值
     * 
     * @details 添加自定义的 HTTP 头信息到请求中。
     *          元数据会作为 HTTP/2 头部发送给服务器。
     * 
     * @note 键名会自动转换为小写（HTTP/2 规范要求）
     * @note 如果键已存在，会覆盖原有值
     */
    void AddMetadata(const std::string& key, const std::string& value);
    
    /**
     * @brief 获取所有元数据
     * @return 元数据映射的常量引用
     */
    const std::map<std::string, std::string>& GetMetadata() const;
    
    /* ========================================================================
     * 超时管理 - 请求截止时间控制
     * ======================================================================== */
    
    /**
     * @brief 设置请求截止时间
     * @param deadline 绝对截止时间点
     * 
     * @details 设置 RPC 调用的最大等待时间。如果在截止时间前
     *          没有收到响应，请求会被取消并返回超时错误。
     * 
     * @note 截止时间是绝对时间，不是相对超时时间
     */
    void set_deadline(const std::chrono::system_clock::time_point& deadline);
    
    /**
     * @brief 获取截止时间
     * @return 设置的截止时间点
     * 
     * @note 如果没有设置截止时间，返回值未定义
     */
    std::chrono::system_clock::time_point deadline() const;
    
    /**
     * @brief 检查是否设置了截止时间
     * @return true 如果设置了截止时间，false 否则
     */
    bool has_deadline() const;
    
    /* ========================================================================
     * 权威名称管理 - 服务器身份验证
     * ======================================================================== */
    
    /**
     * @brief 设置服务器权威名称
     * @param authority 权威名称（通常是主机名）
     * 
     * @details 覆盖默认的服务器权威名称，用于 SSL 证书验证
     *          和虚拟主机路由。在使用负载均衡器或代理时特别有用。
     */
    void set_authority(const std::string& authority);
    
    /**
     * @brief 获取权威名称
     * @return 设置的权威名称
     */
    const std::string& authority() const;
    
    /* ========================================================================
     * 压缩算法管理 - 数据压缩配置
     * ======================================================================== */
    
    /**
     * @brief 设置压缩算法
     * @param algorithm 压缩算法名称（如 "gzip", "deflate"）
     * 
     * @details 指定请求和响应数据的压缩算法。
     *          压缩可以减少网络传输量，但会增加 CPU 开销。
     * 
     * @note 服务器必须支持指定的压缩算法
     */
    void set_compression_algorithm(const std::string& algorithm);
    
    /**
     * @brief 获取压缩算法
     * @return 设置的压缩算法名称
     */
    const std::string& compression_algorithm() const;
    
    /* ========================================================================
     * 用户代理管理 - 客户端标识
     * ======================================================================== */
    
    /**
     * @brief 设置用户代理前缀
     * @param user_agent_prefix 用户代理前缀字符串
     * 
     * @details 设置 HTTP User-Agent 头的前缀部分，
     *          用于标识客户端应用程序。
     */
    void set_user_agent_prefix(const std::string& user_agent_prefix);
    
    /**
     * @brief 获取用户代理前缀
     * @return 设置的用户代理前缀
     */
    const std::string& user_agent_prefix() const;
    
    /* ========================================================================
     * 内部实现方法 - 框架内部使用
     * ======================================================================== */
    
    /**
     * @brief 重置上下文状态
     * @details 清除所有配置，恢复到初始状态
     * 
     * @note 主要用于上下文对象的重用（不推荐）
     */
    void Reset();
    
    /**
     * @brief 检查请求是否已过期
     * @return true 如果已超过截止时间，false 否则
     * 
     * @note 内部方法，用于超时检查
     */
    bool IsExpired() const;
    
    /**
     * @brief 获取剩余超时时间（毫秒）
     * @return 剩余超时时间，如果没有设置截止时间则返回 -1
     * 
     * @note 内部方法，用于 HTTP/2 超时设置
     */
    int GetTimeoutMs() const;
    
private:
    /* ========================================================================
     * 私有成员变量
     * ======================================================================== */
    
    std::map<std::string, std::string> metadata_;           ///< 请求元数据
    std::chrono::system_clock::time_point deadline_;        ///< 截止时间
    bool has_deadline_ = false;                             ///< 是否设置了截止时间
    std::string authority_;                                 ///< 服务器权威名称
    std::string compression_algorithm_;                     ///< 压缩算法
    std::string user_agent_prefix_;                         ///< 用户代理前缀
};

} // namespace litegrpc

#endif // LITEGRPC_CLIENT_CONTEXT_H