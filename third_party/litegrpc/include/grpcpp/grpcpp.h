#pragma once

/**
 * gRPC++ compatibility header for LiteGRPC
 * This file provides a drop-in replacement for grpcpp/grpcpp.h
 */

// Include the main LiteGRPC header
#include "litegrpc/litegrpc.h"

// Additional gRPC compatibility APIs
namespace grpc {
    // Type aliases for compatibility
    using Channel = litegrpc::Channel;
    using ChannelArguments = litegrpc::ChannelArguments;
    using ChannelCredentials = litegrpc::ChannelCredentials;
    using ClientContext = litegrpc::ClientContext;
    using Status = litegrpc::Status;
    using StatusCode = litegrpc::StatusCode;
    using SslCredentialsOptions = litegrpc::SslCredentialsOptions;
    
    // Factory functions with gRPC-compatible signatures
    std::shared_ptr<Channel> CreateCustomChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds,
        const ChannelArguments& args);
    
    // Credentials factory functions
    std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options);
    std::shared_ptr<ChannelCredentials> InsecureChannelCredentials();
}