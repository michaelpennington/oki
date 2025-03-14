#pragma once

#include "defines.h"

typedef struct platform_config {
  const char *application_name;
  i32 x;
  i32 y;
  i32 width;
  i32 height;
} platform_config;

KAPI bool platform_startup(platform_config config);

KAPI void platform_shutdown();

KAPI bool platform_pump_messages();

void *platform_allocate(u64 size, bool aligned);
void platform_free(void *block, bool aligned);
void *platform_zero_memory(void *block, u64 size);
void *platform_copy_memory(void *dest, u64 size, const void *source);
void *platform_set_memory(i32 value, void *dest, u64 size);

f64 platform_get_absolute_time();

void platform_sleep(u32 ms);

#if defined(KBUILD_X11)
bool x11_platform_startup(void *state, platform_config config);
void x11_platform_shutdown();
bool x11_platform_pump_messages();
#endif

#if defined(KBUILD_WAYLAND)
bool wl_platform_startup(void *state, platform_config config);
void wl_platform_shutdown();
bool wl_platform_pump_messages();
#endif
