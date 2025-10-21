#include "litegrpc/status.h"
#include <sstream>

namespace litegrpc {

std::string Status::ToString() const {
    if (ok()) {
        return "OK";
    }
    
    std::ostringstream oss;
    oss << "Status(";
    
    switch (code_) {
        case StatusCode::CANCELLED:
            oss << "CANCELLED";
            break;
        case StatusCode::UNKNOWN:
            oss << "UNKNOWN";
            break;
        case StatusCode::INVALID_ARGUMENT:
            oss << "INVALID_ARGUMENT";
            break;
        case StatusCode::DEADLINE_EXCEEDED:
            oss << "DEADLINE_EXCEEDED";
            break;
        case StatusCode::NOT_FOUND:
            oss << "NOT_FOUND";
            break;
        case StatusCode::ALREADY_EXISTS:
            oss << "ALREADY_EXISTS";
            break;
        case StatusCode::PERMISSION_DENIED:
            oss << "PERMISSION_DENIED";
            break;
        case StatusCode::RESOURCE_EXHAUSTED:
            oss << "RESOURCE_EXHAUSTED";
            break;
        case StatusCode::FAILED_PRECONDITION:
            oss << "FAILED_PRECONDITION";
            break;
        case StatusCode::ABORTED:
            oss << "ABORTED";
            break;
        case StatusCode::OUT_OF_RANGE:
            oss << "OUT_OF_RANGE";
            break;
        case StatusCode::UNIMPLEMENTED:
            oss << "UNIMPLEMENTED";
            break;
        case StatusCode::INTERNAL:
            oss << "INTERNAL";
            break;
        case StatusCode::UNAVAILABLE:
            oss << "UNAVAILABLE";
            break;
        case StatusCode::DATA_LOSS:
            oss << "DATA_LOSS";
            break;
        case StatusCode::UNAUTHENTICATED:
            oss << "UNAUTHENTICATED";
            break;
        default:
            oss << "UNKNOWN_CODE(" << static_cast<int>(code_) << ")";
            break;
    }
    
    if (!message_.empty()) {
        oss << ", \"" << message_ << "\"";
    }
    
    oss << ")";
    return oss.str();
}

} // namespace litegrpc