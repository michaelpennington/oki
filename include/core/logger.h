#pragma once

#include "defines.h"

typedef enum log_level {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5,
} log_level;

bool initialize_logging();
void shutdown_logging();

KAPI void log_output(log_level level, ...);

#define kfatal(...) log_output(LOG_LEVEL_FATAL, __VA_ARGS__)
#define kerror(...) log_output(LOG_LEVEL_ERROR, __VA_ARGS__)

#if defined(LOG_WARN_ENABLED)
#define kwarn(...) log_output(LOG_LEVEL_WARN, __VA_ARGS__)
#else
#define kwarn(...)
#endif

#if defined(LOG_INFO_ENABLED)
#define kinfo(...) log_output(LOG_LEVEL_INFO, __VA_ARGS__)
#else
#define kinfo(...)
#endif

#if defined(LOG_DEBUG_ENABLED)
#define kdebug(...) log_output(LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define kdebug(...)
#endif

#if defined(LOG_TRACE_ENABLED)
#define ktrace(...) log_output(LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define ktrace(...)
#endif
