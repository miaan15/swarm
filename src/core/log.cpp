module;

#include <cstdarg>
#include <cstdio>

export module log;

import def;

export namespace sw {

enum {
    LOG_TRACE_LEVEL,
    LOG_DEBUG_LEVEL,
    LOG_INFO_LEVEL,
    LOG_WARN_LEVEL,
    LOG_ERR_LEVEL,
    LOG_CRITICAL_LEVEL
};

inline const char* _log_level_str(int level) {
    switch (level) {
        case LOG_TRACE_LEVEL:    return "\033[36m"   "TRACE"    "\033[0m";
        case LOG_DEBUG_LEVEL:    return "\033[34m"   "DEBUG"    "\033[0m";
        case LOG_INFO_LEVEL:     return "\033[32m"   "INFO"     "\033[0m";
        case LOG_WARN_LEVEL:     return "\033[33m"   "WARN"     "\033[0m";
        case LOG_ERR_LEVEL:      return "\033[31m"   "ERROR"    "\033[0m";
        case LOG_CRITICAL_LEVEL: return "\033[1;31m" "CRITICAL" "\033[0m";
        default:                 return              "UNKNOWN";
    }
}

inline void _log(int level, const char* format, va_list args) {
    printf("[%s] ", _log_level_str(level));
    vprintf(format, args);
    printf("\n");
}

inline void log_trace([[maybe_unused]] const char* format, ...) {
#ifndef DISABLE_LOG_TRACE
    va_list args;
    va_start(args, format);
    _log(LOG_TRACE_LEVEL, format, args);
    va_end(args);
#endif
}

inline void log_debug([[maybe_unused]] const char* format, ...) {
#ifndef DISABLE_LOG_DEBUG
    va_list args;
    va_start(args, format);
    _log(LOG_DEBUG_LEVEL, format, args);
    va_end(args);
#endif
}

inline void log_info([[maybe_unused]] const char* format, ...) {
#ifndef DISABLE_LOG_INFO
    va_list args;
    va_start(args, format);
    _log(LOG_INFO_LEVEL, format, args);
    va_end(args);
#endif
}

inline void log_warn([[maybe_unused]] const char* format, ...) {
#ifndef DISABLE_LOG_WARN
    va_list args;
    va_start(args, format);
    _log(LOG_WARN_LEVEL, format, args);
    va_end(args);
#endif
}

inline void log_err([[maybe_unused]] const char* format, ...) {
#ifndef DISABLE_LOG_ERR
    va_list args;
    va_start(args, format);
    _log(LOG_ERR_LEVEL, format, args);
    va_end(args);
#endif
}

inline void log_critical([[maybe_unused]] const char* format, ...) {
#ifndef DISABLE_LOG_CRITICAL
    va_list args;
    va_start(args, format);
    _log(LOG_CRITICAL_LEVEL, format, args);
    va_end(args);
#endif
}

}
