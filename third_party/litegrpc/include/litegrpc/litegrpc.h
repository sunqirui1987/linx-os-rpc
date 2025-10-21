#ifndef LITEGRPC_H
#define LITEGRPC_H

// LiteGRPC - Lightweight gRPC implementation
// Compatible with standard gRPC API for embedded systems

#include "litegrpc/core.h"
#include "litegrpc/status.h"
#include "litegrpc/channel.h"
#include "litegrpc/client_context.h"
#include "litegrpc/credentials.h"
#include "litegrpc/stub.h"

// Compatibility namespace for standard gRPC
namespace grpc {
    using Status = litegrpc::Status;
    using StatusCode = litegrpc::StatusCode;
    using ClientContext = litegrpc::ClientContext;
    using ChannelArguments = litegrpc::ChannelArguments;
    using ChannelCredentials = litegrpc::ChannelCredentials;
    using Channel = litegrpc::Channel;
    using StubInterface = litegrpc::StubInterface;
    using SslCredentialsOptions = litegrpc::SslCredentialsOptions;
    
    // Factory functions
    std::shared_ptr<Channel> CreateChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds);
    
    std::shared_ptr<Channel> CreateCustomChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds,
        const ChannelArguments& args);
    
    std::shared_ptr<ChannelCredentials> InsecureChannelCredentials();
    std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options);
}

// LinxOS Device Service compatibility
namespace linxos_device {
    // Forward declarations for protobuf messages
    class RegisterDeviceRequest;
    class RegisterDeviceResponse;
    class HeartbeatRequest;
    class HeartbeatResponse;
    class ToolCallRequest;
    class ToolCallResponse;
    
    class LinxOSDeviceService {
    public:
        class Stub : public grpc::StubInterface {
        public:
            explicit Stub(std::shared_ptr<grpc::Channel> channel);
            
            grpc::Status RegisterDevice(
                grpc::ClientContext* context,
                const RegisterDeviceRequest& request,
                RegisterDeviceResponse* response);
            
            grpc::Status Heartbeat(
                grpc::ClientContext* context,
                const HeartbeatRequest& request,
                HeartbeatResponse* response);
            
            grpc::Status CallTool(
                grpc::ClientContext* context,
                const ToolCallRequest& request,
                ToolCallResponse* response);
        
        private:
            std::shared_ptr<grpc::Channel> channel_;
        };
    };
}

#endif // LITEGRPC_H