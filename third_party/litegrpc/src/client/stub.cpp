/**
 * @file stub.cpp
 * @brief LiteGRPC 客户端存根实现
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件实现了 LiteGRPC 的客户端存根接口，提供：
 * - RPC 方法调用的统一接口
 * - 通道管理和请求转发
 * - 与标准 gRPC Stub 接口的兼容性
 * 
 * 主要特性：
 * - 简化的 RPC 调用接口
 * - 自动错误处理
 * - 支持自定义客户端上下文
 */

#include "litegrpc/stub.h"
#include "litegrpc/client_context.h"

namespace litegrpc {

/**
 * @brief 执行 RPC 方法调用
 * @param method RPC 方法名（格式：/package.service/method）
 * @param context 客户端上下文（包含元数据、超时等信息）
 * @param request_data 序列化的请求数据
 * @param response_data 用于存储响应数据的指针
 * @return 调用状态，成功返回 Status::OK()
 * 
 * 这是客户端存根的核心方法，负责：
 * 1. 验证通道可用性
 * 2. 将请求转发给底层通道
 * 3. 返回执行结果
 * 
 * 此方法为所有生成的客户端存根提供统一的调用接口。
 */
Status StubInterface::MakeCall(
    const std::string& method,
    ClientContext* context,
    const std::string& request_data,
    std::string* response_data) {
    
    // 检查通道是否可用
    if (!channel_) {
        return Status::Internal("Channel not available");
    }
    
    // 转发请求到通道执行
    return channel_->ExecuteRequest(method, context, request_data, response_data);
}

} // namespace litegrpc