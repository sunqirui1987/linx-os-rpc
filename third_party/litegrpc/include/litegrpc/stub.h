#ifndef LITEGRPC_STUB_H
#define LITEGRPC_STUB_H

#include <memory>
#include "litegrpc/core.h"
#include "litegrpc/status.h"
#include "litegrpc/channel.h"

namespace litegrpc {

// Forward declarations
class ClientContext;

class StubInterface {
public:
    virtual ~StubInterface() = default;
    
protected:
    explicit StubInterface(std::shared_ptr<Channel> channel) : channel_(channel) {}
    
    // Helper method for making RPC calls
    Status MakeCall(
        const std::string& method,
        ClientContext* context,
        const std::string& request_data,
        std::string* response_data);
    
    std::shared_ptr<Channel> channel_;
};

} // namespace litegrpc

#endif // LITEGRPC_STUB_H