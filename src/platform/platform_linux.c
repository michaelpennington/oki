#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/logger.h"
#include "platform/platform.h"

const f64 nano = 0.000000001;
const u32 kilo = 1000;

#if defined(KBUILD_X11)
#include "platform/platform_linux_x11.h"
#endif

#if defined(KBUILD_WAYLAND)
#include "platform_linux_wayland.h"
#endif

typedef struct platform_state {
#if defined(KBUILD_X11)
  x11_state x11;
#endif
#if defined(KBUILD_WAYLAND)
  wayland_state wl;
#endif
  void (*platform_shutdown)();
  bool (*platform_pump_messages)();
} platform_state;

static platform_state *state_ptr;

#if defined(KBUILD_WAYLAND)
static bool wl_startup(platform_config config) {
  if (wl_platform_startup(&state_ptr->wl, config)) {
    state_ptr->platform_pump_messages = wl_platform_pump_messages;
    state_ptr->platform_shutdown = wl_platform_shutdown;
    return true;
  }
  return false;
}
#endif

#if defined(KBUILD_X11)
static bool x11_startup(platform_config config) {
  if (x11_platform_startup(&state_ptr->x11, config)) {
    state_ptr->platform_pump_messages = x11_platform_pump_messages;
    state_ptr->platform_shutdown = x11_platform_shutdown;
    return true;
  }
  return false;
}
#endif

bool platform_startup(void **plat_state, platform_config config) {
  *plat_state = malloc(sizeof(platform_state));
  state_ptr = *plat_state;
  if (!state_ptr) {
    return false;
  }

#if defined(KBUILD_WAYLAND) && defined(KBUILD_X11)
  char *backend_choice = getenv("KBACKEND");
  if (backend_choice != NULL) {
    if (strcmp(backend_choice, "wayland") == 0) {
      if (wl_startup(config)) {
        return true;
      }
      kfatal("Wayland specifically requested but failed to start.");
      return false;
    }
    if (strcmp(backend_choice, "x11") == 0) {
      if (x11_startup(config)) {
        return true;
      }
      kfatal("X11 specifically requested but failed to start.");
      return false;
    }
    kfatal("%s is not a valid backend on linux, use x11 or wayland.",
           backend_choice);
    return false;
  }
#endif

#if defined(KBUILD_WAYLAND)
  if (wl_startup(config)) {
    return true;
  }
  kfatal("Wayland could not start");
#endif

#if defined(KBUILD_X11)
  if (x11_startup(config)) {
    return true;
  }
  kfatal("X11 could not start");
#endif

  kfatal("No appropriate linux platforms found.");
  return false;
}

bool platform_pump_messages(void) {
  return state_ptr->platform_pump_messages();
}

void platform_shutdown(void) {
  if (state_ptr) {
    state_ptr->platform_shutdown();
    free(state_ptr);
    state_ptr = nullptr;
  }
}

void *platform_allocate(u64 size, bool aligned) {
  (void)aligned;
  return malloc(size);
}

void platform_free(void *block, bool aligned) {
  (void)aligned;
  free(block);
}
void *platform_zero_memory(void *block, u64 size) {
  return memset(block, 0, size);
}

void *platform_copy_memory(void *dest, u64 size, const void *source) {
  return memcpy(dest, source, size);
}

void *platform_set_memory(i32 value, void *dest, u64 size) {
  return memset(dest, value, size);
}

f64 platform_get_absolute_time(void) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (f64)now.tv_sec + ((f64)now.tv_nsec * nano);
}

void platform_sleep(u32 ms) {
  struct timespec ts;
  ts.tv_sec = ms / kilo;
  ts.tv_nsec = (u32)((ms % kilo) * kilo * kilo);
  nanosleep(&ts, nullptr);
}
