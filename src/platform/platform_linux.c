#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/input.h"
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
  *plat_state = platform_allocate(sizeof(platform_state), false);
  platform_zero_memory(*plat_state, sizeof(platform_state));
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

void *platform_copy_memory(void *dest, const void *source, u64 size) {
  return memcpy(dest, source, size);
}

void *platform_set_memory(void *dest, i32 value, u64 size) {
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

keys translate_keycode(u32 xkb_keycode) {
  switch (xkb_keycode) {
  case XKB_KEY_BackSpace:
    return KKEY_BACKSPACE;
  case XKB_KEY_Return:
    return KKEY_ENTER;
  case XKB_KEY_Pause:
    return KKEY_PAUSE;
  case XKB_KEY_Tab:
    return KKEY_TAB;
  case XKB_KEY_Caps_Lock:
    return KKEY_CAPITAL;
  case XKB_KEY_Escape:
    return KKEY_ESCAPE;
  case XKB_KEY_Mode_switch:
    return KKEY_MODECHANGE;
  case XKB_KEY_space:
    return KKEY_SPACE;
  case XKB_KEY_Prior:
    return KKEY_PRIOR;
  case XKB_KEY_Next:
    return KKEY_NEXT;
  case XKB_KEY_End:
    return KKEY_END;
  case XKB_KEY_Home:
    return KKEY_HOME;
  case XKB_KEY_Left:
    return KKEY_LEFT;
  case XKB_KEY_Right:
    return KKEY_RIGHT;
  case XKB_KEY_Up:
    return KKEY_UP;
  case XKB_KEY_Down:
    return KKEY_DOWN;
  case XKB_KEY_Select:
    return KKEY_SELECT;
  case XKB_KEY_Print:
    return KKEY_PRINT;
  case XKB_KEY_Execute:
    return KKEY_EXECUTE;
  case XKB_KEY_Insert:
    return KKEY_INSERT;
  case XKB_KEY_Delete:
    return KKEY_DELETE;
  case XKB_KEY_Help:
    return KKEY_HELP;
  case XKB_KEY_Super_L:
    return KKEY_LWIN;
  case XKB_KEY_Super_R:
    return KKEY_RWIN;
  case XKB_KEY_XF86Sleep:
    return KKEY_SLEEP;
  case XKB_KEY_KP_0:
    return KKEY_NUMPAD0;
  case XKB_KEY_KP_1:
    return KKEY_NUMPAD1;
  case XKB_KEY_KP_2:
    return KKEY_NUMPAD2;
  case XKB_KEY_KP_3:
    return KKEY_NUMPAD3;
  case XKB_KEY_KP_4:
    return KKEY_NUMPAD4;
  case XKB_KEY_KP_5:
    return KKEY_NUMPAD5;
  case XKB_KEY_KP_6:
    return KKEY_NUMPAD6;
  case XKB_KEY_KP_7:
    return KKEY_NUMPAD7;
  case XKB_KEY_KP_8:
    return KKEY_NUMPAD8;
  case XKB_KEY_KP_9:
    return KKEY_NUMPAD9;
  case XKB_KEY_KP_Multiply:
    return KKEY_MULTIPLY;
  case XKB_KEY_KP_Add:
    return KKEY_ADD;
  case XKB_KEY_KP_Subtract:
    return KKEY_SUBTRACT;
  case XKB_KEY_KP_Divide:
    return KKEY_DIVIDE;
  case XKB_KEY_KP_Separator:
    return KKEY_SEPARATOR;
  case XKB_KEY_KP_Decimal:
    return KKEY_DECIMAL;
  case XKB_KEY_F1:
    return KKEY_F1;
  case XKB_KEY_F2:
    return KKEY_F2;
  case XKB_KEY_F3:
    return KKEY_F3;
  case XKB_KEY_F4:
    return KKEY_F4;
  case XKB_KEY_F5:
    return KKEY_F5;
  case XKB_KEY_F6:
    return KKEY_F6;
  case XKB_KEY_F7:
    return KKEY_F7;
  case XKB_KEY_F8:
    return KKEY_F8;
  case XKB_KEY_F9:
    return KKEY_F9;
  case XKB_KEY_F10:
    return KKEY_F10;
  case XKB_KEY_F11:
    return KKEY_F11;
  case XKB_KEY_F12:
    return KKEY_F12;
  case XKB_KEY_F13:
    return KKEY_F13;
  case XKB_KEY_F14:
    return KKEY_F14;
  case XKB_KEY_F15:
    return KKEY_F15;
  case XKB_KEY_F16:
    return KKEY_F16;
  case XKB_KEY_F17:
    return KKEY_F17;
  case XKB_KEY_F18:
    return KKEY_F18;
  case XKB_KEY_F19:
    return KKEY_F19;
  case XKB_KEY_F20:
    return KKEY_F20;
  case XKB_KEY_F21:
    return KKEY_F21;
  case XKB_KEY_F22:
    return KKEY_F22;
  case XKB_KEY_F23:
    return KKEY_F23;
  case XKB_KEY_F24:
    return KKEY_F24;
  case XKB_KEY_Num_Lock:
    return KKEY_NUMLOCK;
  case XKB_KEY_Scroll_Lock:
    return KKEY_SCROLL;
  case XKB_KEY_KP_Equal:
    return KKEY_NUMPAD_EQUAL;
  case XKB_KEY_Shift_L:
    return KKEY_LSHIFT;
  case XKB_KEY_Shift_R:
    return KKEY_RSHIFT;
  case XKB_KEY_Control_L:
    return KKEY_LCONTROL;
  case XKB_KEY_Control_R:
    return KKEY_RCONTROL;
  case XKB_KEY_Menu:
    return KKEY_RMENU;
  case XKB_KEY_semicolon:
    return KKEY_SEMICOLON;
  case XKB_KEY_plus:
    return KKEY_PLUS;
  case XKB_KEY_comma:
    return KKEY_COMMA;
  case XKB_KEY_minus:
    return KKEY_MINUS;
  case XKB_KEY_period:
    return KKEY_PERIOD;
  case XKB_KEY_slash:
    return KKEY_SLASH;
  case XKB_KEY_grave:
    return KKEY_GRAVE;
  case XKB_KEY_a:
  case XKB_KEY_A:
    return KKEY_A;
  case XKB_KEY_b:
  case XKB_KEY_B:
    return KKEY_B;
  case XKB_KEY_c:
  case XKB_KEY_C:
    return KKEY_C;
  case XKB_KEY_d:
  case XKB_KEY_D:
    return KKEY_D;
  case XKB_KEY_e:
  case XKB_KEY_E:
    return KKEY_E;
  case XKB_KEY_f:
  case XKB_KEY_F:
    return KKEY_F;
  case XKB_KEY_g:
  case XKB_KEY_G:
    return KKEY_G;
  case XKB_KEY_h:
  case XKB_KEY_H:
    return KKEY_H;
  case XKB_KEY_i:
  case XKB_KEY_I:
    return KKEY_I;
  case XKB_KEY_j:
  case XKB_KEY_J:
    return KKEY_J;
  case XKB_KEY_k:
  case XKB_KEY_K:
    return KKEY_K;
  case XKB_KEY_l:
  case XKB_KEY_L:
    return KKEY_L;
  case XKB_KEY_m:
  case XKB_KEY_M:
    return KKEY_M;
  case XKB_KEY_n:
  case XKB_KEY_N:
    return KKEY_N;
  case XKB_KEY_o:
  case XKB_KEY_O:
    return KKEY_O;
  case XKB_KEY_p:
  case XKB_KEY_P:
    return KKEY_P;
  case XKB_KEY_q:
  case XKB_KEY_Q:
    return KKEY_Q;
  case XKB_KEY_r:
  case XKB_KEY_R:
    return KKEY_R;
  case XKB_KEY_s:
  case XKB_KEY_S:
    return KKEY_S;
  case XKB_KEY_t:
  case XKB_KEY_T:
    return KKEY_T;
  case XKB_KEY_u:
  case XKB_KEY_U:
    return KKEY_U;
  case XKB_KEY_v:
  case XKB_KEY_V:
    return KKEY_V;
  case XKB_KEY_w:
  case XKB_KEY_W:
    return KKEY_W;
  case XKB_KEY_x:
  case XKB_KEY_X:
    return KKEY_X;
  case XKB_KEY_y:
  case XKB_KEY_Y:
    return KKEY_Y;
  case XKB_KEY_z:
  case XKB_KEY_Z:
    return KKEY_Z;
  case XKB_KEY_bracketleft:
    return KKEY_LBRACKET;
  case XKB_KEY_bracketright:
    return KKEY_RBRACKET;
  default:
    kwarn("Warning, key %lu not a valid key", xkb_keycode);
    return 0;
  }
}
