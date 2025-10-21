/**
 * @file status.cpp
 * @brief LiteGRPC 状态类实现文件
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件实现了 LiteGRPC 状态类的具体功能，主要包括状态信息的字符串转换。
 * 提供了与标准 gRPC Status 完全兼容的状态表示和错误处理机制。
 * 
 * 实现功能：
 * - Status::ToString() 方法：将状态对象转换为可读的字符串表示
 * - 支持所有标准 gRPC 状态码的字符串映射
 * - 包含错误消息的格式化输出
 * - 处理未知状态码的情况
 */

#include "litegrpc/status.h"
#include <sstream>

namespace litegrpc {

/**
 * @brief 将状态对象转换为字符串表示
 * @return 包含状态码和错误消息的格式化字符串
 * 
 * 此方法将 Status 对象转换为人类可读的字符串格式。
 * 对于成功状态（OK），直接返回 "OK"。
 * 对于错误状态，返回格式为 "Status(STATUS_CODE, "error_message")" 的字符串。
 * 
 * 输出格式示例：
 * - 成功状态：OK
 * - 错误状态：Status(INVALID_ARGUMENT, "参数无效")
 * - 无消息错误：Status(NOT_FOUND)
 */
std::string Status::ToString() const {
    // 如果状态为成功，直接返回 "OK"
    if (ok()) {
        return "OK";
    }
    
    // 构建错误状态的字符串表示
    std::ostringstream oss;
    oss << "Status(";
    
    // 根据状态码添加对应的字符串表示
    switch (code_) {
        case StatusCode::CANCELLED:
            oss << "CANCELLED";
            break;
        case StatusCode::UNKNOWN:
            oss << "UNKNOWN";
            break;
        case StatusCode::INVALID_ARGUMENT:
            oss << "INVALID_ARGUMENT";
            break;
        case StatusCode::DEADLINE_EXCEEDED:
            oss << "DEADLINE_EXCEEDED";
            break;
        case StatusCode::NOT_FOUND:
            oss << "NOT_FOUND";
            break;
        case StatusCode::ALREADY_EXISTS:
            oss << "ALREADY_EXISTS";
            break;
        case StatusCode::PERMISSION_DENIED:
            oss << "PERMISSION_DENIED";
            break;
        case StatusCode::RESOURCE_EXHAUSTED:
            oss << "RESOURCE_EXHAUSTED";
            break;
        case StatusCode::FAILED_PRECONDITION:
            oss << "FAILED_PRECONDITION";
            break;
        case StatusCode::ABORTED:
            oss << "ABORTED";
            break;
        case StatusCode::OUT_OF_RANGE:
            oss << "OUT_OF_RANGE";
            break;
        case StatusCode::UNIMPLEMENTED:
            oss << "UNIMPLEMENTED";
            break;
        case StatusCode::INTERNAL:
            oss << "INTERNAL";
            break;
        case StatusCode::UNAVAILABLE:
            oss << "UNAVAILABLE";
            break;
        case StatusCode::DATA_LOSS:
            oss << "DATA_LOSS";
            break;
        case StatusCode::UNAUTHENTICATED:
            oss << "UNAUTHENTICATED";
            break;
        default:
            // 处理未知的状态码，显示数值
            oss << "UNKNOWN_CODE(" << static_cast<int>(code_) << ")";
            break;
    }
    
    // 如果有错误消息，添加到输出中
    if (!message_.empty()) {
        oss << ", \"" << message_ << "\"";
    }
    
    // 完成状态字符串的构建
    oss << ")";
    return oss.str();
}

} // namespace litegrpc