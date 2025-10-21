#ifndef LITEGRPC_H
#define LITEGRPC_H

/**
 * @file litegrpc.h
 * @brief LiteGRPC 主头文件 - 轻量级 gRPC 实现
 * @details 这是 LiteGRPC 框架的主要入口头文件，提供了与标准 gRPC 
 *          完全兼容的 API 接口。LiteGRPC 是专为嵌入式系统和 LinxOS 
 *          优化的轻量级 gRPC 实现，支持 HTTP/2 协议和 SSL/TLS 加密。
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 与标准 gRPC C++ API 100% 兼容
 * @note 专为 LinxOS 嵌入式系统优化
 * @note 支持 LinxOS 设备服务协议
 * @note 提供完整的 gRPC 命名空间兼容层
 * 
 * @example
 * ```cpp
 * // 使用标准 gRPC API
 * auto channel = grpc::CreateChannel("localhost:50051", 
 *                                   grpc::InsecureChannelCredentials());
 * auto stub = MyService::NewStub(channel);
 * ```
 */

/* ============================================================================
 * 核心组件头文件包含
 * ============================================================================ */

#include "litegrpc/core.h"           // 核心配置和数据结构
#include "litegrpc/status.h"         // 状态码和错误处理
#include "litegrpc/channel.h"        // 通道连接管理
#include "litegrpc/client_context.h" // 客户端上下文
#include "litegrpc/credentials.h"    // 安全凭证管理
#include "litegrpc/stub.h"           // 服务存根接口

/* ============================================================================
 * 标准 gRPC 兼容命名空间
 * ============================================================================ */

/**
 * @namespace grpc
 * @brief 标准 gRPC 兼容命名空间
 * @details 提供与官方 gRPC C++ 库完全兼容的 API 接口。
 *          通过类型别名将 litegrpc 命名空间中的类型映射到
 *          grpc 命名空间，确保现有 gRPC 代码无需修改即可使用。
 * 
 * @note 100% 兼容标准 gRPC C++ API
 * @note 支持无缝迁移现有 gRPC 项目
 */
namespace grpc {
    /* ========================================================================
     * 核心类型别名 - 与标准 gRPC 完全兼容
     * ======================================================================== */
    
    /** @brief 状态码类型别名 */
    using Status = litegrpc::Status;
    
    /** @brief 状态码枚举别名 */
    using StatusCode = litegrpc::StatusCode;
    
    /** @brief 客户端上下文类型别名 */
    using ClientContext = litegrpc::ClientContext;
    
    /** @brief 通道参数类型别名 */
    using ChannelArguments = litegrpc::ChannelArguments;
    
    /** @brief 通道凭证类型别名 */
    using ChannelCredentials = litegrpc::ChannelCredentials;
    
    /** @brief 通道类型别名 */
    using Channel = litegrpc::Channel;
    
    /** @brief 存根接口类型别名 */
    using StubInterface = litegrpc::StubInterface;
    
    /** @brief SSL 凭证选项类型别名 */
    using SslCredentialsOptions = litegrpc::SslCredentialsOptions;
    
    /* ========================================================================
     * 工厂函数 - 与标准 gRPC 完全兼容
     * ======================================================================== */
    
    /**
     * @brief 创建 gRPC 通道
     * @param target 目标服务器地址（如 "localhost:50051"）
     * @param creds 通道安全凭证
     * @return 通道的共享指针
     * 
     * @details 创建到指定服务器的 gRPC 通道连接。
     *          与 grpc::CreateChannel() 完全兼容。
     */
    std::shared_ptr<Channel> CreateChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds);
    
    /**
     * @brief 创建自定义配置的 gRPC 通道
     * @param target 目标服务器地址
     * @param creds 通道安全凭证
     * @param args 通道配置参数
     * @return 通道的共享指针
     * 
     * @details 创建带有自定义配置的 gRPC 通道。
     *          与 grpc::CreateCustomChannel() 完全兼容。
     */
    std::shared_ptr<Channel> CreateCustomChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds,
        const ChannelArguments& args);
    
    /**
     * @brief 创建不安全通道凭证
     * @return 不安全凭证的共享指针
     * 
     * @details 创建明文连接凭证。
     *          与 grpc::InsecureChannelCredentials() 完全兼容。
     */
    std::shared_ptr<ChannelCredentials> InsecureChannelCredentials();
    
    /**
     * @brief 创建 SSL 安全凭证
     * @param options SSL 配置选项
     * @return SSL 凭证的共享指针
     * 
     * @details 创建 SSL/TLS 加密连接凭证。
     *          与 grpc::SslCredentials() 完全兼容。
     */
    std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options);
}

/* ============================================================================
 * LinxOS 设备服务兼容层
 * ============================================================================ */

/**
 * @namespace linxos_device
 * @brief LinxOS 设备服务命名空间
 * @details 提供 LinxOS 设备管理服务的 gRPC 客户端接口。
 *          包含设备注册、心跳检测和工具调用等核心功能。
 * 
 * @note 专为 LinxOS 生态系统设计
 * @note 支持设备生命周期管理
 * @note 提供工具调用代理功能
 */
namespace linxos_device {
    /* ========================================================================
     * Protobuf 消息类型前向声明
     * ======================================================================== */
    
    /** @brief 设备注册请求消息 */
    class RegisterDeviceRequest;
    
    /** @brief 设备注册响应消息 */
    class RegisterDeviceResponse;
    
    /** @brief 心跳检测请求消息 */
    class HeartbeatRequest;
    
    /** @brief 心跳检测响应消息 */
    class HeartbeatResponse;
    
    /** @brief 工具调用请求消息 */
    class ToolCallRequest;
    
    /** @brief 工具调用响应消息 */
    class ToolCallResponse;
    
    /**
     * @class LinxOSDeviceService
     * @brief LinxOS 设备服务类
     * @details 定义了 LinxOS 设备管理服务的接口，
     *          包含设备注册、心跳和工具调用功能。
     */
    class LinxOSDeviceService {
    public:
        /**
         * @class Stub
         * @brief LinxOS 设备服务客户端存根
         * @details 提供 LinxOS 设备服务的客户端调用接口。
         *          继承自标准 gRPC 存根接口，确保兼容性。
         * 
         * @note 与标准 gRPC 存根模式完全兼容
         * @note 支持同步 RPC 调用
         */
        class Stub : public grpc::StubInterface {
        public:
            /**
             * @brief 构造函数
             * @param channel gRPC 通道连接
             * 
             * @details 使用指定的通道创建服务存根
             */
            explicit Stub(std::shared_ptr<grpc::Channel> channel);
            
            /**
             * @brief 注册设备
             * @param context 客户端上下文
             * @param request 注册请求
             * @param response 注册响应
             * @return 调用状态
             * 
             * @details 向 LinxOS 系统注册新设备
             */
            grpc::Status RegisterDevice(
                grpc::ClientContext* context,
                const RegisterDeviceRequest& request,
                RegisterDeviceResponse* response);
            
            /**
             * @brief 发送心跳
             * @param context 客户端上下文
             * @param request 心跳请求
             * @param response 心跳响应
             * @return 调用状态
             * 
             * @details 向 LinxOS 系统发送设备心跳信号
             */
            grpc::Status Heartbeat(
                grpc::ClientContext* context,
                const HeartbeatRequest& request,
                HeartbeatResponse* response);
            
            /**
             * @brief 调用工具
             * @param context 客户端上下文
             * @param request 工具调用请求
             * @param response 工具调用响应
             * @return 调用状态
             * 
             * @details 通过 LinxOS 系统调用指定工具
             */
            grpc::Status CallTool(
                grpc::ClientContext* context,
                const ToolCallRequest& request,
                ToolCallResponse* response);
        
        private:
            std::shared_ptr<grpc::Channel> channel_;  ///< gRPC 通道连接
        };
    };
}

#endif // LITEGRPC_H