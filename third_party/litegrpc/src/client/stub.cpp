#include "litegrpc/stub.h"
#include "litegrpc/client_context.h"

namespace litegrpc {

Status StubInterface::MakeCall(
    const std::string& method,
    ClientContext* context,
    const std::string& request_data,
    std::string* response_data) {
    
    if (!channel_) {
        return Status::Internal("Channel not available");
    }
    
    return channel_->ExecuteRequest(method, context, request_data, response_data);
}

} // namespace litegrpc