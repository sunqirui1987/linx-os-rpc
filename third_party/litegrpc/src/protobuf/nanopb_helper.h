/**
 * @file nanopb_helper.h
 * @brief nanopb Protocol Buffers 辅助工具库
 * @author LiteGRPC Team
 * @date 2024
 * @version 1.0
 * 
 * 本文件提供了 nanopb 库的 C++ 封装和辅助功能，简化 Protocol Buffers
 * 消息的序列化和反序列化操作。主要功能包括：
 * 
 * 核心特性：
 * - 模板化的序列化/反序列化函数
 * - 字符串和字节数组的便捷处理
 * - 动态内存管理的封装
 * - 编码/解码回调函数
 * 
 * 设计目标：
 * - 提供类型安全的 Protocol Buffers 操作
 * - 简化 nanopb 的使用复杂度
 * - 支持动态大小的字符串和数组
 * - 内存安全和自动资源管理
 * 
 * 技术实现：
 * - 基于 nanopb 轻量级 Protocol Buffers 库
 * - 使用 RAII 模式管理内存资源
 * - 提供 STL 容器的兼容接口
 * - 支持自定义编码/解码逻辑
 */

#ifndef LITEGRPC_NANOPB_HELPER_H
#define LITEGRPC_NANOPB_HELPER_H

#include "pb.h"          // nanopb 核心头文件
#include "pb_encode.h"   // nanopb 编码功能
#include "pb_decode.h"   // nanopb 解码功能
#include <string>        // 标准字符串类
#include <vector>        // 标准向量容器
#include <cstdint>       // 标准整数类型

namespace litegrpc {

//==============================================================================
// Protocol Buffers 序列化/反序列化模板函数
//==============================================================================

/**
 * @brief 将 Protocol Buffers 消息序列化为字符串
 * @tparam T Protocol Buffers 消息类型
 * @param message 要序列化的消息对象
 * @param fields nanopb 字段描述符
 * @param output 输出字符串指针
 * @return bool 序列化是否成功
 * 
 * 执行以下步骤进行序列化：
 * 1. 计算编码后的消息大小
 * 2. 调整输出字符串的大小
 * 3. 创建输出流并进行编码
 * 
 * 使用示例：
 * @code
 * MyMessage msg;
 * std::string serialized;
 * if (SerializeToString(msg, MyMessage_fields, &serialized)) {
 *     // 序列化成功
 * }
 * @endcode
 */
template<typename T>
bool SerializeToString(const T& message, const pb_msgdesc_t* fields, std::string* output) {
    size_t encoded_size;
    // 计算编码后的消息大小
    if (!pb_get_encoded_size(&encoded_size, fields, &message)) {
        return false;
    }
    
    // 调整输出字符串大小
    output->resize(encoded_size);
    pb_ostream_t stream = pb_ostream_from_buffer(
        reinterpret_cast<pb_byte_t*>(&(*output)[0]), encoded_size);
    
    // 执行编码
    return pb_encode(&stream, fields, &message);
}

/**
 * @brief 从字符串解析 Protocol Buffers 消息
 * @tparam T Protocol Buffers 消息类型
 * @param message 输出消息对象指针
 * @param fields nanopb 字段描述符
 * @param input 输入字符串
 * @return bool 解析是否成功
 * 
 * 从二进制字符串数据中解析 Protocol Buffers 消息：
 * 1. 创建输入流
 * 2. 执行解码操作
 * 3. 填充消息对象
 * 
 * 使用示例：
 * @code
 * MyMessage msg;
 * std::string serialized_data;
 * if (ParseFromString(&msg, MyMessage_fields, serialized_data)) {
 *     // 解析成功
 * }
 * @endcode
 */
template<typename T>
bool ParseFromString(T* message, const pb_msgdesc_t* fields, const std::string& input) {
    pb_istream_t stream = pb_istream_from_buffer(
        reinterpret_cast<const pb_byte_t*>(input.data()), input.size());
    
    return pb_decode(&stream, fields, message);
}

//==============================================================================
// nanopb 字符串处理辅助结构体
//==============================================================================

/**
 * @brief nanopb 字符串封装结构体
 * 
 * 提供动态字符串的内存管理和 STL 字符串的兼容接口。
 * 自动处理内存分配、释放和大小调整。
 * 
 * 特性：
 * - 自动内存管理（RAII）
 * - 与 std::string 的双向转换
 * - 动态容量调整
 * - 内存安全保证
 */
struct NanopbString {
    char* data;          ///< 字符串数据指针
    size_t size;         ///< 当前字符串长度
    size_t capacity;     ///< 分配的内存容量
    
    /**
     * @brief 默认构造函数
     * 初始化空字符串，设置所有成员为零值
     */
    NanopbString();
    
    /**
     * @brief 析构函数
     * 自动释放分配的内存资源
     */
    ~NanopbString();
    
    /**
     * @brief 设置字符串内容
     * @param str 源字符串
     * 
     * 从 std::string 复制内容，自动调整内存大小
     */
    void SetString(const std::string& str);
    
    /**
     * @brief 转换为 std::string
     * @return std::string 转换后的字符串
     * 
     * 创建包含当前字符串内容的 std::string 对象
     */
    std::string ToString() const;
};

/**
 * @brief nanopb 字符串数组封装结构体
 * 
 * 提供动态字符串数组的内存管理和 STL 向量的兼容接口。
 * 支持动态添加字符串元素和自动内存管理。
 * 
 * 特性：
 * - 动态数组大小调整
 * - 自动内存管理（RAII）
 * - 与 std::vector<std::string> 的双向转换
 * - 高效的元素添加操作
 */
struct NanopbStringArray {
    NanopbString* strings;   ///< 字符串数组指针
    size_t count;            ///< 当前字符串数量
    size_t capacity;         ///< 分配的数组容量
    
    /**
     * @brief 默认构造函数
     * 初始化空数组，设置所有成员为零值
     */
    NanopbStringArray();
    
    /**
     * @brief 析构函数
     * 自动释放所有字符串和数组内存
     */
    ~NanopbStringArray();
    
    /**
     * @brief 添加字符串到数组
     * @param str 要添加的字符串
     * 
     * 将字符串添加到数组末尾，自动扩展容量
     */
    void AddString(const std::string& str);
    
    /**
     * @brief 转换为 std::vector<std::string>
     * @return std::vector<std::string> 转换后的字符串向量
     * 
     * 创建包含所有字符串的 std::vector 对象
     */
    std::vector<std::string> ToVector() const;
};

/**
 * @brief nanopb 字节数组封装结构体
 * 
 * 提供二进制数据的内存管理和便捷操作接口。
 * 支持任意二进制数据的存储和转换。
 * 
 * 特性：
 * - 二进制数据安全存储
 * - 自动内存管理（RAII）
 * - 与字符串的双向转换
 * - 动态容量调整
 */
struct NanopbBytes {
    uint8_t* data;       ///< 字节数据指针
    size_t size;         ///< 当前数据大小
    size_t capacity;     ///< 分配的内存容量
    
    /**
     * @brief 默认构造函数
     * 初始化空字节数组，设置所有成员为零值
     */
    NanopbBytes();
    
    /**
     * @brief 析构函数
     * 自动释放分配的内存资源
     */
    ~NanopbBytes();
    
    /**
     * @brief 设置字节数据
     * @param data 源数据指针
     * @param size 数据大小
     * 
     * 从源数据复制内容，自动调整内存大小
     */
    void SetBytes(const void* data, size_t size);
    
    /**
     * @brief 转换为字符串
     * @return std::string 转换后的字符串
     * 
     * 将字节数据转换为字符串（可能包含二进制数据）
     */
    std::string ToString() const;
};

//==============================================================================
// nanopb 编码/解码回调函数
// 这些函数用于处理动态大小的字段，如字符串、数组和字节数据
//==============================================================================

/**
 * @brief 字符串编码回调函数
 * @param stream 输出流指针
 * @param field 字段迭代器
 * @param arg 用户参数（NanopbString 指针）
 * @return bool 编码是否成功
 * 
 * 用于编码动态字符串字段的回调函数。
 * 在 Protocol Buffers 编码过程中由 nanopb 调用。
 */
bool EncodeString(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg);

/**
 * @brief 字符串解码回调函数
 * @param stream 输入流指针
 * @param field 字段迭代器
 * @param arg 用户参数（NanopbString 指针的指针）
 * @return bool 解码是否成功
 * 
 * 用于解码动态字符串字段的回调函数。
 * 在 Protocol Buffers 解码过程中由 nanopb 调用。
 */
bool DecodeString(pb_istream_t* stream, const pb_field_iter_t* field, void** arg);

/**
 * @brief 字符串数组编码回调函数
 * @param stream 输出流指针
 * @param field 字段迭代器
 * @param arg 用户参数（NanopbStringArray 指针）
 * @return bool 编码是否成功
 * 
 * 用于编码动态字符串数组字段的回调函数。
 * 支持重复字符串字段的编码。
 */
bool EncodeStringArray(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg);

/**
 * @brief 字符串数组解码回调函数
 * @param stream 输入流指针
 * @param field 字段迭代器
 * @param arg 用户参数（NanopbStringArray 指针的指针）
 * @return bool 解码是否成功
 * 
 * 用于解码动态字符串数组字段的回调函数。
 * 支持重复字符串字段的解码。
 */
bool DecodeStringArray(pb_istream_t* stream, const pb_field_iter_t* field, void** arg);

/**
 * @brief 字节数组编码回调函数
 * @param stream 输出流指针
 * @param field 字段迭代器
 * @param arg 用户参数（NanopbBytes 指针）
 * @return bool 编码是否成功
 * 
 * 用于编码动态字节数组字段的回调函数。
 * 支持二进制数据的编码。
 */
bool EncodeBytes(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg);

/**
 * @brief 字节数组解码回调函数
 * @param stream 输入流指针
 * @param field 字段迭代器
 * @param arg 用户参数（NanopbBytes 指针的指针）
 * @return bool 解码是否成功
 * 
 * 用于解码动态字节数组字段的回调函数。
 * 支持二进制数据的解码。
 */
bool DecodeBytes(pb_istream_t* stream, const pb_field_iter_t* field, void** arg);

} // namespace litegrpc

#endif // LITEGRPC_NANOPB_HELPER_H