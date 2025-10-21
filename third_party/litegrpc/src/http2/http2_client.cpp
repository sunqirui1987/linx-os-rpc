#include "http2_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstring>
#include <iostream>

namespace litegrpc {
namespace http2 {

struct Http2Client::ConnectionState {
    nghttp2_session* session = nullptr;
    int socket_fd = -1;
    SSL_CTX* ssl_ctx = nullptr;
    SSL* ssl = nullptr;
    bool use_ssl = false;
    bool connected = false;
    
    // Request/response state
    std::map<int32_t, Http2Response> responses;
    int32_t current_stream_id = -1;
    
    ~ConnectionState() {
        if (session) {
            nghttp2_session_del(session);
        }
        if (ssl) {
            SSL_free(ssl);
        }
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
        }
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }
};

Http2Client::Http2Client() : state_(std::make_unique<ConnectionState>()) {
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

Http2Client::~Http2Client() {
    Disconnect();
}

Status Http2Client::Connect(const std::string& host, int port, bool use_ssl) {
    if (state_->connected) {
        return Status::OK();
    }
    
    state_->use_ssl = use_ssl;
    
    // Create socket
    auto status = CreateSocket(host, port);
    if (!status.ok()) {
        return status;
    }
    
    // Setup SSL if needed
    if (use_ssl) {
        status = SetupSsl();
        if (!status.ok()) {
            return status;
        }
    }
    
    // Initialize HTTP/2 session
    status = InitializeSession();
    if (!status.ok()) {
        return status;
    }
    
    // Perform HTTP/2 handshake
    status = PerformHandshake();
    if (!status.ok()) {
        return status;
    }
    
    state_->connected = true;
    return Status::OK();
}

void Http2Client::Disconnect() {
    if (state_->session) {
        nghttp2_session_terminate_session(state_->session, NGHTTP2_NO_ERROR);
        SendData(); // Send GOAWAY frame
    }
    state_->connected = false;
}

bool Http2Client::IsConnected() const {
    return state_->connected;
}

Status Http2Client::SendRequest(
    const std::string& method,
    const std::string& path,
    const std::map<std::string, std::string>& headers,
    const std::string& body,
    Http2Response* response) {
    
    if (!state_->connected) {
        return Status::Unavailable("Not connected");
    }
    
    // Prepare headers
    std::vector<nghttp2_nv> nva;
    
    // Add pseudo-headers
    nghttp2_nv method_nv = {
        (uint8_t*)":method", (uint8_t*)method.c_str(),
        7, method.length(), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(method_nv);
    
    nghttp2_nv path_nv = {
        (uint8_t*)":path", (uint8_t*)path.c_str(),
        5, path.length(), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(path_nv);
    
    nghttp2_nv scheme_nv = {
        (uint8_t*)":scheme", (uint8_t*)(state_->use_ssl ? "https" : "http"),
        7, static_cast<size_t>(state_->use_ssl ? 5 : 4), NGHTTP2_NV_FLAG_NONE
    };
    nva.push_back(scheme_nv);
    
    // Add custom headers
    std::vector<std::string> header_storage;
    for (const auto& header : headers) {
        header_storage.push_back(header.first);
        header_storage.push_back(header.second);
        
        nghttp2_nv header_nv = {
            (uint8_t*)header_storage[header_storage.size()-2].c_str(),
            (uint8_t*)header_storage[header_storage.size()-1].c_str(),
            header.first.length(), header.second.length(),
            NGHTTP2_NV_FLAG_NONE
        };
        nva.push_back(header_nv);
    }
    
    // Submit request
    int32_t stream_id = nghttp2_submit_request(
        state_->session, nullptr, nva.data(), nva.size(), nullptr, nullptr);
    
    if (stream_id < 0) {
        return Status::Internal("Failed to submit request");
    }
    
    state_->current_stream_id = stream_id;
    state_->responses[stream_id] = Http2Response();
    
    // Send request data
    if (!body.empty()) {
        nghttp2_data_provider data_prd;
        data_prd.source.ptr = (void*)body.c_str();
        data_prd.read_callback = [](nghttp2_session* session, int32_t stream_id,
                                   uint8_t* buf, size_t length, uint32_t* data_flags,
                                   nghttp2_data_source* source, void* user_data) -> ssize_t {
            const char* data = (const char*)source->ptr;
            size_t data_len = strlen(data);
            
            if (data_len > length) {
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }
            
            memcpy(buf, data, data_len);
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            return data_len;
        };
        
        nghttp2_submit_data(state_->session, NGHTTP2_FLAG_END_STREAM, stream_id, &data_prd);
    }
    
    // Process the request/response cycle
    auto status = ProcessEvents();
    if (!status.ok()) {
        return status;
    }
    
    // Get response
    auto it = state_->responses.find(stream_id);
    if (it != state_->responses.end()) {
        *response = it->second;
        state_->responses.erase(it);
        return Status::OK();
    }
    
    return Status::Internal("Response not found");
}

Status Http2Client::CreateSocket(const std::string& host, int port) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int rv = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (rv != 0) {
        return Status::Unavailable("Failed to resolve host: " + std::string(gai_strerror(rv)));
    }
    
    state_->socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (state_->socket_fd < 0) {
        freeaddrinfo(result);
        return Status::Unavailable("Failed to create socket");
    }
    
    if (connect(state_->socket_fd, result->ai_addr, result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        return Status::Unavailable("Failed to connect");
    }
    
    freeaddrinfo(result);
    return Status::OK();
}

Status Http2Client::SetupSsl() {
    state_->ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (!state_->ssl_ctx) {
        return Status::Internal("Failed to create SSL context");
    }
    
    // Set ALPN for HTTP/2
    const unsigned char alpn_protos[] = "\x02h2";
    SSL_CTX_set_alpn_protos(state_->ssl_ctx, alpn_protos, sizeof(alpn_protos) - 1);
    
    state_->ssl = SSL_new(state_->ssl_ctx);
    if (!state_->ssl) {
        return Status::Internal("Failed to create SSL object");
    }
    
    SSL_set_fd(state_->ssl, state_->socket_fd);
    
    if (SSL_connect(state_->ssl) <= 0) {
        return Status::Internal("SSL handshake failed");
    }
    
    return Status::OK();
}

Status Http2Client::InitializeSession() {
    nghttp2_session_callbacks* callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    
    nghttp2_session_callbacks_set_send_callback(callbacks, SendCallback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, OnFrameRecvCallback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, OnDataChunkRecvCallback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, OnHeaderCallback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, OnStreamCloseCallback);
    
    int rv = nghttp2_session_client_new(&state_->session, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);
    
    if (rv != 0) {
        return Status::Internal("Failed to create HTTP/2 session");
    }
    
    return Status::OK();
}

Status Http2Client::PerformHandshake() {
    nghttp2_settings_entry iv[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };
    
    int rv = nghttp2_submit_settings(state_->session, NGHTTP2_FLAG_NONE, iv, 1);
    if (rv != 0) {
        return Status::Internal("Failed to submit settings");
    }
    
    return SendData();
}

Status Http2Client::SendData() {
    int rv = nghttp2_session_send(state_->session);
    if (rv != 0) {
        return Status::Internal("Failed to send data");
    }
    return Status::OK();
}

Status Http2Client::ReceiveData() {
    uint8_t buf[8192];
    ssize_t readlen = SocketRecv(buf, sizeof(buf));
    
    if (readlen < 0) {
        return Status::Internal("Failed to receive data");
    }
    
    if (readlen == 0) {
        return Status::Unavailable("Connection closed");
    }
    
    ssize_t rv = nghttp2_session_mem_recv(state_->session, buf, readlen);
    if (rv < 0) {
        return Status::Internal("Failed to process received data");
    }
    
    return Status::OK();
}

Status Http2Client::ProcessEvents() {
    while (true) {
        auto status = SendData();
        if (!status.ok()) {
            return status;
        }
        
        if (nghttp2_session_want_read(state_->session) == 0 &&
            nghttp2_session_want_write(state_->session) == 0) {
            break;
        }
        
        if (nghttp2_session_want_read(state_->session)) {
            status = ReceiveData();
            if (!status.ok()) {
                return status;
            }
        }
    }
    
    return Status::OK();
}

ssize_t Http2Client::SocketSend(const void* data, size_t len) {
    if (state_->use_ssl) {
        return SSL_write(state_->ssl, data, len);
    } else {
        return send(state_->socket_fd, data, len, 0);
    }
}

ssize_t Http2Client::SocketRecv(void* data, size_t len) {
    if (state_->use_ssl) {
        return SSL_read(state_->ssl, data, len);
    } else {
        return recv(state_->socket_fd, data, len, 0);
    }
}

// Static callback implementations
ssize_t Http2Client::SendCallback(nghttp2_session* session, const uint8_t* data,
                                 size_t length, int flags, void* user_data) {
    Http2Client* client = static_cast<Http2Client*>(user_data);
    return client->SocketSend(data, length);
}

int Http2Client::OnFrameRecvCallback(nghttp2_session* session,
                                    const nghttp2_frame* frame, void* user_data) {
    // Handle frame reception
    return 0;
}

int Http2Client::OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags,
                                        int32_t stream_id, const uint8_t* data,
                                        size_t len, void* user_data) {
    Http2Client* client = static_cast<Http2Client*>(user_data);
    auto& response = client->state_->responses[stream_id];
    response.body.append(reinterpret_cast<const char*>(data), len);
    return 0;
}

int Http2Client::OnHeaderCallback(nghttp2_session* session,
                                 const nghttp2_frame* frame,
                                 const uint8_t* name, size_t namelen,
                                 const uint8_t* value, size_t valuelen,
                                 uint8_t flags, void* user_data) {
    Http2Client* client = static_cast<Http2Client*>(user_data);
    
    std::string header_name(reinterpret_cast<const char*>(name), namelen);
    std::string header_value(reinterpret_cast<const char*>(value), valuelen);
    
    if (header_name == ":status") {
        client->state_->responses[frame->hd.stream_id].status_code = std::stoi(header_value);
    } else {
        client->state_->responses[frame->hd.stream_id].headers[header_name] = header_value;
    }
    
    return 0;
}

int Http2Client::OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id,
                                      uint32_t error_code, void* user_data) {
    // Stream closed
    return 0;
}

} // namespace http2
} // namespace litegrpc