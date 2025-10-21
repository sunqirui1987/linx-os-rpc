#ifndef LITEGRPC_NANOPB_HELPER_H
#define LITEGRPC_NANOPB_HELPER_H

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include <string>
#include <vector>
#include <cstdint>

namespace litegrpc {

// Helper functions for nanopb serialization/deserialization
template<typename T>
bool SerializeToString(const T& message, const pb_msgdesc_t* fields, std::string* output) {
    size_t encoded_size;
    if (!pb_get_encoded_size(&encoded_size, fields, &message)) {
        return false;
    }
    
    output->resize(encoded_size);
    pb_ostream_t stream = pb_ostream_from_buffer(
        reinterpret_cast<pb_byte_t*>(&(*output)[0]), encoded_size);
    
    return pb_encode(&stream, fields, &message);
}

template<typename T>
bool ParseFromString(T* message, const pb_msgdesc_t* fields, const std::string& input) {
    pb_istream_t stream = pb_istream_from_buffer(
        reinterpret_cast<const pb_byte_t*>(input.data()), input.size());
    
    return pb_decode(&stream, fields, message);
}

// Helper structures for nanopb string handling
struct NanopbString {
    char* data;
    size_t size;
    size_t capacity;
    
    NanopbString();
    ~NanopbString();
    void SetString(const std::string& str);
    std::string ToString() const;
};

struct NanopbStringArray {
    NanopbString* strings;
    size_t count;
    size_t capacity;
    
    NanopbStringArray();
    ~NanopbStringArray();
    void AddString(const std::string& str);
    std::vector<std::string> ToVector() const;
};

struct NanopbBytes {
    uint8_t* data;
    size_t size;
    size_t capacity;
    
    NanopbBytes();
    ~NanopbBytes();
    void SetBytes(const void* data, size_t size);
    std::string ToString() const;
};

// Encoding/decoding helpers
bool EncodeString(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg);
bool DecodeString(pb_istream_t* stream, const pb_field_iter_t* field, void** arg);
bool EncodeStringArray(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg);
bool DecodeStringArray(pb_istream_t* stream, const pb_field_iter_t* field, void** arg);
bool EncodeBytes(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg);
bool DecodeBytes(pb_istream_t* stream, const pb_field_iter_t* field, void** arg);

} // namespace litegrpc

#endif // LITEGRPC_NANOPB_HELPER_H