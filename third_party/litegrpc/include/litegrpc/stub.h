/**
 * @file stub.h
 * @brief LiteGRPC 客户端存根接口头文件
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件定义了 LiteGRPC 的客户端存根接口，为生成的服务客户端提供基础功能。
 * 存根（Stub）是客户端与服务端通信的代理对象，封装了 RPC 调用的底层细节。
 * 
 * 主要功能：
 * - 定义客户端存根的抽象基类接口
 * - 提供 RPC 调用的通用方法
 * - 管理与服务端的通道连接
 * - 与标准 gRPC Stub 接口兼容
 * - 支持各种 RPC 调用模式（同步/异步）
 */

#ifndef LITEGRPC_STUB_H
#define LITEGRPC_STUB_H

#include <memory>
#include "litegrpc/core.h"
#include "litegrpc/status.h"
#include "litegrpc/channel.h"

namespace litegrpc {

// 前向声明
class ClientContext;

/**
 * @brief 客户端存根接口抽象基类
 * 
 * 为所有生成的服务客户端存根提供基础功能和通用接口。
 * 存根对象封装了与特定服务通信的逻辑，隐藏了网络通信的复杂性。
 * 
 * 主要特性：
 * - 管理与服务端的通道连接
 * - 提供 RPC 调用的通用方法
 * - 支持客户端上下文传递
 * - 处理请求序列化和响应反序列化
 * - 与标准 gRPC Stub 接口兼容
 * 
 * 使用方式：
 * 通常不直接使用此类，而是使用 protoc 生成的具体服务存根类，
 * 这些生成的类继承自 StubInterface 并实现具体的服务方法。
 */
class StubInterface {
public:
    /**
     * @brief 虚析构函数
     * 确保派生类能够正确析构
     */
    virtual ~StubInterface() = default;
    
protected:
    /**
     * @brief 构造函数
     * @param channel 与服务端通信的通道对象
     * 
     * 注意：构造函数为 protected，只能被派生类调用
     */
    explicit StubInterface(std::shared_ptr<Channel> channel) : channel_(channel) {}
    
    /**
     * @brief 执行 RPC 调用的辅助方法
     * @param method 要调用的方法名称
     * @param context 客户端上下文，包含调用的元数据和配置
     * @param request_data 序列化后的请求数据
     * @param response_data 用于存储响应数据的字符串指针
     * @return 调用状态，包含成功/失败信息和错误详情
     * 
     * 此方法封装了 RPC 调用的通用逻辑：
     * - 通过通道发送请求
     * - 处理网络通信
     * - 接收并返回响应数据
     * - 处理各种错误情况
     */
    Status MakeCall(
        const std::string& method,
        ClientContext* context,
        const std::string& request_data,
        std::string* response_data);
    
    std::shared_ptr<Channel> channel_;  ///< 与服务端通信的通道对象
};

} // namespace litegrpc

#endif // LITEGRPC_STUB_H