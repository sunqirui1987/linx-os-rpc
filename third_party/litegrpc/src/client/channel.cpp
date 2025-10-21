/**
 * @file channel.cpp
 * @brief LiteGRPC 通道实现
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件实现了 LiteGRPC 的通道管理功能，包括：
 * - HTTP/2 连接管理
 * - 目标地址解析
 * - 连接状态管理
 * - RPC 请求执行
 * - 与标准 gRPC Channel 接口的兼容性
 * 
 * 主要特性：
 * - 支持安全和非安全连接
 * - 自动连接管理
 * - 超时控制
 * - 元数据传递
 * - 错误处理
 */

#include "litegrpc/channel.h"
#include "litegrpc/client_context.h"
#include "../http2/http2_client.h"
#include <regex>
#include <sstream>
#include <thread>
#include <arpa/inet.h>
#include <cstring>

namespace litegrpc {

/**
 * @brief HTTP/2 连接封装结构
 * 
 * 封装了 HTTP/2 客户端连接的相关信息，包括客户端实例、
 * 主机地址、端口号和是否使用 SSL 等配置。
 */
struct LiteGrpcChannel::Http2Connection {
    std::unique_ptr<http2::Http2Client> client;  ///< HTTP/2 客户端实例
    std::string host;                             ///< 服务器主机地址
    int port;                                     ///< 服务器端口号
    bool use_ssl;                                 ///< 是否使用 SSL/TLS 加密
    
    /**
     * @brief 构造函数
     * 初始化 HTTP/2 客户端实例
     */
    Http2Connection() : client(std::make_unique<http2::Http2Client>()) {}
};

/**
 * @brief LiteGrpcChannel 构造函数
 * @param target 目标服务器地址（格式：host:port 或 scheme://host:port）
 * @param credentials 通道凭证（用于身份验证和加密）
 * @param args 通道参数配置
 * 
 * 创建一个新的 gRPC 通道实例，但不立即建立连接。
 * 连接将在第一次 RPC 调用时或显式调用 Connect() 时建立。
 */
LiteGrpcChannel::LiteGrpcChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> credentials,
    const ChannelArguments& args)
    : target_(target)
    , credentials_(credentials)
    , args_(args)
    , connected_(false)
    , connection_(std::make_unique<Http2Connection>()) {
}

/**
 * @brief 析构函数
 * 
 * 自动断开连接并清理资源。
 */
LiteGrpcChannel::~LiteGrpcChannel() {
    Disconnect();
}

/**
 * @brief 检查通道是否已连接
 * @return 如果通道已连接返回 true，否则返回 false
 * 
 * 检查通道的连接状态，包括内部连接标志和底层 HTTP/2 客户端的连接状态。
 */
bool LiteGrpcChannel::IsConnected() const {
    return connected_ && connection_->client->IsConnected();
}

/**
 * @brief 建立到服务器的连接
 * @return 连接状态，成功返回 Status::OK()
 * 
 * 解析目标地址并建立 HTTP/2 连接。如果已经连接，则直接返回成功。
 * 连接过程包括：
 * 1. 解析目标地址（主机、端口、SSL 配置）
 * 2. 配置连接参数
 * 3. 建立底层 HTTP/2 连接
 */
Status LiteGrpcChannel::Connect() {
    // 如果已经连接，直接返回成功
    if (connected_) {
        return Status::OK();
    }
    
    std::string host;
    int port;
    bool use_ssl;
    
    // 解析目标地址
    auto status = ParseTarget(target_, &host, &port, &use_ssl);
    if (!status.ok()) {
        return status;
    }
    
    // 配置连接参数
    connection_->host = host;
    connection_->port = port;
    connection_->use_ssl = use_ssl;
    
    // 建立 HTTP/2 连接
    status = connection_->client->Connect(host, port, use_ssl);
    if (!status.ok()) {
        return status;
    }
    
    connected_ = true;
    return Status::OK();
}

/**
 * @brief 断开与服务器的连接
 * 
 * 关闭底层 HTTP/2 连接并重置连接状态。
 * 此方法是幂等的，可以安全地多次调用。
 */
void LiteGrpcChannel::Disconnect() {
    if (connection_->client) {
        connection_->client->Disconnect();
    }
    connected_ = false;
}

/**
 * @brief 等待连接建立（带超时）
 * @param deadline 等待截止时间
 * @return 如果在截止时间前连接成功返回 true，否则返回 false
 * 
 * 如果尚未连接，则尝试建立连接并等待连接完成。
 * 此方法会定期检查连接状态直到超时。
 */
bool LiteGrpcChannel::WaitForConnected(std::chrono::system_clock::time_point deadline) {
    // 如果已经连接，立即返回
    if (IsConnected()) {
        return true;
    }
    
    // 如果未连接，尝试建立连接
    auto status = Connect();
    if (!status.ok()) {
        return false;
    }
    
    // 等待连接完成（带超时）
    while (std::chrono::system_clock::now() < deadline) {
        if (IsConnected()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return IsConnected();
}

/**
 * @brief 执行 RPC 请求
 * @param method RPC 方法名（格式：/package.service/method）
 * @param context 客户端上下文（包含元数据、超时等信息）
 * @param request_data 序列化的请求数据
 * @param response_data 用于存储响应数据的指针
 * @return 请求执行状态
 * 
 * 执行完整的 gRPC 请求流程：
 * 1. 确保连接已建立
 * 2. 检查超时设置
 * 3. 准备 HTTP/2 头部
 * 4. 格式化 gRPC 消息
 * 5. 发送请求并接收响应
 * 6. 解析响应和状态码
 */
Status LiteGrpcChannel::ExecuteRequest(
    const std::string& method,
    ClientContext* context,
    const std::string& request_data,
    std::string* response_data) {
    
    // 确保连接已建立
    if (!IsConnected()) {
        auto status = Connect();
        if (!status.ok()) {
            return status;
        }
    }
    
    // 检查请求是否已超时
    if (context && context->IsExpired()) {
        return Status::DeadlineExceeded("Request deadline exceeded");
    }
    
    // 准备 HTTP/2 头部
    std::map<std::string, std::string> headers;
    headers["content-type"] = "application/grpc+proto";  // gRPC 内容类型
    headers["te"] = "trailers";                          // 支持 trailers
    headers["user-agent"] = Config::DEFAULT_USER_AGENT;  // 用户代理
    
    // 设置 :authority 伪头部（gRPC 协议要求）
    headers[":authority"] = connection_->host + ":" + std::to_string(connection_->port);
    
    // 添加自定义元数据
    if (context) {
        // 添加用户定义的元数据
        for (const auto& metadata : context->GetMetadata()) {
            headers[metadata.first] = metadata.second;
        }
        
        // 设置权威头部（用于虚拟主机）
        if (!context->authority().empty()) {
            headers[":authority"] = context->authority();
        }
        
        // 设置自定义用户代理前缀
        if (!context->user_agent_prefix().empty()) {
            headers["user-agent"] = context->user_agent_prefix() + " " + Config::DEFAULT_USER_AGENT;
        }
    }
    
    // 准备 gRPC 消息格式
    std::string grpc_message;
    grpc_message.resize(5 + request_data.size());
    
    // gRPC 消息格式：[压缩标志 (1字节)] + [长度 (4字节)] + [数据]
    grpc_message[0] = 0; // 未压缩
    uint32_t length = htonl(static_cast<uint32_t>(request_data.size()));
    memcpy(&grpc_message[1], &length, 4);
    memcpy(&grpc_message[5], request_data.data(), request_data.size());
    
    // 发送 HTTP/2 请求
    http2::Http2Response response;
    auto status = connection_->client->SendRequest(
        "POST", method, headers, grpc_message, &response);
    
    if (!status.ok()) {
        return status;
    }
    
    // 检查 HTTP 状态码
    if (response.status_code != 200) {
        return Status::Internal("HTTP error: " + std::to_string(response.status_code));
    }
    
    // 解析 gRPC 响应
    if (response.body.size() < 5) {
        return Status::Internal("Invalid gRPC response format");
    }
    
    // 跳过 gRPC 头部（5字节）并提取 protobuf 数据
    *response_data = response.body.substr(5);
    
    // 检查 trailers 中的 gRPC 状态码
    auto grpc_status_it = response.headers.find("grpc-status");
    if (grpc_status_it != response.headers.end()) {
        int grpc_status = std::stoi(grpc_status_it->second);
        if (grpc_status != 0) {
            // 获取错误消息
            auto grpc_message_it = response.headers.find("grpc-message");
            std::string error_message = (grpc_message_it != response.headers.end()) 
                ? grpc_message_it->second : "Unknown gRPC error";
            
            return Status(static_cast<StatusCode>(grpc_status), error_message);
        }
    }
    
    return Status::OK();
}

Status LiteGrpcChannel::ParseTarget(const std::string& target, std::string* host, int* port, bool* use_ssl) {
    // Parse target format: [scheme://]host[:port]
    std::regex target_regex(R"(^(?:([^:]+)://)?([^:]+)(?::(\d+))?$)");
    std::smatch matches;
    
    if (!std::regex_match(target, matches, target_regex)) {
        return Status::InvalidArgument("Invalid target format: " + target);
    }
    
    std::string scheme = matches[1].str();
    *host = matches[2].str();
    std::string port_str = matches[3].str();
    
    // Determine SSL usage
    if (scheme.empty()) {
        *use_ssl = credentials_->IsSecure();
    } else if (scheme == "http") {
        *use_ssl = false;
    } else if (scheme == "https") {
        *use_ssl = true;
    } else {
        return Status::InvalidArgument("Unsupported scheme: " + scheme);
    }
    
    // Determine port
    if (port_str.empty()) {
        *port = *use_ssl ? 443 : 80;
    } else {
        *port = std::stoi(port_str);
    }
    
    return Status::OK();
}

// Factory functions
std::shared_ptr<Channel> CreateChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds) {
    
    ChannelArguments args;
    return std::make_shared<LiteGrpcChannel>(target, creds, args);
}

std::shared_ptr<Channel> CreateCustomChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds,
    const ChannelArguments& args) {
    
    return std::make_shared<LiteGrpcChannel>(target, creds, args);
}

} // namespace litegrpc

// gRPC compatibility namespace implementations
namespace grpc {

std::shared_ptr<litegrpc::Channel> CreateChannel(
    const std::string& target,
    std::shared_ptr<litegrpc::ChannelCredentials> creds) {
    return litegrpc::CreateChannel(target, creds);
}

std::shared_ptr<litegrpc::Channel> CreateCustomChannel(
    const std::string& target,
    std::shared_ptr<litegrpc::ChannelCredentials> creds,
    const litegrpc::ChannelArguments& args) {
    return litegrpc::CreateCustomChannel(target, creds, args);
}

std::shared_ptr<litegrpc::ChannelCredentials> SslCredentials(const litegrpc::SslCredentialsOptions& options) {
    return litegrpc::SslCredentials(options);
}

std::shared_ptr<litegrpc::ChannelCredentials> InsecureChannelCredentials() {
    return litegrpc::InsecureChannelCredentials();
}

} // namespace grpc