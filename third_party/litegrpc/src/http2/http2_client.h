#ifndef LITEGRPC_HTTP2_CLIENT_H
#define LITEGRPC_HTTP2_CLIENT_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <nghttp2/nghttp2.h>
#include "litegrpc/status.h"

namespace litegrpc {
namespace http2 {

struct Http2Response {
    int status_code = 0;
    std::map<std::string, std::string> headers;
    std::string body;
};

class Http2Client {
public:
    Http2Client();
    ~Http2Client();
    
    // Connection management
    Status Connect(const std::string& host, int port, bool use_ssl);
    void Disconnect();
    bool IsConnected() const;
    
    // HTTP/2 request
    Status SendRequest(
        const std::string& method,
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::string& body,
        Http2Response* response);
    
private:
    struct ConnectionState;
    std::unique_ptr<ConnectionState> state_;
    
    // nghttp2 callbacks
    static ssize_t SendCallback(nghttp2_session* session, const uint8_t* data,
                               size_t length, int flags, void* user_data);
    static int OnFrameRecvCallback(nghttp2_session* session,
                                  const nghttp2_frame* frame, void* user_data);
    static int OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags,
                                      int32_t stream_id, const uint8_t* data,
                                      size_t len, void* user_data);
    static int OnHeaderCallback(nghttp2_session* session,
                               const nghttp2_frame* frame,
                               const uint8_t* name, size_t namelen,
                               const uint8_t* value, size_t valuelen,
                               uint8_t flags, void* user_data);
    static int OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id,
                                    uint32_t error_code, void* user_data);
    
    // Internal methods
    Status InitializeSession();
    Status PerformHandshake();
    Status SendData();
    Status ReceiveData();
    Status ProcessEvents();
    
    // Socket operations
    Status CreateSocket(const std::string& host, int port);
    Status SetupSsl();
    ssize_t SocketSend(const void* data, size_t len);
    ssize_t SocketRecv(void* data, size_t len);
};

} // namespace http2
} // namespace litegrpc

#endif // LITEGRPC_HTTP2_CLIENT_H