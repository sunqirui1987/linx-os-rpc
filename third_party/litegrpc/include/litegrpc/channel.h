#ifndef LITEGRPC_CHANNEL_H
#define LITEGRPC_CHANNEL_H

#include <string>
#include <memory>
#include <chrono>
#include "litegrpc/core.h"
#include "litegrpc/status.h"
#include "litegrpc/credentials.h"

namespace litegrpc {

// Forward declarations
class ClientContext;

class Channel {
public:
    virtual ~Channel() = default;
    
    // Connection management
    virtual bool IsConnected() const = 0;
    virtual Status Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool WaitForConnected(std::chrono::system_clock::time_point deadline) = 0;
    
    // Request execution
    virtual Status ExecuteRequest(
        const std::string& method,
        ClientContext* context,
        const std::string& request_data,
        std::string* response_data) = 0;
    
    // Channel information
    virtual std::string GetTarget() const = 0;
    virtual std::shared_ptr<ChannelCredentials> GetCredentials() const = 0;
    virtual const ChannelArguments& GetArguments() const = 0;
};

class LiteGrpcChannel : public Channel {
public:
    LiteGrpcChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> credentials,
        const ChannelArguments& args);
    
    ~LiteGrpcChannel() override;
    
    // Channel interface implementation
    bool IsConnected() const override;
    Status Connect() override;
    void Disconnect() override;
    bool WaitForConnected(std::chrono::system_clock::time_point deadline) override;
    
    Status ExecuteRequest(
        const std::string& method,
        ClientContext* context,
        const std::string& request_data,
        std::string* response_data) override;
    
    // Protobuf message call method
    template<typename RequestType, typename ResponseType>
    Status CallMethod(const std::string& method,
                     ClientContext& context,
                     const RequestType& request,
                     ResponseType* response) {
        // Serialize request
        std::string request_data;
        if (!request.SerializeToString(&request_data)) {
            return Status::Internal("Failed to serialize request");
        }
        
        // Execute request
        std::string response_data;
        auto status = ExecuteRequest(method, &context, request_data, &response_data);
        if (!status.ok()) {
            return status;
        }
        
        // Deserialize response
        if (!response->ParseFromString(response_data)) {
            return Status::Internal("Failed to parse response");
        }
        
        return Status::OK();
    }
    
    std::string GetTarget() const override { return target_; }
    std::shared_ptr<ChannelCredentials> GetCredentials() const override { return credentials_; }
    const ChannelArguments& GetArguments() const override { return args_; }
    
private:
    std::string target_;
    std::shared_ptr<ChannelCredentials> credentials_;
    ChannelArguments args_;
    bool connected_;
    
    // HTTP/2 connection state
    struct Http2Connection;
    std::unique_ptr<Http2Connection> connection_;
    
    // Internal methods
    Status ParseTarget(const std::string& target, std::string* host, int* port, bool* use_ssl);
    Status EstablishConnection();
    Status SendHttp2Request(
        const std::string& method,
        const std::map<std::string, std::string>& headers,
        const std::string& body,
        std::string* response_body,
        std::map<std::string, std::string>* response_headers);
};

// Factory functions
std::shared_ptr<Channel> CreateChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds);

std::shared_ptr<Channel> CreateCustomChannel(
    const std::string& target,
    std::shared_ptr<ChannelCredentials> creds,
    const ChannelArguments& args);

} // namespace litegrpc

#endif // LITEGRPC_CHANNEL_H