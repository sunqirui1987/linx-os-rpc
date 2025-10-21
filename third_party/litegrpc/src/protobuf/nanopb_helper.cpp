/**
 * @file nanopb_helper.cpp
 * @brief Nanopb 辅助工具实现文件
 * 
 * 本文件实现了用于 Nanopb（Protocol Buffers 的轻量级 C 实现）的辅助类和函数。
 * 主要功能包括：
 * - NanopbString：用于处理字符串的包装类
 * - NanopbStringArray：用于处理字符串数组的包装类
 * - NanopbBytes：用于处理字节数据的包装类
 * - 编码/解码函数：用于 Protocol Buffers 序列化和反序列化
 * 
 * 这些工具类提供了内存管理、类型转换和数据编码/解码的功能，
 * 简化了在 C++ 中使用 Nanopb 进行 Protocol Buffers 操作的复杂性。
 */
#include "nanopb_helper.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace litegrpc {

// NanopbString 类实现
/**
 * @brief NanopbString 默认构造函数
 * 
 * 初始化一个空的 NanopbString 对象，所有成员变量设置为初始状态：
 * - data: nullptr（无数据指针）
 * - size: 0（数据大小为0）
 * - capacity: 0（容量为0）
 */
NanopbString::NanopbString() : data(nullptr), size(0), capacity(0) {}

/**
 * @brief NanopbString 析构函数
 * 
 * 释放动态分配的内存资源。
 * 如果 data 指针不为空，则调用 free() 释放内存，防止内存泄漏。
 */
NanopbString::~NanopbString() {
    if (data) {
        free(data);
    }
}

/**
 * @brief 设置字符串内容
 * 
 * 将 std::string 对象的内容复制到 NanopbString 中。
 * 该方法会自动管理内存分配和重新分配。
 * 
 * @param str 要设置的字符串内容
 * 
 * 实现细节：
 * 1. 更新 size 为新字符串的长度
 * 2. 如果当前容量不足，则重新分配内存（容量 = 字符串长度 + 1）
 * 3. 使用 memcpy 复制字符串内容
 * 4. 在字符串末尾添加空终止符 '\0'
 */
void NanopbString::SetString(const std::string& str) {
    size = str.size();
    if (size > capacity) {
        capacity = size + 1;  // +1 为空终止符预留空间
        data = static_cast<char*>(realloc(data, capacity));
    }
    memcpy(data, str.c_str(), size);
    data[size] = '\0';  // 添加空终止符
}

/**
 * @brief 将 NanopbString 转换为 std::string
 * 
 * 将内部存储的字符数据转换为标准的 std::string 对象。
 * 
 * @return std::string 转换后的字符串对象
 * @retval 空字符串 如果 data 为空或 size 为 0
 * @retval 有效字符串 包含实际数据的字符串
 * 
 * 注意：使用 std::string(data, size) 构造函数确保正确处理包含空字符的字符串
 */
std::string NanopbString::ToString() const {
    if (!data || size == 0) {
        return std::string();
    }
    return std::string(data, size);
}

// NanopbStringArray 类实现
/**
 * @brief NanopbStringArray 默认构造函数
 * 
 * 初始化一个空的字符串数组对象，所有成员变量设置为初始状态：
 * - strings: nullptr（无字符串数组指针）
 * - count: 0（当前字符串数量为0）
 * - capacity: 0（数组容量为0）
 */
NanopbStringArray::NanopbStringArray() : strings(nullptr), count(0), capacity(0) {}

/**
 * @brief NanopbStringArray 析构函数
 * 
 * 释放动态分配的内存资源，包括：
 * 1. 逐个调用数组中每个 NanopbString 对象的析构函数
 * 2. 释放字符串数组本身占用的内存
 * 
 * 这确保了所有字符串对象及其内部数据都被正确释放，防止内存泄漏。
 */
NanopbStringArray::~NanopbStringArray() {
    if (strings) {
        // 逐个销毁数组中的 NanopbString 对象
        for (size_t i = 0; i < count; ++i) {
            strings[i].~NanopbString();
        }
        // 释放数组内存
        free(strings);
    }
}

/**
 * @brief 向字符串数组添加新字符串
 * 
 * 将一个新的字符串添加到数组末尾。如果当前容量不足，会自动扩展数组容量。
 * 
 * @param str 要添加的字符串
 * 
 * 实现细节：
 * 1. 检查是否需要扩展容量（当 count >= capacity 时）
 * 2. 容量扩展策略：初始容量为4，之后每次翻倍
 * 3. 使用 placement new 在指定位置构造 NanopbString 对象
 * 4. 设置新字符串的内容并增加计数器
 */
void NanopbStringArray::AddString(const std::string& str) {
    if (count >= capacity) {
        // 容量扩展：初始为4，之后翻倍
        capacity = capacity == 0 ? 4 : capacity * 2;
        strings = static_cast<NanopbString*>(realloc(strings, capacity * sizeof(NanopbString)));
    }
    // 使用 placement new 在指定位置构造对象
    new (&strings[count]) NanopbString();
    strings[count].SetString(str);
    ++count;
}

/**
 * @brief 将字符串数组转换为 std::vector<std::string>
 * 
 * 将内部存储的 NanopbString 数组转换为标准的 std::vector<std::string>。
 * 
 * @return std::vector<std::string> 包含所有字符串的向量
 * 
 * 实现细节：
 * 1. 预先分配向量容量以提高性能
 * 2. 遍历所有 NanopbString 对象，调用 ToString() 方法转换
 * 3. 返回包含所有字符串的向量
 */
std::vector<std::string> NanopbStringArray::ToVector() const {
    std::vector<std::string> result;
    result.reserve(count);  // 预分配容量
    for (size_t i = 0; i < count; ++i) {
        result.push_back(strings[i].ToString());
    }
    return result;
}

// NanopbBytes 类实现
/**
 * @brief NanopbBytes 默认构造函数
 * 
 * 初始化一个空的字节数据对象，所有成员变量设置为初始状态：
 * - data: nullptr（无数据指针）
 * - size: 0（数据大小为0）
 * - capacity: 0（容量为0）
 */
NanopbBytes::NanopbBytes() : data(nullptr), size(0), capacity(0) {}

/**
 * @brief NanopbBytes 析构函数
 * 
 * 释放动态分配的内存资源。
 * 如果 data 指针不为空，则调用 free() 释放内存，防止内存泄漏。
 */
NanopbBytes::~NanopbBytes() {
    if (data) {
        free(data);
    }
}

/**
 * @brief 设置字节数据内容
 * 
 * 将指定的字节数据复制到 NanopbBytes 对象中。
 * 该方法会自动管理内存分配和重新分配。
 * 
 * @param bytes 指向要复制的字节数据的指针
 * @param len 字节数据的长度
 * 
 * 实现细节：
 * 1. 更新 size 为新数据的长度
 * 2. 如果当前容量不足，则重新分配内存（容量 = 数据长度）
 * 3. 使用 memcpy 复制字节数据
 * 
 * 注意：与 NanopbString 不同，字节数据不需要空终止符
 */
void NanopbBytes::SetBytes(const void* bytes, size_t len) {
    size = len;
    if (size > capacity) {
        capacity = size;  // 字节数据不需要额外的空终止符空间
        data = static_cast<uint8_t*>(realloc(data, capacity));
    }
    memcpy(data, bytes, size);
}

/**
 * @brief 将 NanopbBytes 转换为 std::string
 * 
 * 将内部存储的字节数据转换为 std::string 对象。
 * 这对于处理二进制数据或将字节数据传递给需要字符串的接口很有用。
 * 
 * @return std::string 包含字节数据的字符串对象
 * @retval 空字符串 如果 data 为空或 size 为 0
 * @retval 有效字符串 包含实际字节数据的字符串
 * 
 * 注意：使用 reinterpret_cast 将 uint8_t* 转换为 const char*，
 *       并使用 std::string(data, size) 构造函数确保正确处理二进制数据
 */
std::string NanopbBytes::ToString() const {
    if (!data || size == 0) {
        return std::string();
    }
    return std::string(reinterpret_cast<const char*>(data), size);
}

// 编码/解码辅助函数
/**
 * @brief 字符串编码函数
 * 
 * 用于 Nanopb 的字符串编码回调函数。将 std::string 对象编码为 Protocol Buffers 格式。
 * 
 * @param stream 输出流指针，用于写入编码后的数据
 * @param field 字段迭代器，包含字段信息
 * @param arg 指向要编码的 std::string 对象的指针
 * @return bool 编码是否成功
 * @retval true 编码成功
 * @retval false 编码失败（参数无效或编码过程出错）
 * 
 * 实现步骤：
 * 1. 验证输入参数的有效性
 * 2. 为字段编码标签
 * 3. 编码字符串内容
 * 
 * 注意：此函数通常作为 pb_callback_t 的回调函数使用
 */
bool EncodeString(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
    const std::string* str = static_cast<const std::string*>(*arg);
    if (!str) {
        return false;
    }
    
    // 为字段编码标签
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    
    // 编码字符串内容
    return pb_encode_string(stream, 
                           reinterpret_cast<const pb_byte_t*>(str->c_str()), 
                           str->size());
}

/**
 * @brief 字符串解码函数
 * 
 * 用于 Nanopb 的字符串解码回调函数。从 Protocol Buffers 格式解码字符串到 std::string 对象。
 * 
 * @param stream 输入流指针，用于读取编码的数据
 * @param field 字段迭代器，包含字段信息
 * @param arg 指向目标 std::string 对象的指针
 * @return bool 解码是否成功
 * @retval true 解码成功
 * @retval false 解码失败（参数无效或解码过程出错）
 * 
 * 实现步骤：
 * 1. 验证输入参数的有效性
 * 2. 获取待读取的字节数
 * 3. 调整目标字符串大小
 * 4. 从流中读取数据到字符串
 * 
 * 注意：此函数通常作为 pb_callback_t 的回调函数使用
 */
bool DecodeString(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    std::string* str = static_cast<std::string*>(*arg);
    if (!str) {
        return false;
    }
    
    // 获取待读取的字节数
    size_t len = stream->bytes_left;
    str->resize(len);
    
    // 从流中读取数据
    return pb_read(stream, reinterpret_cast<pb_byte_t*>(&(*str)[0]), len);
}

/**
 * @brief 字符串数组编码函数
 * 
 * 用于 Nanopb 的字符串数组编码回调函数。将 std::vector<std::string> 对象编码为 Protocol Buffers 格式。
 * 
 * @param stream 输出流指针，用于写入编码后的数据
 * @param field 字段迭代器，包含字段信息
 * @param arg 指向要编码的 std::vector<std::string> 对象的指针
 * @return bool 编码是否成功
 * @retval true 编码成功
 * @retval false 编码失败（参数无效或编码过程出错）
 * 
 * 实现步骤：
 * 1. 验证输入参数的有效性
 * 2. 遍历字符串向量中的每个字符串
 * 3. 为每个字符串编码字段标签和内容
 * 
 * 注意：在 Protocol Buffers 中，重复字段的每个元素都需要单独的标签
 */
bool EncodeStringArray(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
    const std::vector<std::string>* strings = static_cast<const std::vector<std::string>*>(*arg);
    if (!strings) {
        return false;
    }
    
    // 遍历字符串向量中的每个字符串
    for (const auto& str : *strings) {
        // 为每个字符串编码字段标签
        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }
        
        // 编码字符串内容
        if (!pb_encode_string(stream, 
                             reinterpret_cast<const pb_byte_t*>(str.c_str()), 
                             str.size())) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 字符串数组解码函数
 * 
 * 用于 Nanopb 的字符串数组解码回调函数。从 Protocol Buffers 格式解码字符串到 std::vector<std::string> 对象。
 * 
 * @param stream 输入流指针，用于读取编码的数据
 * @param field 字段迭代器，包含字段信息
 * @param arg 指向目标 std::vector<std::string> 对象的指针
 * @return bool 解码是否成功
 * @retval true 解码成功
 * @retval false 解码失败（参数无效或解码过程出错）
 * 
 * 实现步骤：
 * 1. 验证输入参数的有效性
 * 2. 获取待读取的字节数并创建临时字符串
 * 3. 从流中读取数据到临时字符串
 * 4. 将字符串添加到向量中
 * 
 * 注意：此函数会为每个重复字段元素调用一次，逐个添加字符串到向量中
 */
bool DecodeStringArray(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    std::vector<std::string>* strings = static_cast<std::vector<std::string>*>(*arg);
    if (!strings) {
        return false;
    }
    
    // 获取待读取的字节数并创建临时字符串
    size_t len = stream->bytes_left;
    std::string str(len, '\0');
    
    // 从流中读取数据到临时字符串
    if (!pb_read(stream, reinterpret_cast<pb_byte_t*>(&str[0]), len)) {
        return false;
    }
    
    // 将字符串添加到向量中
    strings->push_back(std::move(str));
    return true;
}

/**
 * @brief 字节数据编码函数
 * 
 * 用于 Nanopb 的字节数据编码回调函数。将 std::string 对象（作为字节容器）编码为 Protocol Buffers 格式。
 * 
 * @param stream 输出流指针，用于写入编码后的数据
 * @param field 字段迭代器，包含字段信息
 * @param arg 指向要编码的 std::string 对象的指针（包含字节数据）
 * @return bool 编码是否成功
 * @retval true 编码成功
 * @retval false 编码失败（参数无效或编码过程出错）
 * 
 * 实现步骤：
 * 1. 验证输入参数的有效性
 * 2. 为字段编码标签
 * 3. 编码字节数据内容
 * 
 * 注意：虽然使用 std::string 作为容器，但实际处理的是二进制字节数据
 *       使用 data() 而不是 c_str() 以确保正确处理包含空字节的数据
 */
bool EncodeBytes(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
    const std::string* bytes = static_cast<const std::string*>(*arg);
    if (!bytes) {
        return false;
    }
    
    // 为字段编码标签
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    
    // 编码字节数据内容
    return pb_encode_string(stream, 
                           reinterpret_cast<const pb_byte_t*>(bytes->data()), 
                           bytes->size());
}

/**
 * @brief 字节数据解码函数
 * 
 * 用于 Nanopb 的字节数据解码回调函数。从 Protocol Buffers 格式解码字节数据到 std::string 对象。
 * 
 * @param stream 输入流指针，用于读取编码的数据
 * @param field 字段迭代器，包含字段信息
 * @param arg 指向目标 std::string 对象的指针（用作字节容器）
 * @return bool 解码是否成功
 * @retval true 解码成功
 * @retval false 解码失败（参数无效或解码过程出错）
 * 
 * 实现步骤：
 * 1. 验证输入参数的有效性
 * 2. 获取待读取的字节数
 * 3. 调整目标字符串大小
 * 4. 从流中读取字节数据
 * 
 * 注意：虽然使用 std::string 作为容器，但实际存储的是二进制字节数据
 *       可能包含空字节或其他非文本字符
 */
bool DecodeBytes(pb_istream_t* stream, const pb_field_iter_t* field, void** arg) {
    std::string* bytes = static_cast<std::string*>(*arg);
    if (!bytes) {
        return false;
    }
    
    // 获取待读取的字节数
    size_t len = stream->bytes_left;
    bytes->resize(len);
    
    // 从流中读取字节数据
    return pb_read(stream, reinterpret_cast<pb_byte_t*>(&(*bytes)[0]), len);
}

} // namespace litegrpc