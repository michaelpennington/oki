#pragma once

#include "defines.h"

#if defined(KASSERTIONS_ENABLED)
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

typedef struct assertion_msg {
  const char *expression;
  const char *msg;
  const char *file;
  i32 line_no;
} assertion_msg;

KAPI void report_assertion_failure(assertion_msg *msg);

#define kassert(expr)                                                          \
  {                                                                            \
    if (expr) {                                                                \
    } else {                                                                   \
      assertion_msg msg = {.expression = #expr,                                \
                           .msg = "",                                          \
                           .file = __FILE__,                                   \
                           .line_no = __LINE__};                               \
      report_assertion_failure(&msg);                                          \
      debugBreak();                                                            \
    }                                                                          \
  }

#define kassert_msg(expr, message)                                             \
  {                                                                            \
    if (expr) {                                                                \
    } else {                                                                   \
      assertion_msg msg = {.expression = #expr,                                \
                           .msg = (message),                                   \
                           .file = __FILE__,                                   \
                           .line_no = __LINE__};                               \
      report_assertion_failure(&msg);                                          \
      debugBreak();                                                            \
    }                                                                          \
  }

#if defined(_DEBUG)

#define kassert_debug(expr)                                                    \
  {                                                                            \
    if (expr) {                                                                \
    } else {                                                                   \
      assertion_msg msg = {.expression = #expr,                                \
                           .msg = "",                                          \
                           .file = __FILE__,                                   \
                           .line_no = __LINE__};                               \
      report_assertion_failure(&msg);                                          \
      debugBreak();                                                            \
    }                                                                          \
  }

#define kassert_debug_msg(expr, message)                                       \
  {                                                                            \
    if (expr) {                                                                \
    } else {                                                                   \
      assertion_msg msg = {.expression = #expr,                                \
                           .msg = (message),                                   \
                           .file = __FILE__,                                   \
                           .line_no = __LINE__};                               \
      report_assertion_failure(&msg);                                          \
      debugBreak();                                                            \
    }                                                                          \
  }

#else
#define kassert_debug(expr)
#define kassert_debug_msg(expr, message)
#endif
#else
#define kassert(expr)
#define kassert_msg(expr, message)
#define kassert_debug(expr)
#define kassert_debug_msg(expr, message)
#endif
