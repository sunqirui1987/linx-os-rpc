#ifndef LITEGRPC_CHANNEL_H
#define LITEGRPC_CHANNEL_H

/**
 * @file channel.h
 * @brief LiteGRPC 通道接口定义
 * @details 定义了 gRPC 通道的抽象接口和具体实现，负责管理客户端与服务器之间的连接。
 *          通道是 gRPC 通信的核心组件，封装了底层的网络连接、HTTP/2 协议处理、
 *          SSL/TLS 安全传输等功能。
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 本实现与标准 gRPC Channel 接口完全兼容
 * @note 支持同步和异步 RPC 调用
 * @note 内置连接管理和重连机制
 */

#include <string>       // std::string
#include <memory>       // std::shared_ptr, std::unique_ptr
#include <chrono>       // std::chrono::system_clock
#include "litegrpc/core.h"        // 核心配置和类型定义
#include "litegrpc/status.h"      // 状态码和错误处理
#include "litegrpc/credentials.h" // 安全凭据管理

namespace litegrpc {

// 前向声明
class ClientContext;

/**
 * @class Channel
 * @brief gRPC 通道抽象基类
 * @details 定义了 gRPC 通道的标准接口，所有具体的通道实现都必须继承此类。
 *          通道负责管理与 gRPC 服务器的连接，处理 RPC 请求的发送和响应的接收。
 * 
 * @note 这是一个纯虚基类，不能直接实例化
 * @note 与标准 gRPC Channel 接口完全兼容
 */
class Channel {
public:
    /**
     * @brief 虚析构函数
     * @details 确保派生类能够正确析构
     */
    virtual ~Channel() = default;
    
    /* ========================================================================
     * 连接管理接口
     * ======================================================================== */
    
    /**
     * @brief 检查通道是否已连接
     * @return true 如果通道已建立连接，false 否则
     * 
     * @note 这是一个非阻塞操作
     */
    virtual bool IsConnected() const = 0;
    
    /**
     * @brief 建立与服务器的连接
     * @return Status 连接结果状态
     * 
     * @note 这是一个阻塞操作，会等待连接建立完成
     * @note 如果已经连接，则直接返回成功状态
     */
    virtual Status Connect() = 0;
    
    /**
     * @brief 断开与服务器的连接
     * @details 主动关闭连接，释放相关资源
     * 
     * @note 断开连接后，需要重新调用 Connect() 才能发送请求
     */
    virtual void Disconnect() = 0;
    
    /**
     * @brief 等待连接建立（带超时）
     * @param deadline 等待的截止时间点
     * @return true 如果在截止时间前连接建立，false 如果超时
     * 
     * @note 这是一个阻塞操作，会等待直到连接建立或超时
     */
    virtual bool WaitForConnected(std::chrono::system_clock::time_point deadline) = 0;
    
    /* ========================================================================
     * 请求执行接口
     * ======================================================================== */
    
    /**
     * @brief 执行 RPC 请求
     * @param method RPC 方法名（格式：/service/method）
     * @param context 客户端上下文，包含请求元数据和配置
     * @param request_data 序列化后的请求数据
     * @param response_data 输出参数，存储响应数据
     * @return Status 请求执行结果状态
     * 
     * @note 这是同步 RPC 调用的核心接口
     * @note 请求和响应数据都是序列化后的二进制格式
     */
    virtual Status ExecuteRequest(
        const std::string& method,
        ClientContext* context,
        const std::string& request_data,
        std::string* response_data) = 0;
    
    /* ========================================================================
     * 通道信息查询接口
     * ======================================================================== */
    
    /**
     * @brief 获取目标服务器地址
     * @return 目标地址字符串（格式：host:port）
     */
    virtual std::string GetTarget() const = 0;
    
    /**
     * @brief 获取通道凭据
     * @return 通道使用的安全凭据
     */
    virtual std::shared_ptr<ChannelCredentials> GetCredentials() const = 0;
    
    /**
     * @brief 获取通道参数
     * @return 通道配置参数的常量引用
     */
    virtual const ChannelArguments& GetArguments() const = 0;
};

/**
 * @class LiteGrpcChannel
 * @brief LiteGRPC 通道的具体实现
 * @details 基于 HTTP/2 协议实现的轻量级 gRPC 通道，支持安全和非安全连接。
 *          提供完整的 gRPC 客户端功能，包括连接管理、请求发送、响应处理等。
 * 
 * @note 与标准 gRPC Channel 完全兼容
 * @note 内置 HTTP/2 客户端，无需外部依赖
 * @note 支持 SSL/TLS 加密传输
 */
class LiteGrpcChannel : public Channel {
public:
    /**
     * @brief 构造函数
     * @param target 目标服务器地址（格式：host:port 或 https://host:port）
     * @param credentials 通道安全凭据
     * @param args 通道配置参数
     * 
     * @note 构造函数不会立即建立连接，需要调用 Connect() 或发送请求时自动连接
     */
    LiteGrpcChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> credentials,
        const ChannelArguments& args);
    
    /**
     * @brief 析构函数
     * @details 自动断开连接并释放所有资源
     */
    ~LiteGrpcChannel() override;
    
    /* ========================================================================
     * Channel 接口实现
     * ======================================================================== */
    
    /**
     * @brief 检查连接状态
     * @return true 如果已连接，false 否则
     */
    bool IsConnected() const override;
    
    /**
     * @brief 建立连接
     * @return Status 连接结果
     */
    Status Connect() override;
    
    /**
     * @brief 断开连接
     */
    void Disconnect() override;
    
    /**
     * @brief 等待连接建立
     * @param deadline 等待截止时间
     * @return true 如果连接成功，false 如果超时
     */
    bool WaitForConnected(std::chrono::system_clock::time_point deadline) override;
    
    /**
     * @brief 执行 RPC 请求
     * @param method RPC 方法名
     * @param context 客户端上下文
     * @param request_data 请求数据
     * @param response_data 响应数据输出
     * @return Status 执行结果
     */
    Status ExecuteRequest(
        const std::string& method,
        ClientContext* context,
        const std::string& request_data,
        std::string* response_data) override;
    
    /* ========================================================================
     * Protobuf 消息调用方法 - 类型安全的 RPC 接口
     * ======================================================================== */
    
    /**
     * @brief 调用 RPC 方法（Protobuf 消息版本）
     * @tparam RequestType 请求消息类型
     * @tparam ResponseType 响应消息类型
     * @param method RPC 方法名（格式：/service/method）
     * @param context 客户端上下文
     * @param request 请求消息对象
     * @param response 响应消息对象（输出参数）
     * @return Status 调用结果状态
     * 
     * @details 这是一个模板方法，提供类型安全的 RPC 调用接口。
     *          自动处理 Protobuf 消息的序列化和反序列化。
     * 
     * @note 要求 RequestType 和 ResponseType 都是 Protobuf 消息类型
     * @note 内部会调用 SerializeToString() 和 ParseFromString() 方法
     */
    template<typename RequestType, typename ResponseType>
    Status CallMethod(const std::string& method,
                     ClientContext& context,
                     const RequestType& request,
                     ResponseType* response) {
        // 序列化请求消息
        std::string request_data;
        if (!request.SerializeToString(&request_data)) {
            return Status::Internal("Failed to serialize request");
        }
        
        // 执行 RPC 请求
        std::string response_data;
        auto status = ExecuteRequest(method, &context, request_data, &response_data);
        if (!status.ok()) {
            return status;
        }
        
        // 反序列化响应消息
        if (!response->ParseFromString(response_data)) {
            return Status::Internal("Failed to parse response");
        }
        
        return Status::OK();
    }
    
    /* ========================================================================
     * 通道信息查询方法
     * ======================================================================== */
    
    /**
     * @brief 获取目标地址
     * @return 目标服务器地址
     */
    std::string GetTarget() const override { return target_; }
    
    /**
     * @brief 获取凭据
     * @return 通道安全凭据
     */
    std::shared_ptr<ChannelCredentials> GetCredentials() const override { return credentials_; }
    
    /**
     * @brief 获取参数
     * @return 通道配置参数
     */
    const ChannelArguments& GetArguments() const override { return args_; }

private:
    /* ========================================================================
     * 私有成员变量
     * ======================================================================== */
    
    std::string target_;                                    ///< 目标服务器地址
    std::shared_ptr<ChannelCredentials> credentials_;       ///< 安全凭据
    ChannelArguments args_;                                 ///< 通道参数
    bool connected_;                                        ///< 连接状态标志
    
    /**
     * @brief HTTP/2 连接详细信息
     * @details 使用 PIMPL 模式隐藏 HTTP/2 实现细节
     */
    struct Http2Connection;
    std::unique_ptr<Http2Connection> connection_;
    
    /* ========================================================================
     * 私有辅助方法
     * ======================================================================== */
    
    /**
     * @brief 解析目标地址
     * @param target 目标地址字符串
     * @param host 输出主机名
     * @param port 输出端口号
     * @param use_ssl 输出是否使用 SSL
     * @return Status 解析结果
     */
    Status ParseTarget(const std::string& target, std::string* host, int* port, bool* use_ssl);
    
    /**
     * @brief 建立底层连接
     * @return Status 连接建立结果
     */
    Status EstablishConnection();
    
    /**
     * @brief 发送 HTTP/2 请求
     * @param method HTTP 方法
     * @param headers 请求头
     * @param body 请求体
     * @param response_body 响应体输出
     * @param response_headers 响应头输出
     * @return Status 发送结果
     */
    Status SendHttp2Request(
        const std::string& method,
        const std::map<std::string, std::string>& headers,
        const std::string& body,
        std::string* response_body,
        std::map<std::string, std::string>* response_headers);
};

/* ========================================================================
 * 通道工厂函数 - 便捷的通道创建接口
 * ======================================================================== */

/**
 * @brief 创建标准通道
 * @param target 目标服务器地址（格式：host:port）
 * @param creds 通道安全凭据
 * @return 创建的通道智能指针
 * 
 * @details 使用默认参数创建 gRPC 通道，适用于大多数场景。
 *          内部会使用默认的 ChannelArguments 配置。
 * 
 * @note 这是最常用的通道创建方法
 * @note 与标准 gRPC CreateChannel 函数完全兼容
 */
std::shared_ptr<Channel> CreateChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds);

/**
 * @brief 创建自定义通道
 * @param target 目标服务器地址（格式：host:port）
 * @param creds 通道安全凭据
 * @param args 自定义通道参数
 * @return 创建的通道智能指针
 * 
 * @details 使用自定义参数创建 gRPC 通道，允许精细控制通道行为。
 *          可以配置超时、重试策略、压缩算法等高级选项。
 * 
 * @note 适用于需要特殊配置的高级场景
 * @note 与标准 gRPC CreateCustomChannel 函数完全兼容
 */
std::shared_ptr<Channel> CreateCustomChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds,
    const ChannelArguments& args);

} // namespace litegrpc

#endif // LITEGRPC_CHANNEL_H