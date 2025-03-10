#pragma once

#include "defines.h"

typedef struct platform_config {
  const char *application_name;
  i32 x;
  i32 y;
  i32 width;
  i32 height;
} platform_config;

KAPI bool platform_startup(void *platform_state, platform_config config);

KAPI void platform_shutdown();

KAPI bool platform_pump_messages();

void *platform_allocate(u64 size, bool aligned);
void platform_free(void *block, bool aligned);
void *platform_zero_memory(void *block, u64 size);
void *platform_copy_memory(void *dest, u64 size, const void *source);
void *platform_set_memory(i32 value, void *dest, u64 size);

f64 platform_get_absolute_time();

void platform_sleep(u64 ms);
