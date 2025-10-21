#include "nanopb_helper.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace litegrpc {

// NanopbString implementation
NanopbString::NanopbString() : data(nullptr), size(0), capacity(0) {}

NanopbString::~NanopbString() {
    if (data) {
        free(data);
    }
}

void NanopbString::SetString(const std::string& str) {
    size = str.size();
    if (size > capacity) {
        capacity = size + 1;
        data = static_cast<char*>(realloc(data, capacity));
    }
    memcpy(data, str.c_str(), size);
    data[size] = '\0';
}

std::string NanopbString::ToString() const {
    if (!data || size == 0) {
        return std::string();
    }
    return std::string(data, size);
}

// NanopbStringArray implementation
NanopbStringArray::NanopbStringArray() : strings(nullptr), count(0), capacity(0) {}

NanopbStringArray::~NanopbStringArray() {
    if (strings) {
        for (size_t i = 0; i < count; ++i) {
            strings[i].~NanopbString();
        }
        free(strings);
    }
}

void NanopbStringArray::AddString(const std::string& str) {
    if (count >= capacity) {
        capacity = capacity == 0 ? 4 : capacity * 2;
        strings = static_cast<NanopbString*>(realloc(strings, capacity * sizeof(NanopbString)));
    }
    new (&strings[count]) NanopbString();
    strings[count].SetString(str);
    ++count;
}

std::vector<std::string> NanopbStringArray::ToVector() const {
    std::vector<std::string> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(strings[i].ToString());
    }
    return result;
}

// NanopbBytes implementation
NanopbBytes::NanopbBytes() : data(nullptr), size(0), capacity(0) {}

NanopbBytes::~NanopbBytes() {
    if (data) {
        free(data);
    }
}

void NanopbBytes::SetBytes(const void* bytes, size_t len) {
    size = len;
    if (size > capacity) {
        capacity = size;
        data = static_cast<uint8_t*>(realloc(data, capacity));
    }
    memcpy(data, bytes, size);
}

std::string NanopbBytes::ToString() const {
    if (!data || size == 0) {
        return std::string();
    }
    return std::string(reinterpret_cast<const char*>(data), size);
}

// Encoding/decoding helpers
bool EncodeString(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
    const std::string* str = static_cast<const std::string*>(*arg);
    if (!str) {
        return false;
    }
    
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    
    return pb_encode_string(stream, 
                           reinterpret_cast<const pb_byte_t*>(str->c_str()), 
                           str->size());
}

bool DecodeString(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    std::string* str = static_cast<std::string*>(*arg);
    if (!str) {
        return false;
    }
    
    size_t len = stream->bytes_left;
    str->resize(len);
    
    return pb_read(stream, reinterpret_cast<pb_byte_t*>(&(*str)[0]), len);
}

bool EncodeStringArray(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
    const std::vector<std::string>* strings = static_cast<const std::vector<std::string>*>(*arg);
    if (!strings) {
        return false;
    }
    
    for (const auto& str : *strings) {
        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }
        
        if (!pb_encode_string(stream, 
                             reinterpret_cast<const pb_byte_t*>(str.c_str()), 
                             str.size())) {
            return false;
        }
    }
    
    return true;
}

bool DecodeStringArray(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    std::vector<std::string>* strings = static_cast<std::vector<std::string>*>(*arg);
    if (!strings) {
        return false;
    }
    
    size_t len = stream->bytes_left;
    std::string str(len, '\0');
    
    if (!pb_read(stream, reinterpret_cast<pb_byte_t*>(&str[0]), len)) {
        return false;
    }
    
    strings->push_back(std::move(str));
    return true;
}

bool EncodeBytes(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
    const std::string* bytes = static_cast<const std::string*>(*arg);
    if (!bytes) {
        return false;
    }
    
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    
    return pb_encode_string(stream, 
                           reinterpret_cast<const pb_byte_t*>(bytes->data()), 
                           bytes->size());
}

bool DecodeBytes(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    std::string* bytes = static_cast<std::string*>(*arg);
    if (!bytes) {
        return false;
    }
    
    size_t len = stream->bytes_left;
    bytes->resize(len);
    
    return pb_read(stream, reinterpret_cast<pb_byte_t*>(&(*bytes)[0]), len);
}

} // namespace litegrpc