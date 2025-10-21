#ifndef LITEGRPC_CORE_H
#define LITEGRPC_CORE_H

#include <string>
#include <memory>
#include <map>
#include <vector>

namespace litegrpc {

// Core configuration
struct Config {
    static constexpr int DEFAULT_TIMEOUT_MS = 30000;
    static constexpr int DEFAULT_MAX_MESSAGE_SIZE = 4 * 1024 * 1024; // 4MB
    static constexpr const char* DEFAULT_USER_AGENT = "LiteGRPC/1.0";
};

// Forward declarations
class Channel;
class ClientContext;
class ChannelCredentials;
class ChannelArguments;
class StubInterface;

// SSL credentials options
struct SslCredentialsOptions {
    std::string pem_root_certs;
    std::string pem_private_key;
    std::string pem_cert_chain;
};

} // namespace litegrpc

#endif // LITEGRPC_CORE_H