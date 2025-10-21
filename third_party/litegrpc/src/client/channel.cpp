#include "litegrpc/channel.h"
#include "litegrpc/client_context.h"
#include "../http2/http2_client.h"
#include <regex>
#include <sstream>
#include <arpa/inet.h>
#include <cstring>

namespace litegrpc {

struct LiteGrpcChannel::Http2Connection {
    std::unique_ptr<http2::Http2Client> client;
    std::string host;
    int port;
    bool use_ssl;
    
    Http2Connection() : client(std::make_unique<http2::Http2Client>()) {}
};

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

LiteGrpcChannel::~LiteGrpcChannel() {
    Disconnect();
}

bool LiteGrpcChannel::IsConnected() const {
    return connected_ && connection_->client->IsConnected();
}

Status LiteGrpcChannel::Connect() {
    if (connected_) {
        return Status::OK();
    }
    
    std::string host;
    int port;
    bool use_ssl;
    
    auto status = ParseTarget(target_, &host, &port, &use_ssl);
    if (!status.ok()) {
        return status;
    }
    
    connection_->host = host;
    connection_->port = port;
    connection_->use_ssl = use_ssl;
    
    status = connection_->client->Connect(host, port, use_ssl);
    if (!status.ok()) {
        return status;
    }
    
    connected_ = true;
    return Status::OK();
}

void LiteGrpcChannel::Disconnect() {
    if (connection_->client) {
        connection_->client->Disconnect();
    }
    connected_ = false;
}

Status LiteGrpcChannel::ExecuteRequest(
    const std::string& method,
    ClientContext* context,
    const std::string& request_data,
    std::string* response_data) {
    
    if (!IsConnected()) {
        auto status = Connect();
        if (!status.ok()) {
            return status;
        }
    }
    
    // Check for timeout
    if (context && context->IsExpired()) {
        return Status::DeadlineExceeded("Request deadline exceeded");
    }
    
    // Prepare headers
    std::map<std::string, std::string> headers;
    headers["content-type"] = "application/grpc+proto";
    headers["te"] = "trailers";
    headers["user-agent"] = Config::DEFAULT_USER_AGENT;
    
    // Add custom metadata
    if (context) {
        for (const auto& metadata : context->GetMetadata()) {
            headers[metadata.first] = metadata.second;
        }
        
        if (!context->authority().empty()) {
            headers[":authority"] = context->authority();
        }
        
        if (!context->user_agent_prefix().empty()) {
            headers["user-agent"] = context->user_agent_prefix() + " " + Config::DEFAULT_USER_AGENT;
        }
    }
    
    // Prepare gRPC message format
    std::string grpc_message;
    grpc_message.resize(5 + request_data.size());
    
    // gRPC message format: [compressed flag (1 byte)] + [length (4 bytes)] + [data]
    grpc_message[0] = 0; // Not compressed
    uint32_t length = htonl(static_cast<uint32_t>(request_data.size()));
    memcpy(&grpc_message[1], &length, 4);
    memcpy(&grpc_message[5], request_data.data(), request_data.size());
    
    // Send HTTP/2 request
    http2::Http2Response response;
    auto status = connection_->client->SendRequest(
        "POST", method, headers, grpc_message, &response);
    
    if (!status.ok()) {
        return status;
    }
    
    // Check HTTP status
    if (response.status_code != 200) {
        return Status::Internal("HTTP error: " + std::to_string(response.status_code));
    }
    
    // Parse gRPC response
    if (response.body.size() < 5) {
        return Status::Internal("Invalid gRPC response format");
    }
    
    // Skip gRPC header (5 bytes) and extract protobuf data
    *response_data = response.body.substr(5);
    
    // Check for gRPC status in trailers
    auto grpc_status_it = response.headers.find("grpc-status");
    if (grpc_status_it != response.headers.end()) {
        int grpc_status = std::stoi(grpc_status_it->second);
        if (grpc_status != 0) {
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

std::shared_ptr<Channel> CreateCustomChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds,
    const ChannelArguments& args) {
    
    return litegrpc::CreateCustomChannel(target, creds, args);
}

} // namespace grpc