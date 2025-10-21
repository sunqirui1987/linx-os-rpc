#ifndef LITEGRPC_CREDENTIALS_H
#define LITEGRPC_CREDENTIALS_H

#include <string>
#include <memory>
#include "litegrpc/core.h"

namespace litegrpc {

class ChannelCredentials {
public:
    virtual ~ChannelCredentials() = default;
    virtual bool IsSecure() const = 0;
    virtual std::string GetType() const = 0;
};

class InsecureChannelCredentialsImpl : public ChannelCredentials {
public:
    bool IsSecure() const override { return false; }
    std::string GetType() const override { return "insecure"; }
};

class SslChannelCredentialsImpl : public ChannelCredentials {
public:
    explicit SslChannelCredentialsImpl(const SslCredentialsOptions& options)
        : options_(options) {}
    
    bool IsSecure() const override { return true; }
    std::string GetType() const override { return "ssl"; }
    const SslCredentialsOptions& GetOptions() const { return options_; }
    
private:
    SslCredentialsOptions options_;
};

class ChannelArguments {
public:
    ChannelArguments() = default;
    
    void SetInt(const std::string& key, int value);
    void SetString(const std::string& key, const std::string& value);
    void SetPointer(const std::string& key, void* value);
    
    bool GetInt(const std::string& key, int* value) const;
    bool GetString(const std::string& key, std::string* value) const;
    bool GetPointer(const std::string& key, void** value) const;
    
    // Common argument keys
    static const std::string GRPC_ARG_KEEPALIVE_TIME_MS;
    static const std::string GRPC_ARG_KEEPALIVE_TIMEOUT_MS;
    static const std::string GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS;
    static const std::string GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA;
    static const std::string GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS;
    static const std::string GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS;
    static const std::string GRPC_ARG_MAX_CONNECTION_IDLE_MS;
    static const std::string GRPC_ARG_MAX_CONNECTION_AGE_MS;
    static const std::string GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS;
    
private:
    std::map<std::string, int> int_args_;
    std::map<std::string, std::string> string_args_;
    std::map<std::string, void*> pointer_args_;
};

// Factory functions
std::shared_ptr<ChannelCredentials> InsecureChannelCredentials();
std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options);

} // namespace litegrpc

#endif // LITEGRPC_CREDENTIALS_H