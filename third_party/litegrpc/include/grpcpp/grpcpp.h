#pragma once

/**
 * gRPC++ compatibility header for LiteGRPC
 * This file provides a drop-in replacement for grpcpp/grpcpp.h
 */

// Include the main LiteGRPC header
#include "litegrpc/litegrpc.h"

// Additional gRPC compatibility APIs
namespace grpc {
    using ChannelArguments = litegrpc::ChannelArguments;
    
    // Factory functions with gRPC-compatible signatures
    std::shared_ptr<Channel> CreateCustomChannel(
        const std::string& target,
        std::shared_ptr<ChannelCredentials> creds,
        const ChannelArguments& args);
}