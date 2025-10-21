#include "litegrpc/client_context.h"
#include <chrono>

namespace litegrpc {

void ClientContext::AddMetadata(const std::string& key, const std::string& value) {
    metadata_[key] = value;
}

const std::map<std::string, std::string>& ClientContext::GetMetadata() const {
    return metadata_;
}

void ClientContext::set_deadline(const std::chrono::system_clock::time_point& deadline) {
    deadline_ = deadline;
    has_deadline_ = true;
}

std::chrono::system_clock::time_point ClientContext::deadline() const {
    return deadline_;
}

bool ClientContext::has_deadline() const {
    return has_deadline_;
}

void ClientContext::set_authority(const std::string& authority) {
    authority_ = authority;
}

const std::string& ClientContext::authority() const {
    return authority_;
}

void ClientContext::set_compression_algorithm(const std::string& algorithm) {
    compression_algorithm_ = algorithm;
}

const std::string& ClientContext::compression_algorithm() const {
    return compression_algorithm_;
}

void ClientContext::set_user_agent_prefix(const std::string& user_agent_prefix) {
    user_agent_prefix_ = user_agent_prefix;
}

const std::string& ClientContext::user_agent_prefix() const {
    return user_agent_prefix_;
}

void ClientContext::Reset() {
    metadata_.clear();
    has_deadline_ = false;
    authority_.clear();
    compression_algorithm_.clear();
    user_agent_prefix_.clear();
}

bool ClientContext::IsExpired() const {
    if (!has_deadline_) {
        return false;
    }
    return std::chrono::system_clock::now() > deadline_;
}

int ClientContext::GetTimeoutMs() const {
    if (!has_deadline_) {
        return -1; // No timeout
    }
    
    auto now = std::chrono::system_clock::now();
    if (deadline_ <= now) {
        return 0; // Already expired
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - now);
    return static_cast<int>(duration.count());
}

} // namespace litegrpc