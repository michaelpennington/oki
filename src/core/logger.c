#include "core/logger.h"
#include "core/asserts.h"

#include <stdarg.h>
#include <stdio.h>

static const char *colors[] = {
    "\x1b[1;38;2;0;0;0;41m", "\x1b[4;31m", "\x1b[33m", "\x1b[34m", "\x1b[32m",
    "\x1b[38;2;208;208;208m"};

static const char *level_strings[] = {
    "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};

static const char *reset = "\x1b[0m";

bool initialize_logging() {
  // TODO: create log file
  return true;
}
void shutdown_logging() {
  // TODO: cleanup logging/write queued entries
}

#define BUFFER_SIZE 1600

void log_output(log_level level, ...) {
  char buffer[BUFFER_SIZE];
  va_list args;
  va_start(args, level);
  char *message = va_arg(args, char *);
  u32 written = snprintf(buffer, BUFFER_SIZE, "%s", level_strings[level]);
  written += vsnprintf(buffer + written, BUFFER_SIZE - written, message, args);
  printf("%s%s%s\n", colors[level], buffer, reset);
  snprintf(buffer + written, BUFFER_SIZE - written, "\n");

  va_end(args);
}

void report_assertion_failure(assertion_msg *msg) {
  log_output(LOG_LEVEL_FATAL,
             "Assertion Failure: %s, message: `%s`, in file: %s, line: %d",
             msg->expression, msg->msg, msg->file, msg->line_no);
}
