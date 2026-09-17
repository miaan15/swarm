package engine_core

import "core:fmt"

log_level :: enum {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
}

DISABLE_LOG_TRACE :: #config(DISABLE_LOG_TRACE, false)
DISABLE_LOG_DEBUG :: #config(DISABLE_LOG_DEBUG, false)
DISABLE_LOG_INFO  :: #config(DISABLE_LOG_INFO, false)
DISABLE_LOG_WARN  :: #config(DISABLE_LOG_WARN, false)
DISABLE_LOG_ERROR :: #config(DISABLE_LOG_ERROR, false)

log_level_str :: proc(level: log_level) -> string {
    switch level {
    case .Trace: return "\033[36mTRACE\033[0m"
    case .Debug: return "\033[34mDEBUG\033[0m"
    case .Info:  return "\033[32mINFO\033[0m"
    case .Warn:  return "\033[33mWARN\033[0m"
    case .Error: return "\033[31mERROR\033[0m"
    }
    return "UNKNOWN"
}

_log :: proc(level: log_level, format: string, args: ..any) {
    fmt.printf("[%s] ", log_level_str(level))
    fmt.printf(format, ..args)
    fmt.println()
}

log_trace :: proc(format: string, args: ..any) {
    when !DISABLE_LOG_TRACE {
        _log(.Trace, format, ..args)
    }
}

log_debug :: proc(format: string, args: ..any) {
    when !DISABLE_LOG_DEBUG {
        _log(.Debug, format, ..args)
    }
}

log_info :: proc(format: string, args: ..any) {
    when !DISABLE_LOG_INFO {
        _log(.Info, format, ..args)
    }
}

log_warn :: proc(format: string, args: ..any) {
    when !DISABLE_LOG_WARN {
        _log(.Warn, format, ..args)
    }
}

log_error :: proc(format: string, args: ..any) {
    when !DISABLE_LOG_ERROR {
        _log(.Error, format, ..args)
    }
}
