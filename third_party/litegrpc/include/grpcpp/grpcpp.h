#pragma once

/**
 * @file grpcpp.h
 * @brief gRPC++ 兼容性头文件 - LiteGRPC 的 gRPC 兼容层
 * @details 本文件提供了与标准 gRPC++ 库的兼容接口，使得现有的 gRPC 代码
 *          可以无缝迁移到 LiteGRPC 实现。通过类型别名和工厂函数，
 *          确保 API 的完全兼容性。
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 这是一个替代性头文件，可以直接替换标准的 grpcpp/grpcpp.h
 * @note 支持所有常用的 gRPC 客户端功能，包括安全和非安全连接
 */

// 包含 LiteGRPC 主头文件
#include "litegrpc/litegrpc.h"

/**
 * @namespace grpc
 * @brief 标准 gRPC 命名空间
 * @details 提供与官方 gRPC 库完全兼容的 API 接口，
 *          所有类型和函数都映射到对应的 LiteGRPC 实现
 */
namespace grpc {
    /* ========================================================================
     * 类型别名定义 - 确保与标准 gRPC 的完全兼容性
     * ======================================================================== */
    
    /**
     * @brief 通道类型别名
     * @details 映射到 LiteGRPC 的 Channel 实现
     */
    using Channel = litegrpc::Channel;
    
    /**
     * @brief 通道参数类型别名
     * @details 用于配置 gRPC 通道的各种参数
     */
    using ChannelArguments = litegrpc::ChannelArguments;
    
    /**
     * @brief 通道凭据类型别名
     * @details 用于配置通道的安全凭据（SSL/TLS 等）
     */
    using ChannelCredentials = litegrpc::ChannelCredentials;
    
    /**
     * @brief 客户端上下文类型别名
     * @details 用于传递请求级别的配置和元数据
     */
    using ClientContext = litegrpc::ClientContext;
    
    /**
     * @brief 状态类型别名
     * @details 表示 RPC 调用的结果状态
     */
    using Status = litegrpc::Status;
    
    /**
     * @brief 状态码类型别名
     * @details 标准 gRPC 状态码枚举
     */
    using StatusCode = litegrpc::StatusCode;
    
    /**
     * @brief SSL 凭据选项类型别名
     * @details 用于配置 SSL/TLS 连接的各种选项
     */
    using SslCredentialsOptions = litegrpc::SslCredentialsOptions;
    
    /* ========================================================================
     * 工厂函数 - 与标准 gRPC 签名完全兼容
     * ======================================================================== */
    
    /**
     * @brief 创建自定义通道
     * @details 使用指定的目标地址、凭据和参数创建 gRPC 通道
     * 
     * @param target 目标服务器地址（格式：host:port）
     * @param creds 通道凭据（安全或非安全）
     * @param args 通道参数配置
     * @return 创建的通道智能指针
     * 
     * @note 这是标准 gRPC 的核心 API，确保完全兼容
     */
    std::shared_ptr<Channel> CreateCustomChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds,
        const ChannelArguments& args);
    
    /* ========================================================================
     * 凭据工厂函数 - 安全连接配置
     * ======================================================================== */
    
    /**
     * @brief 创建 SSL 凭据
     * @details 使用指定的 SSL 选项创建安全通道凭据
     * 
     * @param options SSL 配置选项（证书、私钥等）
     * @return SSL 凭据智能指针
     * 
     * @note 支持客户端证书认证和服务器证书验证
     */
    std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options);
    
    /**
     * @brief 创建非安全凭据
     * @details 创建用于非加密连接的凭据（仅用于开发和测试）
     * 
     * @return 非安全凭据智能指针
     * 
     * @warning 生产环境中应避免使用非安全连接
     */
    std::shared_ptr<ChannelCredentials> InsecureChannelCredentials();
}