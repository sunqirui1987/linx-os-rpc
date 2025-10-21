#ifndef LINX_LOG_H
#define LINX_LOG_H

/**
 * @file linx_log.h
 * @brief LinxOS 轻量级日志系统 - 与 gRPC 兼容的高性能日志库
 * @details 这是一个专为嵌入式系统和 gRPC 服务设计的轻量级日志系统，
 *          提供线程安全、高性能的日志记录功能，完全兼容标准 gRPC 日志接口
 * @author LinxOS Team
 * @version 1.0
 * @date 2024
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 日志级别枚举定义
 * @details 定义了五个标准日志级别，与 gRPC 日志级别完全兼容
 *          级别从低到高：DEBUG < INFO < WARN < ERROR < FATAL
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,    /**< 调试级别 - 详细的调试信息，仅在开发阶段使用 */
    LOG_LEVEL_INFO,         /**< 信息级别 - 一般性信息，记录程序正常运行状态 */
    LOG_LEVEL_WARN,         /**< 警告级别 - 潜在问题，不影响程序继续运行 */
    LOG_LEVEL_ERROR,        /**< 错误级别 - 错误信息，可能影响功能但程序可继续 */
    LOG_LEVEL_FATAL,        /**< 致命级别 - 严重错误，程序无法继续运行 */
    LOG_LEVEL_MAX           /**< 级别上限标记，用于数组边界检查 */
} log_level_t;

/**
 * @brief 日志配置结构体
 * @details 包含所有日志系统的配置选项，支持运行时动态调整
 *          所有配置项都有合理的默认值，确保开箱即用
 */
typedef struct {
    log_level_t level;              /**< 最低日志级别 - 低于此级别的日志将被过滤 */
    bool enable_timestamp;          /**< 是否启用时间戳 - 在日志中显示精确的时间信息 */
    bool enable_thread_id;          /**< 是否启用线程ID - 多线程环境下便于调试 */
    bool enable_color;              /**< 是否启用颜色输出 - 提高日志可读性（仅终端支持） */
    bool enable_file_info;          /**< 是否启用文件信息 - 显示源文件名和行号 */
    bool enable_function_name;      /**< 是否启用函数名 - 显示调用日志的函数名 */
    size_t max_message_length;      /**< 最大消息长度 - 防止过长日志影响性能 */
} log_config_t;

/**
 * @brief 日志上下文结构体
 * @details 维护日志系统的全局状态，包括配置信息和初始化状态
 *          使用单例模式确保全局唯一性
 */
typedef struct {
    log_config_t config;            /**< 当前日志配置 */
    bool initialized;               /**< 初始化状态标志 */
    FILE* output_file;              /**< 输出文件句柄（NULL表示输出到stderr） */
} log_context_t;

/**
 * @brief 默认日志配置
 * @details 提供合理的默认配置，适用于大多数应用场景
 *          - 日志级别：INFO（过滤调试信息，保留重要信息）
 *          - 启用时间戳：便于问题追踪和性能分析
 *          - 禁用线程ID：减少输出冗余（单线程应用）
 *          - 启用颜色：提高终端可读性
 *          - 启用文件信息：便于定位问题源码位置
 *          - 启用函数名：便于调用栈追踪
 *          - 最大消息长度：1024字节（平衡性能和功能）
 */
#define LOG_DEFAULT_CONFIG { \
    .level = LOG_LEVEL_INFO, \
    .enable_timestamp = true, \
    .enable_thread_id = false, \
    .enable_color = true, \
    .enable_file_info = true, \
    .enable_function_name = true, \
    .max_message_length = 1024 \
}

/**
 * @brief 日志级别字符串数组
 * @details 将日志级别枚举值转换为可读的字符串表示
 *          用于日志输出中的级别标识，便于日志解析和过滤
 */
extern const char* log_level_strings[LOG_LEVEL_MAX];

/**
 * @brief 日志级别颜色代码数组
 * @details ANSI 颜色转义序列，为不同级别的日志提供颜色区分
 *          仅在支持 ANSI 颜色的终端中生效，提高日志可读性
 *          颜色映射：DEBUG(青色), INFO(绿色), WARN(黄色), ERROR(红色), FATAL(紫色)
 */
extern const char* log_level_colors[LOG_LEVEL_MAX];

/* ========================================================================
 * 核心 API 函数
 * ======================================================================== */

/**
 * @brief 初始化日志模块
 * @details 初始化全局日志系统，设置配置参数并准备日志输出环境
 *          此函数是线程安全的，可以在多线程环境中安全调用
 *          如果已经初始化，会先清理现有配置再重新初始化
 * 
 * @param config 日志配置指针，如果为 NULL 则使用默认配置
 * @return int 返回状态码
 *         - 0: 初始化成功
 *         - -1: 初始化失败（内存不足或其他系统错误）
 * 
 * @note 建议在程序启动时调用一次，在程序退出前调用 log_cleanup()
 * @see log_cleanup(), LOG_DEFAULT_CONFIG
 */
int log_init(const log_config_t *config);

/**
 * @brief 清理日志模块
 * @details 清理日志系统资源，关闭文件句柄，释放内存
 *          此函数是线程安全的，调用后日志系统将停止工作
 *          建议在程序退出前调用以确保所有日志都被正确输出
 * 
 * @note 清理后如需继续使用日志，需要重新调用 log_init()
 * @see log_init()
 */
void log_cleanup(void);

/**
 * @brief 设置日志级别
 * @details 动态调整日志输出级别，低于设定级别的日志将被过滤
 *          此函数是线程安全的，可以在运行时随时调用
 * 
 * @param level 要设置的日志级别，必须是有效的 log_level_t 值
 * 
 * @note 无效的级别值将被忽略，不会改变当前设置
 * @see log_get_level(), log_is_level_enabled()
 */
void log_set_level(log_level_t level);

/**
 * @brief 获取当前日志级别
 * @details 返回当前设置的最低日志输出级别
 *          此函数是线程安全的
 * 
 * @return log_level_t 当前的日志级别
 * 
 * @see log_set_level()
 */
log_level_t log_get_level(void);

/**
 * @brief 写入日志消息
 * @details 核心日志输出函数，格式化并输出日志消息
 *          此函数是线程安全的，支持 printf 风格的格式化字符串
 *          会自动添加时间戳、文件信息、函数名等元数据（根据配置）
 * 
 * @param level 日志级别，必须是有效的 log_level_t 值
 * @param file 源文件名，通常使用 __FILE__ 宏
 * @param line 源文件行号，通常使用 __LINE__ 宏
 * @param func 函数名，通常使用 __func__ 宏
 * @param format printf 风格的格式化字符串
 * @param ... 可变参数，对应格式化字符串中的占位符
 * 
 * @note 如果日志级别低于当前设置或系统未初始化，消息将被忽略
 * @note 建议使用便捷宏（LOG_DEBUG, LOG_INFO 等）而不是直接调用此函数
 * @see LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
 */
void log_write(log_level_t level, const char *file, int line, 
               const char *func, const char *format, ...);

/**
 * @brief 刷新日志缓冲区
 * @details 强制将缓冲区中的所有日志消息立即输出到目标设备
 *          此函数是线程安全的，在需要确保日志及时输出时调用
 * 
 * @note 在程序异常退出前调用此函数可以确保重要日志不丢失
 * @see log_write()
 */
void log_flush(void);

/**
 * @brief 检查指定日志级别是否启用
 * @details 快速检查指定级别的日志是否会被输出，用于性能优化
 *          避免在日志被过滤时进行昂贵的字符串格式化操作
 * 
 * @param level 要检查的日志级别
 * @return bool 
 *         - true: 该级别的日志会被输出
 *         - false: 该级别的日志会被过滤
 * 
 * @note 便捷宏内部会自动调用此函数进行优化
 * @see LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
 */
bool log_is_level_enabled(log_level_t level);

/**
 * @brief 设置日志输出文件
 * @details 将日志输出重定向到指定文件，如果文件为 NULL 则输出到 stderr
 *          此函数是线程安全的，支持运行时切换输出目标
 * 
 * @param filepath 日志文件路径，NULL 表示输出到 stderr
 * @return int 返回状态码
 *         - 0: 设置成功
 *         - -1: 文件打开失败
 * 
 * @note 如果之前设置了文件输出，旧文件会被自动关闭
 */
int log_set_output_file(const char *filepath);

/**
 * @brief 获取当前线程ID（用于多线程日志）
 * @details 获取当前线程的唯一标识符，用于多线程环境下的日志区分
 * 
 * @return unsigned long 当前线程ID
 * 
 * @note 仅在启用线程ID显示时使用
 */
unsigned long log_get_thread_id(void);

/* ========================================================================
 * 便捷宏定义 - 推荐使用的日志接口
 * ======================================================================== */

/**
 * @brief 调试级别日志宏
 * @details 输出调试信息，仅在开发和调试阶段使用
 *          自动包含文件名、行号、函数名等调试信息
 *          在发布版本中通常会被过滤以提高性能
 * 
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 * 
 * @note 使用 do-while(0) 包装确保宏的安全性
 * @note 内置级别检查避免不必要的字符串格式化开销
 */
#define LOG_DEBUG(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_DEBUG)) { \
            log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief 信息级别日志宏
 * @details 输出一般性信息，记录程序正常运行状态
 *          适用于记录重要的业务流程和状态变化
 * 
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LOG_INFO(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_INFO)) { \
            log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief 警告级别日志宏
 * @details 输出警告信息，表示潜在问题但不影响程序继续运行
 *          适用于记录异常情况、性能问题、配置问题等
 * 
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LOG_WARN(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_WARN)) { \
            log_write(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief 错误级别日志宏
 * @details 输出错误信息，表示发生了错误但程序可以继续运行
 *          适用于记录操作失败、资源不足、网络错误等
 * 
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LOG_ERROR(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_ERROR)) { \
            log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief 致命错误级别日志宏
 * @details 输出致命错误信息，表示严重错误导致程序无法继续运行
 *          适用于记录系统崩溃、内存耗尽、关键资源失败等
 * 
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 * 
 * @note 通常在输出 FATAL 日志后程序会退出
 */
#define LOG_FATAL(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_FATAL)) { \
            log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/* ========================================================================
 * LinxOS 设备专用日志宏 - 带标签的日志接口
 * ======================================================================== */

/**
 * @brief LinxOS 错误日志宏（带标签）
 * @details 专为 LinxOS 设备服务设计的错误日志接口
 *          自动添加标签前缀，便于日志分类和过滤
 * 
 * @param tag 日志标签，用于标识日志来源模块
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LINX_LOGE(tag, fmt, ...) LOG_ERROR("[%s] " fmt, tag, ##__VA_ARGS__)

/**
 * @brief LinxOS 警告日志宏（带标签）
 * @details 专为 LinxOS 设备服务设计的警告日志接口
 * 
 * @param tag 日志标签，用于标识日志来源模块
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LINX_LOGW(tag, fmt, ...) LOG_WARN("[%s] " fmt, tag, ##__VA_ARGS__)

/**
 * @brief LinxOS 信息日志宏（带标签）
 * @details 专为 LinxOS 设备服务设计的信息日志接口
 * 
 * @param tag 日志标签，用于标识日志来源模块
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LINX_LOGI(tag, fmt, ...) LOG_INFO("[%s] " fmt, tag, ##__VA_ARGS__)

/**
 * @brief LinxOS 调试日志宏（带标签）
 * @details 专为 LinxOS 设备服务设计的调试日志接口
 * 
 * @param tag 日志标签，用于标识日志来源模块
 * @param fmt printf 风格的格式化字符串
 * @param ... 可变参数列表
 */
#define LINX_LOGD(tag, fmt, ...) LOG_DEBUG("[%s] " fmt, tag, ##__VA_ARGS__)

/* ========================================================================
 * gRPC 兼容性宏定义 - 与标准 gRPC 日志接口兼容
 * ======================================================================== */

/**
 * @brief gRPC 兼容的日志宏
 * @details 提供与标准 gRPC 库兼容的日志接口，便于代码迁移
 *          支持 gRPC 的日志级别和格式约定
 */
#ifndef GRPC_LOG_VERBOSITY_NONE
#define GRPC_LOG_VERBOSITY_NONE    0
#define GRPC_LOG_VERBOSITY_ERROR   1
#define GRPC_LOG_VERBOSITY_WARNING 2
#define GRPC_LOG_VERBOSITY_INFO    3
#define GRPC_LOG_VERBOSITY_DEBUG   4

/* gRPC 风格的日志宏 */
#define gpr_log(severity, ...) \
    do { \
        switch(severity) { \
            case GRPC_LOG_VERBOSITY_ERROR: LOG_ERROR(__VA_ARGS__); break; \
            case GRPC_LOG_VERBOSITY_WARNING: LOG_WARN(__VA_ARGS__); break; \
            case GRPC_LOG_VERBOSITY_INFO: LOG_INFO(__VA_ARGS__); break; \
            case GRPC_LOG_VERBOSITY_DEBUG: LOG_DEBUG(__VA_ARGS__); break; \
            default: break; \
        } \
    } while(0)

#define gpr_log_message(severity, message) gpr_log(severity, "%s", message)
#endif /* GRPC_LOG_VERBOSITY_NONE */


#ifdef __cplusplus
}
#endif

#endif /* LINX_LOG_H */