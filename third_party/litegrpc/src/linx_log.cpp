/**
 * @file linx_log.cpp
 * @brief LinxOS 轻量级日志系统实现
 * @details 提供线程安全的日志记录功能，支持多种日志级别、颜色输出、
 *          时间戳记录、文件/行号/函数名追踪等特性。
 *          设计目标：
 *          1. 高性能：最小化日志记录开销
 *          2. 线程安全：支持多线程并发日志记录
 *          3. 可配置：支持运行时配置日志级别和输出格式
 *          4. gRPC 兼容：提供与标准 gRPC 日志系统的兼容接口
 *          5. 轻量级：最小化内存占用和依赖
 * 
 * @author LinxOS Team
 * @date 2024
 * @version 1.0
 * 
 * @note 本日志系统专为 LiteGRPC 项目设计，确保与 gRPC 生态系统的兼容性
 */

#include "linx_log.h"
#include <stdio.h>      // printf, fprintf, fflush
#include <stdlib.h>     // malloc, free, exit
#include <string.h>     // strlen, strcpy, strncpy
#include <stdarg.h>     // va_list, va_start, va_end
#include <time.h>       // time, localtime, strftime
#include <pthread.h>    // pthread_mutex_t, pthread_mutex_lock/unlock
#include <unistd.h>     // getpid, syscall
#include <sys/time.h>   // gettimeofday
#include <sys/syscall.h> // SYS_gettid

/**
 * @brief 全局日志上下文
 * @details 存储日志系统的全局状态和配置信息
 *          使用静态初始化确保在程序启动时就可用
 */
static log_context_t g_log_context = {0};
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 日志级别字符串 */
const char* log_level_strings[LOG_LEVEL_MAX] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

/* 日志级别颜色代码 (ANSI) */
const char* log_level_colors[LOG_LEVEL_MAX] = {
    "\033[36m",  /* DEBUG - 青色 */
    "\033[32m",  /* INFO  - 绿色 */
    "\033[33m",  /* WARN  - 黄色 */
    "\033[31m",  /* ERROR - 红色 */
    "\033[35m"   /* FATAL - 紫色 */
};

/* 颜色重置 */
#define COLOR_RESET "\033[0m"

/* 内部函数声明 */
static void log_format_timestamp(char *buffer, size_t size);

int log_init(const log_config_t *config)
{
    pthread_mutex_lock(&g_log_mutex);
    
    /* 如果已经初始化，先清理 */
    if (g_log_ctx.initialized) {
        log_cleanup();
    }
    
    /* 使用默认配置或用户配置 */
    if (config) {
        g_log_ctx.config = *config;
    } else {
        log_config_t default_config = LOG_DEFAULT_CONFIG;
        g_log_ctx.config = default_config;
    }
    
    g_log_ctx.initialized = true;
    pthread_mutex_unlock(&g_log_mutex);
    
    return 0;
}

void log_cleanup(void)
{
    pthread_mutex_lock(&g_log_mutex);
    
    g_log_ctx.initialized = false;
    
    pthread_mutex_unlock(&g_log_mutex);
}

void log_set_level(log_level_t level)
{
    if (level >= LOG_LEVEL_DEBUG && level < LOG_LEVEL_MAX) {
        pthread_mutex_lock(&g_log_mutex);
        g_log_ctx.config.level = level;
        pthread_mutex_unlock(&g_log_mutex);
    }
}

log_level_t log_get_level(void)
{
    pthread_mutex_lock(&g_log_mutex);
    log_level_t level = g_log_ctx.config.level;
    pthread_mutex_unlock(&g_log_mutex);
    return level;
}

bool log_is_level_enabled(log_level_t level)
{
    return (g_log_ctx.initialized && level >= g_log_ctx.config.level);
}

void log_write(log_level_t level, const char *file, int line, 
               const char *func, const char *format, ...)
{
    if (!g_log_ctx.initialized || level < g_log_ctx.config.level) {
        return;
    }
    
    pthread_mutex_lock(&g_log_mutex);
    
    char timestamp[64] = {0};
    char message[1024] = {0};
    char log_line[1536] = {0};
    
    /* 格式化时间戳 */
    if (g_log_ctx.config.enable_timestamp) {
        log_format_timestamp(timestamp, sizeof(timestamp));
    }
    
    /* 格式化用户消息 */
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    /* 构建完整的日志行 */
    const char *basename = strrchr(file, '/');
    basename = basename ? basename + 1 : file;
    
    if (g_log_ctx.config.enable_timestamp) {
        snprintf(log_line, sizeof(log_line), "[%s] [%s] %s:%d %s() - %s\n",
                timestamp, log_level_strings[level], basename, line, func, message);
    } else {
        snprintf(log_line, sizeof(log_line), "[%s] %s:%d %s() - %s\n",
                log_level_strings[level], basename, line, func, message);
    }
    
    /* 输出到控制台 */
    if (g_log_ctx.config.enable_color) {
        fprintf(stderr, "%s%s%s", log_level_colors[level], log_line, COLOR_RESET);
    } else {
        fprintf(stderr, "%s", log_line);
    }
    fflush(stderr);
    
    pthread_mutex_unlock(&g_log_mutex);
}

void log_flush(void)
{
    pthread_mutex_lock(&g_log_mutex);
    
    fflush(stderr);
    fflush(stdout);
    
    pthread_mutex_unlock(&g_log_mutex);
}

/* 内部函数实现 */

static void log_format_timestamp(char *buffer, size_t size)
{
    time_t now;
    struct tm *tm_info;
    
    time(&now);
    tm_info = localtime(&now);
    
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}