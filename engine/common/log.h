// PSV原生移植 - 日志系统
#pragma once
#include <cstdio>
#include <cstdarg>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "P5D-Native"
#define LOG_PRINT(level, fmt, ...) \
    __android_log_print(level, LOG_TAG, fmt, ##__VA_ARGS__)
#else
// 非Android平台定义日志级别常量
#ifndef ANDROID_LOG_DEBUG
#define ANDROID_LOG_DEBUG 3
#define ANDROID_LOG_INFO  4
#define ANDROID_LOG_WARN  5
#define ANDROID_LOG_ERROR 6
#define ANDROID_LOG_FATAL 7
#endif
#define LOG_PRINT(level, fmt, ...) \
    fprintf(stderr, "[%s] " fmt "\n", log_level_str(level), ##__VA_ARGS__)
#endif

enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4,
};

static inline const char* log_level_str(int level) {
    // ANDROID_LOG_*: DEBUG=3, INFO=4, WARN=5, ERROR=6, FATAL=7
    switch (level) {
        case 3: return "DEBUG";
        case 4: return "INFO";
        case 5: return "WARN";
        case 6: return "ERROR";
        case 7: return "FATAL";
        default: return "UNKNOWN";
    }
}

// 全局日志级别控制
extern int g_log_level;

#define LOGD(fmt, ...) do { if (g_log_level <= LOG_LEVEL_DEBUG) LOG_PRINT(ANDROID_LOG_DEBUG, fmt, ##__VA_ARGS__); } while(0)
#define LOGI(fmt, ...) do { if (g_log_level <= LOG_LEVEL_INFO)  LOG_PRINT(ANDROID_LOG_INFO,  fmt, ##__VA_ARGS__); } while(0)
#define LOGW(fmt, ...) do { if (g_log_level <= LOG_LEVEL_WARN)  LOG_PRINT(ANDROID_LOG_WARN,  fmt, ##__VA_ARGS__); } while(0)
#define LOGE(fmt, ...) do { if (g_log_level <= LOG_LEVEL_ERROR) LOG_PRINT(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__); } while(0)
#define LOGF(fmt, ...) do { LOG_PRINT(ANDROID_LOG_FATAL, fmt, ##__VA_ARGS__); } while(0)

// 用于标记未实现的PSV API调用
#define STUB_LOG(func_name) \
    LOGW("[STUB] %s called - NOT IMPLEMENTED", func_name)

#define STUB_LOG_WITH_ARGS(func_name, fmt, ...) \
    LOGW("[STUB] %s(" fmt ") called - NOT IMPLEMENTED", func_name, ##__VA_ARGS__)
