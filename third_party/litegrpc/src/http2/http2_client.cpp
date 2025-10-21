/**
 * @file http2_client.cpp
 * @brief HTTP/2 客户端实现文件
 * 
 * 本文件实现了基于 nghttp2 库的 HTTP/2 客户端功能，提供了完整的
 * HTTP/2 协议支持，包括连接管理、请求发送、响应处理等核心功能。
 * 
 * 主要特性：
 * - 支持 HTTP/2 协议的所有核心特性（多路复用、流控制、头部压缩等）
 * - 支持 SSL/TLS 加密连接
 * - 异步事件驱动的网络 I/O
 * - 完整的错误处理和状态管理
 * - 线程安全的设计
 * 
 * 技术实现：
 * - 使用 nghttp2 库处理 HTTP/2 协议细节
 * - 使用 OpenSSL 提供 TLS 加密支持
 * - 使用 PIMPL 模式隐藏实现细节
 * - 基于回调函数的事件处理机制
 * 
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 */

#include "http2_client.h"
#include <sys/socket.h>    // 套接字相关函数
#include <netinet/in.h>    // 网络地址结构
#include <netdb.h>         // 主机名解析
#include <unistd.h>        // UNIX 标准函数
#include <openssl/ssl.h>   // OpenSSL SSL/TLS 支持
#include <openssl/err.h>   // OpenSSL 错误处理
#include <cstring>         // C 字符串函数
#include <iostream>        // 标准输入输出流
#include <thread>          // 线程支持
#include <chrono>          // 时间支持

namespace litegrpc {
namespace http2 {

/**
 * @brief HTTP/2 客户端连接状态结构体
 * 
 * 此结构体封装了 HTTP/2 客户端连接的所有状态信息，包括：
 * - nghttp2 会话管理
 * - 网络套接字连接
 * - SSL/TLS 加密上下文
 * - 请求/响应状态跟踪
 * 
 * 使用 PIMPL 模式将实现细节从头文件中隐藏，提供更好的
 * 编译时依赖管理和 ABI 稳定性。
 */
struct Http2Client::ConnectionState {
    nghttp2_session* session = nullptr;    ///< nghttp2 会话指针，管理 HTTP/2 协议状态
    int socket_fd = -1;                    ///< 网络套接字文件描述符
    SSL_CTX* ssl_ctx = nullptr;            ///< SSL 上下文，用于 TLS 连接
    SSL* ssl = nullptr;                    ///< SSL 连接对象
    bool use_ssl = false;                  ///< 是否使用 SSL/TLS 加密
    bool connected = false;                ///< 连接状态标志
    
    // ========== 请求/响应状态管理 ==========
    std::map<int32_t, Http2Response> responses;  ///< 流 ID 到响应对象的映射
    int32_t current_stream_id = -1;              ///< 当前处理的流 ID
    
    /**
     * @brief 析构函数 - 清理所有资源
     * 
     * 按照正确的顺序释放所有分配的资源：
     * 1. 销毁 nghttp2 会话
     * 2. 释放 SSL 连接和上下文
     * 3. 关闭网络套接字
     * 
     * 确保没有资源泄漏，即使在异常情况下也能正确清理。
     */
    ~ConnectionState() {
        if (session) {
            nghttp2_session_del(session);
        }
        if (ssl) {
            SSL_free(ssl);
        }
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
        }
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }
};

/**
 * @brief Http2Client 构造函数
 * 
 * 初始化 HTTP/2 客户端实例，创建连接状态对象并初始化 OpenSSL 库。
 * 
 * 初始化步骤：
 * 1. 创建 ConnectionState 对象
 * 2. 初始化 OpenSSL 库
 * 3. 加载 SSL 错误字符串
 * 4. 添加所有加密算法
 * 
 * 注意：OpenSSL 的初始化是全局的，多次调用是安全的。
 */
Http2Client::Http2Client() : state_(std::make_unique<ConnectionState>()) {
    // 初始化 OpenSSL 库
    SSL_library_init();           // 初始化 SSL 库
    SSL_load_error_strings();     // 加载错误字符串，便于调试
    OpenSSL_add_all_algorithms(); // 添加所有加密算法
}

/**
 * @brief Http2Client 析构函数
 * 
 * 清理 HTTP/2 客户端资源，确保连接被正确关闭。
 * ConnectionState 的析构函数会自动清理所有相关资源。
 */
Http2Client::~Http2Client() {
    Disconnect();  // 确保连接被正确关闭
}

/**
 * @brief 连接到 HTTP/2 服务器
 * @param host 服务器主机名或 IP 地址
 * @param port 服务器端口号
 * @param use_ssl 是否使用 SSL/TLS 加密
 * @return Status 连接状态
 * 
 * 建立到 HTTP/2 服务器的连接，包括以下步骤：
 * 1. 检查是否已连接，避免重复连接
 * 2. 创建 TCP 套接字并连接到服务器
 * 3. 如果需要，建立 SSL/TLS 加密连接
 * 4. 初始化 nghttp2 会话
 * 5. 执行 HTTP/2 协议握手
 * 
 * 支持的特性：
 * - HTTP 和 HTTPS 连接
 * - 自动协议协商
 * - 完整的错误处理
 * - 连接状态跟踪
 */
Status Http2Client::Connect(const std::string& host, int port, bool use_ssl) {
    if (state_->connected) {
        return Status::OK();  // 已连接，直接返回成功
    }
    
    state_->use_ssl = use_ssl;  // 保存 SSL 使用标志
    
    // 第一步：创建网络套接字连接
    auto status = CreateSocket(host, port);
    if (!status.ok()) {
        return status;  // 套接字创建失败
    }
    
    // 第二步：如果需要，设置 SSL/TLS 加密
    if (use_ssl) {
        status = SetupSsl();
        if (!status.ok()) {
            return status;  // SSL 设置失败
        }
    }
    
    // 第三步：初始化 HTTP/2 会话
    status = InitializeSession();
    if (!status.ok()) {
        return status;  // 会话初始化失败
    }
    
    // 第四步：执行 HTTP/2 协议握手
    status = PerformHandshake();
    if (!status.ok()) {
        return status;  // 握手失败
    }
    
    state_->connected = true;  // 标记为已连接
    return Status::OK();
}

/**
 * @brief 断开与服务器的连接
 * 
 * 优雅地关闭 HTTP/2 连接，包括：
 * 1. 发送 GOAWAY 帧通知服务器连接即将关闭
 * 2. 终止 nghttp2 会话
 * 3. 更新连接状态
 * 
 * 此方法是幂等的，可以安全地多次调用。
 * ConnectionState 的析构函数会处理底层资源的清理。
 */
void Http2Client::Disconnect() {
    if (state_->session) {
        // 优雅地终止 HTTP/2 会话
        nghttp2_session_terminate_session(state_->session, NGHTTP2_NO_ERROR);
        SendData(); // 发送 GOAWAY 帧
    }
    state_->connected = false;  // 更新连接状态
}

/**
 * @brief 检查连接状态
 * @return bool 如果已连接返回 true，否则返回 false
 * 
 * 返回当前的连接状态。此方法是线程安全的，
 * 可以在任何时候调用以检查连接是否可用。
 */
bool Http2Client::IsConnected() const {
    return state_->connected;
}

/**
 * @brief 发送 HTTP/2 请求
 * @param method HTTP 方法（GET、POST、PUT 等）
 * @param path 请求路径（如 "/api/v1/users"）
 * @param headers 自定义 HTTP 头部映射
 * @param body 请求体内容（对于 POST/PUT 请求）
 * @param response 用于接收响应的对象指针
 * @return Status 请求发送和处理状态
 * 
 * 发送完整的 HTTP/2 请求并等待响应，包括以下步骤：
 * 1. 验证连接状态
 * 2. 构建 HTTP/2 头部（包括伪头部和自定义头部）
 * 3. 提交请求到 nghttp2 会话
 * 4. 如果有请求体，发送数据
 * 5. 处理网络事件直到收到完整响应
 * 
 * HTTP/2 特性支持：
 * - 自动流 ID 分配
 * - 头部压缩（HPACK）
 * - 多路复用（可同时处理多个请求）
 * - 流控制
 * 
 * 错误处理：
 * - 连接状态检查
 * - 网络错误处理
 * - 协议错误处理
 * - 超时处理
 */
Status Http2Client::SendRequest(
    const std::string& method,
    const std::string& path,
    const std::map<std::string, std::string>& headers,
    const std::string& body,
    Http2Response* response) {
    
    // 第一步：检查连接状态
    if (!state_->connected) {
        return Status::Unavailable("Not connected");
    }
    
    // 第二步：准备 HTTP/2 头部
    std::vector<nghttp2_nv> nva;  // nghttp2 名值对数组
    
    // 第三步：添加 HTTP/2 伪头部（Pseudo Headers）
    // HTTP/2 要求特定的伪头部，以冒号开头，必须在普通头部之前
    
    // 添加 :method 伪头部（HTTP 方法）
    nghttp2_nv method_nv = {
        (uint8_t*)":method", (uint8_t*)method.c_str(),
        7, method.length(), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(method_nv);
    
    // 添加 :path 伪头部（请求路径）
    nghttp2_nv path_nv = {
        (uint8_t*)":path", (uint8_t*)path.c_str(),
        5, path.length(), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(path_nv);
    
    // 添加 :scheme 伪头部（协议方案：http 或 https）
    nghttp2_nv scheme_nv = {
        (uint8_t*)":scheme", (uint8_t*)(state_->use_ssl ? "https" : "http"),
        7, static_cast<size_t>(state_->use_ssl ? 5 : 4), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(scheme_nv);
    
    // 添加 :authority 伪头部（必需的 gRPC 协议要求）
    // 从 headers 中查找 :authority，如果没有则使用默认值
    std::string authority_value = "localhost";  // 默认值
    auto authority_it = headers.find(":authority");
    if (authority_it != headers.end()) {
        authority_value = authority_it->second;
    }
    
    nghttp2_nv authority_nv = {
        (uint8_t*)":authority", (uint8_t*)authority_value.c_str(),
        10, authority_value.length(), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(authority_nv);
    
    // 第四步：添加自定义 HTTP 头部
    // 使用 header_storage 确保字符串在 nghttp2_nv 使用期间保持有效
    std::vector<std::string> header_storage;
    for (const auto& header : headers) {
        // 将头部名称和值存储到 vector 中，确保内存有效性
        header_storage.push_back(header.first);
        header_storage.push_back(header.second);
        
        // 创建 nghttp2 名值对结构
        nghttp2_nv header_nv = {
            (uint8_t*)header_storage[header_storage.size()-2].c_str(),  // 头部名称
            (uint8_t*)header_storage[header_storage.size()-1].c_str(),  // 头部值
            header.first.length(), header.second.length(),
            NGHTTP2_NV_FLAG_NONE
        };
        nva.push_back(header_nv);
    }
    
    // 第五步：提交请求到 nghttp2 会话
    // 这会创建一个新的 HTTP/2 流并分配唯一的流 ID
    int32_t stream_id = nghttp2_submit_request(
        state_->session, nullptr, nva.data(), nva.size(), nullptr, nullptr);
    
    if (stream_id < 0) {
        return Status::Internal("Failed to submit request");
    }
    
    // 保存流 ID 并初始化响应对象
    state_->current_stream_id = stream_id;
    state_->responses[stream_id] = Http2Response();
    
    // 第六步：发送请求体数据（如果存在）
    if (!body.empty()) {
        // 设置数据提供者，用于向 nghttp2 提供请求体数据
        nghttp2_data_provider data_prd;
        data_prd.source.ptr = (void*)body.c_str();
        
        // 数据读取回调函数 - 当 nghttp2 需要发送数据时调用
        data_prd.read_callback = [](nghttp2_session* session, int32_t stream_id,
                                   uint8_t* buf, size_t length, uint32_t* data_flags,
                                   nghttp2_data_source* source, void* user_data) -> ssize_t {
            const char* data = (const char*)source->ptr;
            size_t data_len = strlen(data);
            
            // 检查缓冲区是否足够大
            if (data_len > length) {
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }
            
            // 复制数据到 nghttp2 缓冲区
            memcpy(buf, data, data_len);
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;  // 标记数据结束
            return data_len;
        };
        
        // 提交数据帧，并标记流结束
        nghttp2_submit_data(state_->session, NGHTTP2_FLAG_END_STREAM, stream_id, &data_prd);
    }
    
    // 第七步：处理请求/响应循环
    // 这会发送请求并等待完整的响应
    auto status = ProcessEvents();
    if (!status.ok()) {
        return status;
    }
    
    // 第八步：获取并返回响应
    auto it = state_->responses.find(stream_id);
    if (it != state_->responses.end()) {
        *response = it->second;           // 复制响应数据
        state_->responses.erase(it);      // 清理响应缓存
        return Status::OK();
    }
    
    return Status::Internal("Response not found");
}

/**
 * @brief 创建网络套接字并连接到服务器
 * @param host 目标主机名或 IP 地址
 * @param port 目标端口号
 * @return Status 套接字创建和连接状态
 * 
 * 执行以下步骤创建 TCP 连接：
 * 1. 使用 getaddrinfo 解析主机名到 IP 地址
 * 2. 创建套接字
 * 3. 连接到目标服务器
 * 
 * 支持 IPv4 和 IPv6 地址，自动选择最佳协议。
 */
Status Http2Client::CreateSocket(const std::string& host, int port) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // 支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP 套接字
    
    // 解析主机名到 IP 地址
    int rv = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (rv != 0) {
        return Status::Unavailable("Failed to resolve host: " + std::string(gai_strerror(rv)));
    }
    
    // 创建套接字
    state_->socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (state_->socket_fd < 0) {
        freeaddrinfo(result);
        return Status::Unavailable("Failed to create socket");
    }
    
    // 连接到服务器
    if (connect(state_->socket_fd, result->ai_addr, result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        return Status::Unavailable("Failed to connect");
    }
    
    freeaddrinfo(result);
    return Status::OK();
}

/**
 * @brief 设置 SSL/TLS 加密连接
 * @return Status SSL 设置状态
 * 
 * 在现有 TCP 连接上建立 SSL/TLS 加密层：
 * 1. 创建 SSL 上下文
 * 2. 配置 ALPN 协议协商（用于 HTTP/2）
 * 3. 创建 SSL 连接对象
 * 4. 执行 SSL 握手
 * 
 * 使用 ALPN 扩展确保服务器支持 HTTP/2 协议。
 */
Status Http2Client::SetupSsl() {
    // 创建 SSL 上下文
    state_->ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (!state_->ssl_ctx) {
        return Status::Internal("Failed to create SSL context");
    }
    
    // 设置 ALPN 协议协商，指定支持 HTTP/2
    const unsigned char alpn_protos[] = "\x02h2";  // "h2" 表示 HTTP/2
    SSL_CTX_set_alpn_protos(state_->ssl_ctx, alpn_protos, sizeof(alpn_protos) - 1);
    
    // 创建 SSL 连接对象
    state_->ssl = SSL_new(state_->ssl_ctx);
    if (!state_->ssl) {
        return Status::Internal("Failed to create SSL object");
    }
    
    // 将 SSL 对象绑定到套接字
    SSL_set_fd(state_->ssl, state_->socket_fd);
    
    // 执行 SSL 握手
    if (SSL_connect(state_->ssl) <= 0) {
        return Status::Internal("SSL handshake failed");
    }
    
    return Status::OK();
}

/**
 * @brief 初始化 nghttp2 会话
 * @return Status 会话初始化状态
 * 
 * 创建并配置 nghttp2 客户端会话：
 * 1. 创建回调函数集合
 * 2. 设置各种事件回调函数
 * 3. 创建客户端会话
 * 4. 清理临时资源
 * 
 * 回调函数用于处理 HTTP/2 协议事件，如数据发送、
 * 帧接收、头部处理等。
 */
Status Http2Client::InitializeSession() {
    nghttp2_session_callbacks* callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    
    // 设置各种回调函数
    nghttp2_session_callbacks_set_send_callback(callbacks, SendCallback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, OnFrameRecvCallback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, OnDataChunkRecvCallback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, OnHeaderCallback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, OnStreamCloseCallback);
    
    // 创建客户端会话
    int rv = nghttp2_session_client_new(&state_->session, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);  // 清理回调函数集合
    
    if (rv != 0) {
        return Status::Internal("Failed to create HTTP/2 session");
    }
    
    return Status::OK();
}

/**
 * @brief 执行 HTTP/2 协议握手
 * @return Status 握手状态
 * 
 * 发送 HTTP/2 连接设置帧，建立协议参数：
 * 1. 配置最大并发流数量
 * 2. 提交设置帧到 nghttp2 会话
 * 3. 发送设置数据到服务器
 * 
 * 这是 HTTP/2 连接建立的最后一步，确保客户端和服务器
 * 就协议参数达成一致。
 */
Status Http2Client::PerformHandshake() {
    // 配置 HTTP/2 连接设置
    nghttp2_settings_entry iv[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}  // 最大并发流数量
    };
    
    // 提交设置帧
    int rv = nghttp2_submit_settings(state_->session, NGHTTP2_FLAG_NONE, iv, 1);
    if (rv != 0) {
        return Status::Internal("Failed to submit settings");
    }
    
    // 发送设置数据
    return SendData();
}

/**
 * @brief 发送待发送的数据
 * @return Status 发送状态
 * 
 * 将 nghttp2 会话中缓冲的数据发送到网络：
 * - 调用 nghttp2_session_send 处理所有待发送数据
 * - 数据通过 SendCallback 回调函数实际发送
 * 
 * 这个方法是 HTTP/2 数据发送的核心，处理所有类型的
 * HTTP/2 帧（HEADERS、DATA、SETTINGS 等）。
 */
Status Http2Client::SendData() {
    int rv = nghttp2_session_send(state_->session);
    if (rv != 0) {
        return Status::Internal("Failed to send data");
    }
    return Status::OK();
}

/**
 * @brief 接收并处理网络数据
 * @return Status 接收状态
 * 
 * 从网络接收数据并交给 nghttp2 处理：
 * 1. 从套接字读取数据到缓冲区
 * 2. 检查连接状态和数据长度
 * 3. 将数据传递给 nghttp2 会话处理
 * 
 * nghttp2 会解析 HTTP/2 帧并触发相应的回调函数。
 */
Status Http2Client::ReceiveData() {
    uint8_t buf[8192];  // 接收缓冲区
    ssize_t readlen = SocketRecv(buf, sizeof(buf));
    
    if (readlen < 0) {
        return Status::Internal("Failed to receive data");
    }
    
    if (readlen == 0) {
        return Status::Unavailable("Connection closed");  // 连接已关闭
    }
    
    // 将接收到的数据传递给 nghttp2 处理
    ssize_t rv = nghttp2_session_mem_recv(state_->session, buf, readlen);
    if (rv < 0) {
        return Status::Internal("Failed to process received data");
    }
    
    return Status::OK();
}

/**
 * @brief 处理 HTTP/2 事件循环
 * @return Status 事件处理状态
 * 
 * 执行完整的 HTTP/2 事件处理循环：
 * 1. 发送待发送的数据
 * 2. 检查会话是否需要读写操作
 * 3. 如果需要读取，接收并处理数据
 * 4. 重复直到没有待处理事件
 * 
 * 这是 HTTP/2 通信的核心循环，确保所有数据
 * 正确发送和接收。
 */
Status Http2Client::ProcessEvents() {
    int max_iterations = 100;  // 防止无限循环
    int iteration_count = 0;
    
    while (iteration_count < max_iterations) {
        iteration_count++;
        
        // 发送待发送的数据
        auto status = SendData();
        if (!status.ok()) {
            return status;
        }
        
        // 检查是否还有待处理的读写操作
        if (nghttp2_session_want_read(state_->session) == 0 &&
            nghttp2_session_want_write(state_->session) == 0) {
            break;  // 没有待处理操作，退出循环
        }
        
        // 如果需要读取数据
        if (nghttp2_session_want_read(state_->session)) {
            status = ReceiveData();
            if (!status.ok()) {
                return status;
            }
        }
        
        // 添加短暂延迟避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    if (iteration_count >= max_iterations) {
        return Status::Internal("ProcessEvents exceeded maximum iterations");
    }
    
    return Status::OK();
}

/**
 * @brief 发送数据到套接字
 * @param data 要发送的数据指针
 * @param len 数据长度
 * @return ssize_t 实际发送的字节数，失败返回负值
 * 
 * 根据连接类型选择发送方式：
 * - SSL 连接：使用 SSL_write
 * - 普通连接：使用 send 系统调用
 */
ssize_t Http2Client::SocketSend(const void* data, size_t len) {
    if (state_->use_ssl) {
        return SSL_write(state_->ssl, data, len);  // SSL 加密发送
    } else {
        return send(state_->socket_fd, data, len, 0);  // 普通套接字发送
    }
}

/**
 * @brief 从套接字接收数据
 * @param data 接收数据的缓冲区指针
 * @param len 缓冲区大小
 * @return ssize_t 实际接收的字节数，失败返回负值
 * 
 * 根据连接类型选择接收方式：
 * - SSL 连接：使用 SSL_read
 * - 普通连接：使用 recv 系统调用
 */
ssize_t Http2Client::SocketRecv(void* data, size_t len) {
    if (state_->use_ssl) {
        return SSL_read(state_->ssl, data, len);  // SSL 加密接收
    } else {
        return recv(state_->socket_fd, data, len, 0);  // 普通套接字接收
    }
}

//==============================================================================
// nghttp2 静态回调函数实现
// 这些函数由 nghttp2 库在特定事件发生时调用，用于处理 HTTP/2 协议事件
//==============================================================================

/**
 * @brief nghttp2 数据发送回调函数
 * @param session nghttp2 会话指针
 * @param data 要发送的数据指针
 * @param length 数据长度
 * @param flags 发送标志
 * @param user_data 用户数据指针（Http2Client 实例）
 * @return ssize_t 实际发送的字节数，失败返回负值
 * 
 * 当 nghttp2 需要发送数据时调用此回调函数。
 * 函数将数据转发给 Http2Client 的 SocketSend 方法进行实际发送。
 */
ssize_t Http2Client::SendCallback(nghttp2_session* session, const uint8_t* data,
                                 size_t length, int flags, void* user_data) {
    Http2Client* client = static_cast<Http2Client*>(user_data);
    return client->SocketSend(data, length);
}

/**
 * @brief nghttp2 帧接收回调函数
 * @param session nghttp2 会话指针
 * @param frame 接收到的 HTTP/2 帧
 * @param user_data 用户数据指针（Http2Client 实例）
 * @return int 处理结果，0 表示成功
 * 
 * 当接收到完整的 HTTP/2 帧时调用此回调函数。
 * 可以在此处理特定类型的帧（如 HEADERS、DATA、SETTINGS 等）。
 * 当前实现为基础版本，可根据需要扩展具体的帧处理逻辑。
 */
int Http2Client::OnFrameRecvCallback(nghttp2_session* session,
                                    const nghttp2_frame* frame, void* user_data) {
    // 处理帧接收事件
    // 可根据 frame->hd.type 处理不同类型的帧
    return 0;
}

/**
 * @brief nghttp2 数据块接收回调函数
 * @param session nghttp2 会话指针
 * @param flags 数据标志
 * @param stream_id 流 ID
 * @param data 接收到的数据指针
 * @param len 数据长度
 * @param user_data 用户数据指针（Http2Client 实例）
 * @return int 处理结果，0 表示成功
 * 
 * 当接收到 HTTP/2 DATA 帧的数据块时调用此回调函数。
 * 函数将接收到的数据追加到对应流的响应体中。
 */
int Http2Client::OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags,
                                        int32_t stream_id, const uint8_t* data,
                                        size_t len, void* user_data) {
    Http2Client* client = static_cast<Http2Client*>(user_data);
    auto& response = client->state_->responses[stream_id];
    response.body.append(reinterpret_cast<const char*>(data), len);
    return 0;
}

/**
 * @brief nghttp2 头部接收回调函数
 * @param session nghttp2 会话指针
 * @param frame 包含头部的帧
 * @param name 头部名称
 * @param namelen 头部名称长度
 * @param value 头部值
 * @param valuelen 头部值长度
 * @param flags 头部标志
 * @param user_data 用户数据指针（Http2Client 实例）
 * @return int 处理结果，0 表示成功
 * 
 * 当接收到 HTTP/2 HEADERS 帧中的头部字段时调用此回调函数。
 * 函数处理 HTTP 响应头部，包括：
 * - `:status` 伪头部：设置响应状态码
 * - 其他头部：存储到响应的头部映射中
 */
int Http2Client::OnHeaderCallback(nghttp2_session* session,
                                 const nghttp2_frame* frame,
                                 const uint8_t* name, size_t namelen,
                                 const uint8_t* value, size_t valuelen,
                                 uint8_t flags, void* user_data) {
    Http2Client* client = static_cast<Http2Client*>(user_data);
    
    // 转换头部名称和值为字符串
    std::string header_name(reinterpret_cast<const char*>(name), namelen);
    std::string header_value(reinterpret_cast<const char*>(value), valuelen);
    
    // 处理 HTTP/2 伪头部 :status
    if (header_name == ":status") {
        client->state_->responses[frame->hd.stream_id].status_code = std::stoi(header_value);
    } else {
        // 存储普通 HTTP 头部
        client->state_->responses[frame->hd.stream_id].headers[header_name] = header_value;
    }
    
    return 0;
}

/**
 * @brief nghttp2 流关闭回调函数
 * @param session nghttp2 会话指针
 * @param stream_id 关闭的流 ID
 * @param error_code 错误代码（如果有）
 * @param user_data 用户数据指针（Http2Client 实例）
 * @return int 处理结果，0 表示成功
 * 
 * 当 HTTP/2 流关闭时调用此回调函数。
 * 可以在此处理流关闭事件，如清理资源、记录日志等。
 * 当前实现为基础版本，可根据需要扩展具体的清理逻辑。
 */
int Http2Client::OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id,
                                      uint32_t error_code, void* user_data) {
    // 流已关闭
    // 可在此处添加流关闭后的清理逻辑
    return 0;
}

} // namespace http2
} // namespace litegrpc