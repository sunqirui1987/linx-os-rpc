/**
 * @file status.h
 * @brief LiteGRPC 状态管理头文件
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件定义了 LiteGRPC 的状态管理系统，包括状态码枚举和状态类。
 * 提供了与标准 gRPC Status 接口兼容的状态表示和错误处理机制。
 * 
 * 主要功能：
 * - 定义标准 gRPC 状态码枚举
 * - 提供状态对象封装，包含状态码和错误消息
 * - 支持各种预定义状态的静态工厂方法
 * - 提供状态查询和字符串转换功能
 * - 与标准 gRPC Status API 完全兼容
 */

#ifndef LITEGRPC_STATUS_H
#define LITEGRPC_STATUS_H

#include <string>

namespace litegrpc {

/**
 * @brief gRPC 状态码枚举
 * 
 * 定义了所有标准 gRPC 状态码，与 grpc::StatusCode 完全兼容。
 * 这些状态码遵循 gRPC 规范，用于表示 RPC 调用的各种结果状态。
 */
enum class StatusCode {
    OK = 0,                    ///< 成功状态，操作正常完成
    CANCELLED = 1,             ///< 操作被取消（通常由调用者取消）
    UNKNOWN = 2,               ///< 未知错误，通常是服务器错误
    INVALID_ARGUMENT = 3,      ///< 客户端指定了无效参数
    DEADLINE_EXCEEDED = 4,     ///< 操作超时，在截止时间前未完成
    NOT_FOUND = 5,             ///< 请求的实体（文件或目录）未找到
    ALREADY_EXISTS = 6,        ///< 尝试创建的实体已存在
    PERMISSION_DENIED = 7,     ///< 调用者没有执行指定操作的权限
    RESOURCE_EXHAUSTED = 8,    ///< 资源耗尽（配额不足、磁盘空间不足等）
    FAILED_PRECONDITION = 9,   ///< 系统状态不满足操作的前置条件
    ABORTED = 10,              ///< 操作被中止，通常由于并发问题
    OUT_OF_RANGE = 11,         ///< 操作超出有效范围
    UNIMPLEMENTED = 12,        ///< 操作未实现或不支持
    INTERNAL = 13,             ///< 内部错误，通常是服务器 bug
    UNAVAILABLE = 14,          ///< 服务当前不可用，通常是临时状态
    DATA_LOSS = 15,            ///< 不可恢复的数据丢失或损坏
    UNAUTHENTICATED = 16       ///< 请求没有有效的身份验证凭据
};

/**
 * @brief gRPC 状态类
 * 
 * 封装了 gRPC 操作的状态信息，包括状态码和可选的错误消息。
 * 与标准 grpc::Status 完全兼容，提供了丰富的静态工厂方法来创建各种状态。
 * 
 * 主要特性：
 * - 封装状态码和错误消息
 * - 提供便捷的状态检查方法
 * - 支持所有标准 gRPC 状态的静态工厂方法
 * - 提供状态信息的字符串表示
 */
class Status {
public:
    /**
     * @brief 默认构造函数
     * 创建一个成功状态（OK）
     */
    Status() : code_(StatusCode::OK) {}
    
    /**
     * @brief 构造函数
     * @param code 状态码
     * @param message 错误消息（可选）
     */
    Status(StatusCode code, const std::string& message) 
        : code_(code), message_(message) {}
    
    /**
     * @brief 创建成功状态
     * @return 表示成功的 Status 对象
     */
    static Status OK() { return Status(); }
    
    /**
     * @brief 创建取消状态
     * @param message 错误消息（可选）
     * @return 表示操作被取消的 Status 对象
     */
    static Status Cancelled(const std::string& message = "") {
        return Status(StatusCode::CANCELLED, message);
    }
    
    /**
     * @brief 创建未知错误状态
     * @param message 错误消息（可选）
     * @return 表示未知错误的 Status 对象
     */
    static Status Unknown(const std::string& message = "") {
        return Status(StatusCode::UNKNOWN, message);
    }
    
    /**
     * @brief 创建无效参数状态
     * @param message 错误消息（可选）
     * @return 表示参数无效的 Status 对象
     */
    static Status InvalidArgument(const std::string& message = "") {
        return Status(StatusCode::INVALID_ARGUMENT, message);
    }
    
    /**
     * @brief 创建超时状态
     * @param message 错误消息（可选）
     * @return 表示操作超时的 Status 对象
     */
    static Status DeadlineExceeded(const std::string& message = "") {
        return Status(StatusCode::DEADLINE_EXCEEDED, message);
    }
    
    /**
     * @brief 创建未找到状态
     * @param message 错误消息（可选）
     * @return 表示资源未找到的 Status 对象
     */
    static Status NotFound(const std::string& message = "") {
        return Status(StatusCode::NOT_FOUND, message);
    }
    
    /**
     * @brief 创建已存在状态
     * @param message 错误消息（可选）
     * @return 表示资源已存在的 Status 对象
     */
    static Status AlreadyExists(const std::string& message = "") {
        return Status(StatusCode::ALREADY_EXISTS, message);
    }
    
    /**
     * @brief 创建权限拒绝状态
     * @param message 错误消息（可选）
     * @return 表示权限被拒绝的 Status 对象
     */
    static Status PermissionDenied(const std::string& message = "") {
        return Status(StatusCode::PERMISSION_DENIED, message);
    }
    
    /**
     * @brief 创建资源耗尽状态
     * @param message 错误消息（可选）
     * @return 表示资源耗尽的 Status 对象
     */
    static Status ResourceExhausted(const std::string& message = "") {
        return Status(StatusCode::RESOURCE_EXHAUSTED, message);
    }
    
    /**
     * @brief 创建前置条件失败状态
     * @param message 错误消息（可选）
     * @return 表示前置条件失败的 Status 对象
     */
    static Status FailedPrecondition(const std::string& message = "") {
        return Status(StatusCode::FAILED_PRECONDITION, message);
    }
    
    /**
     * @brief 创建中止状态
     * @param message 错误消息（可选）
     * @return 表示操作被中止的 Status 对象
     */
    static Status Aborted(const std::string& message = "") {
        return Status(StatusCode::ABORTED, message);
    }
    
    /**
     * @brief 创建超出范围状态
     * @param message 错误消息（可选）
     * @return 表示操作超出范围的 Status 对象
     */
    static Status OutOfRange(const std::string& message = "") {
        return Status(StatusCode::OUT_OF_RANGE, message);
    }
    
    /**
     * @brief 创建未实现状态
     * @param message 错误消息（可选）
     * @return 表示功能未实现的 Status 对象
     */
    static Status Unimplemented(const std::string& message = "") {
        return Status(StatusCode::UNIMPLEMENTED, message);
    }
    
    /**
     * @brief 创建内部错误状态
     * @param message 错误消息（可选）
     * @return 表示内部错误的 Status 对象
     */
    static Status Internal(const std::string& message = "") {
        return Status(StatusCode::INTERNAL, message);
    }
    
    /**
     * @brief 创建不可用状态
     * @param message 错误消息（可选）
     * @return 表示服务不可用的 Status 对象
     */
    static Status Unavailable(const std::string& message = "") {
        return Status(StatusCode::UNAVAILABLE, message);
    }
    
    /**
     * @brief 创建数据丢失状态
     * @param message 错误消息（可选）
     * @return 表示数据丢失的 Status 对象
     */
    static Status DataLoss(const std::string& message = "") {
        return Status(StatusCode::DATA_LOSS, message);
    }
    
    /**
     * @brief 创建未认证状态
     * @param message 错误消息（可选）
     * @return 表示身份验证失败的 Status 对象
     */
    static Status Unauthenticated(const std::string& message = "") {
        return Status(StatusCode::UNAUTHENTICATED, message);
    }
    
    /**
     * @brief 检查状态是否为成功
     * @return 如果状态为 OK 则返回 true，否则返回 false
     */
    bool ok() const { return code_ == StatusCode::OK; }
    
    /**
     * @brief 获取状态码
     * @return 当前状态的状态码
     */
    StatusCode error_code() const { return code_; }
    
    /**
     * @brief 获取错误消息
     * @return 当前状态的错误消息字符串引用
     */
    const std::string& error_message() const { return message_; }
    
    /**
     * @brief 将状态转换为字符串表示
     * @return 包含状态码和错误消息的字符串
     */
    std::string ToString() const;
    
private:
    StatusCode code_;      ///< 状态码
    std::string message_;  ///< 错误消息
};

} // namespace litegrpc

#endif // LITEGRPC_STATUS_H