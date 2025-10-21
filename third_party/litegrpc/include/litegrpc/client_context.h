#ifndef LITEGRPC_CLIENT_CONTEXT_H
#define LITEGRPC_CLIENT_CONTEXT_H

#include <string>
#include <map>
#include <chrono>

namespace litegrpc {

class ClientContext {
public:
    ClientContext() = default;
    ~ClientContext() = default;
    
    // Disable copy and move
    ClientContext(const ClientContext&) = delete;
    ClientContext& operator=(const ClientContext&) = delete;
    ClientContext(ClientContext&&) = delete;
    ClientContext& operator=(ClientContext&&) = delete;
    
    // Metadata management
    void AddMetadata(const std::string& key, const std::string& value);
    const std::map<std::string, std::string>& GetMetadata() const;
    
    // Timeout management
    void set_deadline(const std::chrono::system_clock::time_point& deadline);
    std::chrono::system_clock::time_point deadline() const;
    bool has_deadline() const;
    
    // Authority (host) override
    void set_authority(const std::string& authority);
    const std::string& authority() const;
    
    // Compression
    void set_compression_algorithm(const std::string& algorithm);
    const std::string& compression_algorithm() const;
    
    // User agent
    void set_user_agent_prefix(const std::string& user_agent_prefix);
    const std::string& user_agent_prefix() const;
    
    // Internal methods for implementation
    void Reset();
    bool IsExpired() const;
    int GetTimeoutMs() const;
    
private:
    std::map<std::string, std::string> metadata_;
    std::chrono::system_clock::time_point deadline_;
    bool has_deadline_ = false;
    std::string authority_;
    std::string compression_algorithm_;
    std::string user_agent_prefix_;
};

} // namespace litegrpc

#endif // LITEGRPC_CLIENT_CONTEXT_H