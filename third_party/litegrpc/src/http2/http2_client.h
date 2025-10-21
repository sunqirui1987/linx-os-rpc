/**
 * @file http2_client.h
 * @brief HTTP/2 客户端实现头文件
 * 
 * 此文件定义了基于 nghttp2 库的 HTTP/2 客户端实现，为 LiteGRPC 提供底层的
 * HTTP/2 通信能力。该实现支持标准的 HTTP/2 协议特性，包括多路复用、
 * 流控制、头部压缩等。
 * 
 * 主要特性：
 * - 基于 nghttp2 库的高性能 HTTP/2 实现
 * - 支持 SSL/TLS 加密连接
 * - 异步事件驱动的网络 I/O
 * - 完整的 HTTP/2 协议支持
 * - 与 gRPC 协议兼容的消息传输
 * 
 * 设计目标：
 * - 为 LiteGRPC 提供可靠的传输层
 * - 支持嵌入式和资源受限环境
 * - 保持与标准 gRPC 的协议兼容性
 * - 提供简洁易用的 C++ 接口
 * 
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 */

#ifndef LITEGRPC_HTTP2_CLIENT_H
#define LITEGRPC_HTTP2_CLIENT_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <nghttp2/nghttp2.h>  // nghttp2 库，提供 HTTP/2 协议实现
#include "litegrpc/status.h"  // LiteGRPC 状态码定义

namespace litegrpc {
namespace http2 {

/**
 * @brief HTTP/2 响应结构体
 * 
 * 封装了 HTTP/2 响应的所有信息，包括状态码、头部和响应体。
 * 用于在客户端接收和处理服务器响应时传递数据。
 * 
 * 字段说明：
 * - status_code: HTTP 状态码（如 200, 404, 500 等）
 * - headers: HTTP 头部字段的键值对映射
 * - body: 响应体内容（对于 gRPC，通常是序列化的 protobuf 数据）
 */
struct Http2Response {
    int status_code = 0;                                ///< HTTP 状态码
    std::map<std::string, std::string> headers;         ///< HTTP 头部字段
    std::string body;                                   ///< 响应体内容
};

/**
 * @brief HTTP/2 客户端类
 * 
 * 基于 nghttp2 库实现的 HTTP/2 客户端，提供完整的 HTTP/2 协议支持。
 * 该类封装了复杂的 HTTP/2 协议细节，为上层应用提供简洁的接口。
 * 
 * 主要功能：
 * - HTTP/2 连接建立和管理
 * - SSL/TLS 安全连接支持
 * - HTTP/2 请求发送和响应接收
 * - 多路复用流管理
 * - 自动处理 HTTP/2 协议握手
 * 
 * 使用场景：
 * - gRPC 客户端的底层传输实现
 * - 高性能 HTTP/2 客户端应用
 * - 需要 HTTP/2 协议支持的网络库
 * 
 * 线程安全性：
 * - 非线程安全，需要外部同步
 * - 建议每个线程使用独立的实例
 * 
 * 使用示例：
 * @code
 *   Http2Client client;
 *   Status status = client.Connect("example.com", 443, true);
 *   if (status.ok()) {
 *       Http2Response response;
 *       std::map<std::string, std::string> headers;
 *       headers["content-type"] = "application/grpc";
 *       client.SendRequest("POST", "/service/method", headers, body, &response);
 *   }
 * @endcode
 */
class Http2Client {
public:
    /**
     * @brief 构造函数
     * 
     * 初始化 HTTP/2 客户端实例，创建必要的内部状态结构。
     * 此时不会建立网络连接，需要调用 Connect() 方法建立连接。
     */
    Http2Client();
    
    /**
     * @brief 析构函数
     * 
     * 清理资源并关闭连接。如果连接仍然活跃，会自动调用 Disconnect()。
     * 释放 nghttp2 会话和相关的网络资源。
     */
    ~Http2Client();
    
    // ========== 连接管理接口 ==========
    
    /**
     * @brief 连接到 HTTP/2 服务器
     * @param host 服务器主机名或 IP 地址
     * @param port 服务器端口号
     * @param use_ssl 是否使用 SSL/TLS 加密连接
     * @return Status 连接状态，成功返回 OK
     * 
     * 建立到指定服务器的 HTTP/2 连接。此方法会：
     * 1. 创建 TCP 套接字连接
     * 2. 如果启用 SSL，进行 TLS 握手
     * 3. 执行 HTTP/2 协议握手
     * 4. 初始化 nghttp2 会话
     * 
     * 注意：
     * - 如果已有活跃连接，会先断开现有连接
     * - SSL 连接需要有效的证书验证
     * - 连接失败时会返回相应的错误状态
     */
    Status Connect(const std::string& host, int port, bool use_ssl);
    
    /**
     * @brief 断开连接
     * 
     * 优雅地关闭 HTTP/2 连接。此方法会：
     * 1. 发送 GOAWAY 帧通知服务器
     * 2. 等待未完成的流结束
     * 3. 关闭 TCP 套接字
     * 4. 清理 nghttp2 会话资源
     * 
     * 此方法是幂等的，多次调用不会产生副作用。
     */
    void Disconnect();
    
    /**
     * @brief 检查连接状态
     * @return bool 如果连接活跃返回 true，否则返回 false
     * 
     * 检查当前是否有活跃的 HTTP/2 连接。此方法会验证：
     * - TCP 套接字是否有效
     * - nghttp2 会话是否正常
     * - 连接是否可用于发送请求
     */
    bool IsConnected() const;
    
    // ========== HTTP/2 请求接口 ==========
    
    /**
     * @brief 发送 HTTP/2 请求
     * @param method HTTP 方法（如 "GET", "POST", "PUT" 等）
     * @param path 请求路径（如 "/service/method"）
     * @param headers HTTP 头部字段的键值对映射
     * @param body 请求体内容（对于 gRPC，通常是序列化的 protobuf 数据）
     * @param response 输出参数，用于接收服务器响应
     * @return Status 请求状态，成功返回 OK
     * 
     * 发送 HTTP/2 请求并等待响应。此方法会：
     * 1. 创建新的 HTTP/2 流
     * 2. 发送请求头部和数据
     * 3. 等待服务器响应
     * 4. 接收响应头部和数据
     * 5. 关闭流并返回结果
     * 
     * 特性：
     * - 支持任意 HTTP 方法和头部
     * - 自动处理 HTTP/2 流控制
     * - 支持大文件传输
     * - 兼容 gRPC 协议要求
     * 
     * 注意：
     * - 必须在连接建立后调用
     * - 此方法是同步的，会阻塞直到响应完成
     * - 网络错误或协议错误会返回相应状态码
     */
    Status SendRequest(
        const std::string& method,
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::string& body,
        Http2Response* response);
    
private:
    // ========== 内部状态管理 ==========
    
    /**
     * @brief 连接状态结构体（前向声明）
     * 
     * 包含所有连接相关的状态信息，如套接字、SSL 上下文、
     * nghttp2 会话等。使用 PIMPL 模式隐藏实现细节。
     */
    struct ConnectionState;
    std::unique_ptr<ConnectionState> state_;  ///< 连接状态的智能指针
    
    // ========== nghttp2 回调函数 ==========
    
    /**
     * @brief 数据发送回调函数
     * @param session nghttp2 会话指针
     * @param data 要发送的数据缓冲区
     * @param length 数据长度
     * @param flags 发送标志
     * @param user_data 用户数据指针（指向 Http2Client 实例）
     * @return 实际发送的字节数，或错误码
     * 
     * 当 nghttp2 需要发送数据时调用此回调。实现将数据写入底层套接字。
     */
    static ssize_t SendCallback(nghttp2_session* session, const uint8_t* data,
                               size_t length, int flags, void* user_data);
    
    /**
     * @brief 帧接收回调函数
     * @param session nghttp2 会话指针
     * @param frame 接收到的 HTTP/2 帧
     * @param user_data 用户数据指针（指向 Http2Client 实例）
     * @return 0 表示成功，非 0 表示错误
     * 
     * 当接收到完整的 HTTP/2 帧时调用。用于处理各种类型的帧，
     * 如 HEADERS、DATA、SETTINGS 等。
     */
    static int OnFrameRecvCallback(nghttp2_session* session,
                                  const nghttp2_frame* frame, void* user_data);
    
    /**
     * @brief 数据块接收回调函数
     * @param session nghttp2 会话指针
     * @param flags 数据标志
     * @param stream_id 流 ID
     * @param data 接收到的数据缓冲区
     * @param len 数据长度
     * @param user_data 用户数据指针（指向 Http2Client 实例）
     * @return 0 表示成功，非 0 表示错误
     * 
     * 当接收到 DATA 帧的数据部分时调用。用于累积响应体数据。
     */
    static int OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags,
                                      int32_t stream_id, const uint8_t* data,
                                      size_t len, void* user_data);
    
    /**
     * @brief 头部字段接收回调函数
     * @param session nghttp2 会话指针
     * @param frame 包含头部的帧
     * @param name 头部字段名
     * @param namelen 字段名长度
     * @param value 头部字段值
     * @param valuelen 字段值长度
     * @param flags 头部标志
     * @param user_data 用户数据指针（指向 Http2Client 实例）
     * @return 0 表示成功，非 0 表示错误
     * 
     * 当接收到 HEADERS 帧中的每个头部字段时调用。
     * 用于构建响应的头部映射。
     */
    static int OnHeaderCallback(nghttp2_session* session,
                               const nghttp2_frame* frame,
                               const uint8_t* name, size_t namelen,
                               const uint8_t* value, size_t valuelen,
                               uint8_t flags, void* user_data);
    
    /**
     * @brief 流关闭回调函数
     * @param session nghttp2 会话指针
     * @param stream_id 被关闭的流 ID
     * @param error_code 错误码（0 表示正常关闭）
     * @param user_data 用户数据指针（指向 Http2Client 实例）
     * @return 0 表示成功，非 0 表示错误
     * 
     * 当 HTTP/2 流被关闭时调用。用于清理流相关资源并
     * 通知等待响应的线程。
     */
    static int OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id,
                                    uint32_t error_code, void* user_data);
    
    // ========== 内部方法 ==========
    
    /**
     * @brief 初始化 nghttp2 会话
     * @return Status 初始化状态
     * 
     * 创建并配置 nghttp2 客户端会话，设置回调函数和选项。
     */
    Status InitializeSession();
    
    /**
     * @brief 执行 HTTP/2 协议握手
     * @return Status 握手状态
     * 
     * 发送 HTTP/2 连接前言和初始 SETTINGS 帧，
     * 完成 HTTP/2 协议级别的握手。
     */
    Status PerformHandshake();
    
    /**
     * @brief 发送待发送的数据
     * @return Status 发送状态
     * 
     * 将 nghttp2 缓冲区中的数据发送到网络套接字。
     */
    Status SendData();
    
    /**
     * @brief 接收网络数据
     * @return Status 接收状态
     * 
     * 从网络套接字接收数据并提交给 nghttp2 处理。
     */
    Status ReceiveData();
    
    /**
     * @brief 处理 nghttp2 事件
     * @return Status 处理状态
     * 
     * 让 nghttp2 处理接收到的数据，触发相应的回调函数。
     */
    Status ProcessEvents();
    
    // ========== 套接字操作 ==========
    
    /**
     * @brief 创建网络套接字
     * @param host 目标主机名或 IP 地址
     * @param port 目标端口号
     * @return Status 创建状态
     * 
     * 创建 TCP 套接字并连接到指定的主机和端口。
     */
    Status CreateSocket(const std::string& host, int port);
    
    /**
     * @brief 设置 SSL/TLS 连接
     * @return Status 设置状态
     * 
     * 在现有套接字上建立 SSL/TLS 加密连接，
     * 执行 TLS 握手并验证服务器证书。
     */
    Status SetupSsl();
    
    /**
     * @brief 套接字数据发送
     * @param data 要发送的数据缓冲区
     * @param len 数据长度
     * @return 实际发送的字节数，或负数表示错误
     * 
     * 向套接字发送数据，支持 SSL 和非 SSL 连接。
     */
    ssize_t SocketSend(const void* data, size_t len);
    
    /**
     * @brief 套接字数据接收
     * @param data 接收数据的缓冲区
     * @param len 缓冲区大小
     * @return 实际接收的字节数，或负数表示错误
     * 
     * 从套接字接收数据，支持 SSL 和非 SSL 连接。
     */
    ssize_t SocketRecv(void* data, size_t len);
};

} // namespace http2
} // namespace litegrpc

#endif // LITEGRPC_HTTP2_CLIENT_H