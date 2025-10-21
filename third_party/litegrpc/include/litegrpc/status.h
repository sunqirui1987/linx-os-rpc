#ifndef LITEGRPC_STATUS_H
#define LITEGRPC_STATUS_H

#include <string>

namespace litegrpc {

enum class StatusCode {
    OK = 0,
    CANCELLED = 1,
    UNKNOWN = 2,
    INVALID_ARGUMENT = 3,
    DEADLINE_EXCEEDED = 4,
    NOT_FOUND = 5,
    ALREADY_EXISTS = 6,
    PERMISSION_DENIED = 7,
    RESOURCE_EXHAUSTED = 8,
    FAILED_PRECONDITION = 9,
    ABORTED = 10,
    OUT_OF_RANGE = 11,
    UNIMPLEMENTED = 12,
    INTERNAL = 13,
    UNAVAILABLE = 14,
    DATA_LOSS = 15,
    UNAUTHENTICATED = 16
};

class Status {
public:
    Status() : code_(StatusCode::OK) {}
    Status(StatusCode code, const std::string& message) 
        : code_(code), message_(message) {}
    
    static Status OK() { return Status(); }
    static Status Cancelled(const std::string& message = "") {
        return Status(StatusCode::CANCELLED, message);
    }
    static Status Unknown(const std::string& message = "") {
        return Status(StatusCode::UNKNOWN, message);
    }
    static Status InvalidArgument(const std::string& message = "") {
        return Status(StatusCode::INVALID_ARGUMENT, message);
    }
    static Status DeadlineExceeded(const std::string& message = "") {
        return Status(StatusCode::DEADLINE_EXCEEDED, message);
    }
    static Status NotFound(const std::string& message = "") {
        return Status(StatusCode::NOT_FOUND, message);
    }
    static Status AlreadyExists(const std::string& message = "") {
        return Status(StatusCode::ALREADY_EXISTS, message);
    }
    static Status PermissionDenied(const std::string& message = "") {
        return Status(StatusCode::PERMISSION_DENIED, message);
    }
    static Status ResourceExhausted(const std::string& message = "") {
        return Status(StatusCode::RESOURCE_EXHAUSTED, message);
    }
    static Status FailedPrecondition(const std::string& message = "") {
        return Status(StatusCode::FAILED_PRECONDITION, message);
    }
    static Status Aborted(const std::string& message = "") {
        return Status(StatusCode::ABORTED, message);
    }
    static Status OutOfRange(const std::string& message = "") {
        return Status(StatusCode::OUT_OF_RANGE, message);
    }
    static Status Unimplemented(const std::string& message = "") {
        return Status(StatusCode::UNIMPLEMENTED, message);
    }
    static Status Internal(const std::string& message = "") {
        return Status(StatusCode::INTERNAL, message);
    }
    static Status Unavailable(const std::string& message = "") {
        return Status(StatusCode::UNAVAILABLE, message);
    }
    static Status DataLoss(const std::string& message = "") {
        return Status(StatusCode::DATA_LOSS, message);
    }
    static Status Unauthenticated(const std::string& message = "") {
        return Status(StatusCode::UNAUTHENTICATED, message);
    }
    
    bool ok() const { return code_ == StatusCode::OK; }
    StatusCode error_code() const { return code_; }
    const std::string& error_message() const { return message_; }
    std::string ToString() const;
    
private:
    StatusCode code_;
    std::string message_;
};

} // namespace litegrpc

#endif // LITEGRPC_STATUS_H