#pragma once

#include <stdarg.h>
#include <stdio.h>

enum {
    _LOG_TRACE,
    _LOG_DEBUG,
    _LOG_INFO,
    _LOG_WARN,
    _LOG_ERR,
    _LOG_CRITICAL
};

static inline const char* _log_level_str(int level) {
    switch (level) {
        case _LOG_TRACE:    return "\033[36m"   "TRACE"    "\033[0m";
        case _LOG_DEBUG:    return "\033[34m"   "DEBUG"    "\033[0m";
        case _LOG_INFO:     return "\033[32m"   "INFO"     "\033[0m";
        case _LOG_WARN:     return "\033[33m"   "WARN"     "\033[0m";
        case _LOG_ERR:      return "\033[31m"   "ERROR"    "\033[0m";
        case _LOG_CRITICAL: return "\033[1;31m" "CRITICAL" "\033[0m";
        default:           return              "UNKNOWN";
    }
}

static inline void _log(int level, const char* format, va_list args) {
    printf("[%s] ", _log_level_str(level));

    vprintf(format, args);

    printf("\n");
}

static inline void log_trace(const char* format, ...) {
#ifndef DISABLE_LOG_TRACE
    va_list args; va_start(args, format);
    _log(_LOG_TRACE, format, args);
    va_end(args);
#endif
}

static inline void log_debug(const char* format, ...) {
#ifndef DISABLE_LOG_DEBUG
    va_list args; va_start(args, format);
    _log(_LOG_DEBUG, format, args);
    va_end(args);
#endif
}

static inline void log_info(const char* format, ...) {
#ifndef DISABLE_LOG_INFO
    va_list args; va_start(args, format);
    _log(_LOG_INFO, format, args);
    va_end(args);
#endif
}

static inline void log_warn(const char* format, ...) {
#ifndef DISABLE_LOG_WARN
    va_list args; va_start(args, format);
    _log(_LOG_WARN, format, args);
    va_end(args);
#endif
}

static inline void log_err(const char* format, ...) {
#ifndef DISABLE_LOG_ERR
    va_list args; va_start(args, format);
    _log(_LOG_ERR, format, args);
    va_end(args);
#endif
}
