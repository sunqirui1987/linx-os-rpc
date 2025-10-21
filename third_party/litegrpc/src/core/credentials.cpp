#include "litegrpc/credentials.h"

namespace litegrpc {

// ChannelArguments implementation
const std::string ChannelArguments::GRPC_ARG_KEEPALIVE_TIME_MS = "grpc.keepalive_time_ms";
const std::string ChannelArguments::GRPC_ARG_KEEPALIVE_TIMEOUT_MS = "grpc.keepalive_timeout_ms";
const std::string ChannelArguments::GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS = "grpc.keepalive_permit_without_calls";
const std::string ChannelArguments::GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA = "grpc.http2.max_pings_without_data";
const std::string ChannelArguments::GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS = "grpc.http2.min_sent_ping_interval_without_data_ms";
const std::string ChannelArguments::GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS = "grpc.http2.min_recv_ping_interval_without_data_ms";
const std::string ChannelArguments::GRPC_ARG_MAX_CONNECTION_IDLE_MS = "grpc.max_connection_idle_ms";
const std::string ChannelArguments::GRPC_ARG_MAX_CONNECTION_AGE_MS = "grpc.max_connection_age_ms";
const std::string ChannelArguments::GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS = "grpc.max_connection_age_grace_ms";

void ChannelArguments::SetInt(const std::string& key, int value) {
    int_args_[key] = value;
}

void ChannelArguments::SetString(const std::string& key, const std::string& value) {
    string_args_[key] = value;
}

void ChannelArguments::SetPointer(const std::string& key, void* value) {
    pointer_args_[key] = value;
}

bool ChannelArguments::GetInt(const std::string& key, int* value) const {
    auto it = int_args_.find(key);
    if (it != int_args_.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

bool ChannelArguments::GetString(const std::string& key, std::string* value) const {
    auto it = string_args_.find(key);
    if (it != string_args_.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

bool ChannelArguments::GetPointer(const std::string& key, void** value) const {
    auto it = pointer_args_.find(key);
    if (it != pointer_args_.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

// Factory functions
std::shared_ptr<ChannelCredentials> InsecureChannelCredentials() {
    return std::make_shared<InsecureChannelCredentialsImpl>();
}

std::shared_ptr<ChannelCredentials> SslCredentials(const SslCredentialsOptions& options) {
    return std::make_shared<SslChannelCredentialsImpl>(options);
}

} // namespace litegrpc