#include "platform/platform_linux_wayland.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <dlfcn.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

static wayland_state *state_ptr;

enum pointer_event_mask {
  POINTER_EVENT_ENTER = 1 << 0,
  POINTER_EVENT_LEAVE = 1 << 1,
  POINTER_EVENT_MOTION = 1 << 2,
  POINTER_EVENT_BUTTON = 1 << 3,
  POINTER_EVENT_AXIS = 1 << 4,
  POINTER_EVENT_AXIS_SOURCE = 1 << 5,
  POINTER_EVENT_AXIS_STOP = 1 << 6,
  POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
};

static const u32 axis_events = POINTER_EVENT_AXIS | POINTER_EVENT_AXIS_SOURCE |
                               POINTER_EVENT_AXIS_STOP |
                               POINTER_EVENT_AXIS_DISCRETE;

#define wl_proxy_marshal_flags state_ptr->fns.wl_proxy_marshal_flags
#define wl_proxy_get_version state_ptr->fns.wl_proxy_get_version
#define wl_proxy_add_listener state_ptr->fns.wl_proxy_add_listener

#include "wayland-client-protocol.h"
#include "wayland-xdg-shell-client-protocol.h"

#include "wayland-client-protocol-code.h"
#include "wayland-xdg-shell-client-protocol-code.h"

#undef wl_proxy_marshal_flags
#undef wl_proxy_get_version
#undef wl_proxy_add_listener

bool init_wpfns(void) {
  wpfns *fns = &state_ptr->fns;

  state_ptr->wl_client_handle =
      dlopen("libwayland-client.so", RTLD_LAZY | RTLD_LOCAL);
  if (state_ptr->wl_client_handle == NULL) {
    kfatal("Failed to load wayland-client");
    return false;
  }

  fns->wl_display_connect = (PFN_wl_display_connect)dlsym(
      state_ptr->wl_client_handle, "wl_display_connect");
  fns->wl_proxy_marshal_flags = (PFN_wl_proxy_marshal_flags)dlsym(
      state_ptr->wl_client_handle, "wl_proxy_marshal_flags");
  fns->wl_proxy_get_version = (PFN_wl_proxy_get_version)dlsym(
      state_ptr->wl_client_handle, "wl_proxy_get_version");
  fns->wl_proxy_add_listener = (PFN_wl_proxy_add_listener)dlsym(
      state_ptr->wl_client_handle, "wl_proxy_add_listener");
  fns->wl_display_roundtrip = (PFN_wl_display_roundtrip)dlsym(
      state_ptr->wl_client_handle, "wl_display_roundtrip");
  fns->wl_display_disconnect = (PFN_wl_display_disconnect)dlsym(
      state_ptr->wl_client_handle, "wl_display_disconnect");
  fns->wl_display_dispatch_pending = (PFN_wl_display_dispatch_pending)dlsym(
      state_ptr->wl_client_handle, "wl_display_dispatch_pending");
  fns->wl_display_dispatch = (PFN_wl_display_dispatch)dlsym(
      state_ptr->wl_client_handle, "wl_display_dispatch");
  fns->wl_display_get_fd = (PFN_wl_display_get_fd)dlsym(
      state_ptr->wl_client_handle, "wl_display_get_fd");
  fns->wl_display_flush = (PFN_wl_display_flush)dlsym(
      state_ptr->wl_client_handle, "wl_display_flush");
  fns->wl_display_prepare_read = (PFN_wl_display_prepare_read)dlsym(
      state_ptr->wl_client_handle, "wl_display_prepare_read");
  fns->wl_display_read_events = (PFN_wl_display_read_events)dlsym(
      state_ptr->wl_client_handle, "wl_display_read_events");
  fns->wl_display_cancel_read = (PFN_wl_display_cancel_read)dlsym(
      state_ptr->wl_client_handle, "wl_display_cancel_read");

  if (!fns->wl_display_connect || !fns->wl_proxy_marshal_flags ||
      !fns->wl_proxy_get_version || !fns->wl_proxy_add_listener ||
      !fns->wl_display_roundtrip || !fns->wl_display_disconnect ||
      !fns->wl_display_dispatch_pending || !fns->wl_display_dispatch ||
      !fns->wl_display_flush || !fns->wl_display_get_fd ||
      !fns->wl_display_prepare_read || !fns->wl_display_cancel_read ||
      !fns->wl_display_read_events) {
    kfatal("Couldn't get required wayland fns");
    return false;
  }

  return true;
}

static void handle_toplevel_configure(void *data,
                                      struct xdg_toplevel *xdg_toplevel,
                                      i32 width, i32 height,
                                      struct wl_array *states) {
  (void)states;
  (void)xdg_toplevel;
  wayland_state *state = data;
  if (width > 0) {
    state->width = width;
  }
  if (height > 0) {
    state->height = height;
  }
  if (width > 0 && height > 0) {
    /* event_context context; */
    /* context.data.u16[0] = (u16)width; */
    /* context.data.u16[1] = (u16)height; */
    /* event_fire(EVENT_CODE_RESIZED, 0, context); */
  }
}

static void handle_toplevel_close(void *data,
                                  struct xdg_toplevel *xdg_toplevel) {
  (void)xdg_toplevel;
  wayland_state *state = data;
  state->quit_flagged = true;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = handle_toplevel_configure,
    .close = handle_toplevel_close,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  u32 serial) {
  wayland_state *state = data;
  xdg_surface_ack_configure(xdg_surface, serial);

  if (state->xdg_surface_configured) {
    if (state->width > 0 && state->height > 0) {
      /* event_context context; */
      /* context.data.u16[0] = (u16)state->width; */
      /* context.data.u16[1] = (u16)state->height; */
      /* event_fire(EVENT_CODE_RESIZED, 0, context); */
    }
  }
  wl_surface_commit(state->wl_surface);
  state->xdg_surface_configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                             u32 serial) {
  (void)data;
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void wl_pointer_enter(void *data, struct wl_pointer *pointer, u32 serial,
                             struct wl_surface *surface, wl_fixed_t surface_x,
                             wl_fixed_t surface_y) {
  (void)pointer;
  (void)surface;
  wayland_state *state = data;
  state->pointer_event.event_mask |= POINTER_EVENT_ENTER;
  state->pointer_event.serial = serial;
  state->pointer_event.surface_x = surface_x;
  state->pointer_event.surface_y = surface_y;
}

static void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer,
                             u32 serial, struct wl_surface *surface) {
  (void)surface;
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.serial = serial;
  state->pointer_event.event_mask |= POINTER_EVENT_LEAVE;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer,
                              u32 time, wl_fixed_t surface_x,
                              wl_fixed_t surface_y) {
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.event_mask |= POINTER_EVENT_MOTION;
  state->pointer_event.time = time;
  state->pointer_event.surface_x = surface_x;
  state->pointer_event.surface_y = surface_y;
}

static void wl_pointer_button(void *data, struct wl_pointer *wl_pointer,
                              u32 serial, u32 time, u32 button,
                              u32 button_state) {
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.event_mask |= POINTER_EVENT_BUTTON;
  state->pointer_event.time = time;
  state->pointer_event.serial = serial;
  state->pointer_event.button = button;
  state->pointer_event.state = button_state;
}

static void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, u32 time,
                            u32 axis, wl_fixed_t value) {
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS;
  state->pointer_event.time = time;
  state->pointer_event.axes[axis].valid = true;
  state->pointer_event.axes[axis].value = value;
}

static void wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer,
                                   u32 axis_source) {
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS_SOURCE;
  state->pointer_event.axis_source = axis_source;
}

static void wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer,
                                 u32 time, u32 axis) {
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.time = time;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS_STOP;
  state->pointer_event.axes[axis].valid = true;
}

static void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer,
                                     u32 axis, i32 discrete) {
  (void)wl_pointer;
  wayland_state *state = data;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS_DISCRETE;
  state->pointer_event.axes[axis].valid = true;
  state->pointer_event.axes[axis].discrete = discrete;
}

static void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
  (void)wl_pointer;
  wayland_state *state = data;
  pointer_event *event = &state->pointer_event;

  if (event->event_mask & (POINTER_EVENT_ENTER | POINTER_EVENT_MOTION)) {
    /* input_process_mouse_move((i16)wl_fixed_to_int(event->surface_x), */
    /*                          (i16)wl_fixed_to_int(event->surface_y)); */
  }

  if (event->event_mask & POINTER_EVENT_BUTTON) {
    /* bool pressed = (bool)(event->state == WL_POINTER_BUTTON_STATE_PRESSED);
     */
    /* switch (event->button) { */
    /* case BTN_LEFT: */
    /*   input_process_button(BUTTON_LEFT, pressed); */
    /*   break; */
    /* case BTN_MIDDLE: */
    /*   input_process_button(BUTTON_MIDDLE, pressed); */
    /*   break; */
    /* case BTN_RIGHT: */
    /*   input_process_button(BUTTON_RIGHT, pressed); */
    /*   break; */
    /* } */
  }

  if (event->event_mask & axis_events && (int)event->axes[0].valid) {
    /* if (event->event_mask & POINTER_EVENT_AXIS) { */
    /*   input_process_mouse_wheel( */
    /*       wl_fixed_to_int(event->axes[0].value) >= 0 ? 1 : -1); */
    /* } */
    /* if (event->event_mask & POINTER_EVENT_AXIS_DISCRETE) { */
    /*   input_process_mouse_wheel(event->axes[0].discrete >= 0 ? 1 : -1); */
    /* } */
  }

  memset(event, 0, sizeof(*event));
}

static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
                               u32 format, i32 fd, u32 size) {
  (void)wl_keyboard;
  wayland_state *state = data;
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    return;
  }

  char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (map_shm == MAP_FAILED) { // NOLINT
    return;
  }

  struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
      state->xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
      XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map_shm, size);
  close(fd);

  struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
  xkb_keymap_unref(state->xkb_keymap);
  xkb_state_unref(state->xkb_state);
  state->xkb_keymap = xkb_keymap;
  state->xkb_state = xkb_state;
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
                              u32 serial, struct wl_surface *surface,
                              struct wl_array *wlkeys) {
  (void)wl_keyboard;
  (void)surface;
  (void)serial;
  wayland_state *state = data;
  u32 *key;
  /* keys new_key; */
  wl_array_for_each(key, wlkeys) {
    u32 sym = xkb_state_key_get_one_sym(state->xkb_state, *key + 8);
    (void)sym;
    /* new_key = translate_keycode(sym); */
    /* input_process_key(new_key, true); */
  }
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
                            u32 serial, u32 time, u32 key, u32 state) {
  (void)wl_keyboard;
  (void)time;
  (void)serial;
  (void)state;
  wayland_state *client_state = data;
  xkb_keysym_t sym =
      xkb_state_key_get_one_sym(client_state->xkb_state, key + 8);
  (void)sym;
  /* input_process_key(translate_keycode(sym), */
  /*                   (bool)(state == WL_KEYBOARD_KEY_STATE_PRESSED)); */
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
                              u32 serial, struct wl_surface *surface) {
  (void)data;
  (void)wl_keyboard;
  (void)serial;
  (void)surface;
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                                  u32 serial, u32 mods_depressed,
                                  u32 mods_latched, u32 mods_locked,
                                  u32 group) {
  (void)wl_keyboard;
  (void)serial;
  (void)data;
  (void)mods_depressed;
  (void)mods_latched;
  (void)mods_locked;
  (void)group;
  // internal_state *state = data;
  // xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched,
  //                       mods_locked, 0, 0, group);
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                    i32 rate, i32 delay) {
  (void)data;
  (void)wl_keyboard;
  (void)rate;
  (void)delay;
}

static const struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
};

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

static void wl_seat_capabilities(void *data, struct wl_seat *wl_seat,
                                 u32 capabilities) {
  wayland_state *state = data;
  (void)data;
  (void)wl_seat;

  i32 have_pointer = (i32)capabilities & WL_SEAT_CAPABILITY_POINTER;

  if (have_pointer && state->pointer == NULL) {
    state->pointer = wl_seat_get_pointer(state->seat);
    wl_pointer_add_listener(state->pointer, &wl_pointer_listener, state);
  } else if (!have_pointer && state->pointer != NULL) {
    wl_pointer_release(state->pointer);
    state->pointer = nullptr;
  }

  i32 have_keyboard = (i32)capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

  if (have_keyboard && state->keyboard == NULL) {
    state->keyboard = wl_seat_get_keyboard(state->seat);
    wl_keyboard_add_listener(state->keyboard, &wl_keyboard_listener, state);
  } else if (!have_keyboard && state->keyboard != NULL) {
    wl_keyboard_release(state->keyboard);
    state->keyboard = nullptr;
  }
}

static void wl_seat_name(void *data, struct wl_seat *wl_seat,
                         const char *name) {
  (void)wl_seat;
  (void)data;
  kdebug("Seat name: %s\n", name);
  (void)name;
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name = wl_seat_name,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   u32 name, const char *interface,
                                   u32 version) {
  wayland_state *state = data;
  (void)version;
  // printf("interface: %s\n", interface);
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, 4);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    state->wm_base =
        wl_registry_bind(registry, name, &xdg_wm_base_interface, 2);
    xdg_wm_base_add_listener(state->wm_base, &xdg_wm_base_listener, state);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    state->seat =
        wl_registry_bind(registry, name, &wl_seat_interface, 7); // NOLINT
    wl_seat_add_listener(state->seat, &wl_seat_listener, state);
  }
}

static void registry_handle_global_remove(void *data,
                                          struct wl_registry *registry,
                                          u32 name) {
  (void)data;
  (void)registry;
  (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

bool wl_platform_startup(void *state, platform_config config) { // NOLINT
  state_ptr = state;

  state_ptr->xdg_surface_configured = false;
  state_ptr->quit_flagged = false;

  state_ptr->x = config.x;
  state_ptr->y = config.y;
  state_ptr->width = config.width;
  state_ptr->height = config.height;

  if (!init_wpfns()) {
    kfatal("Wayland not ok.");
    state_ptr = nullptr;
    return false;
  }

  state_ptr->display = state_ptr->fns.wl_display_connect(nullptr);
  if (state_ptr->display == nullptr) {
    state_ptr = nullptr;
    return false;
  }
  state_ptr->registry = wl_display_get_registry(state_ptr->display);
  state_ptr->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  wl_registry_add_listener(state_ptr->registry, &registry_listener, state);

  state_ptr->fns.wl_display_roundtrip(state_ptr->display);

  state_ptr->wl_fd = state_ptr->fns.wl_display_get_fd(state_ptr->display);

  kinfo("wl display fd: %d", state_ptr->wl_fd);
  struct pollfd wl_pollfd = {state_ptr->wl_fd, POLLIN, 0};
  state_ptr->wl_pollfd = wl_pollfd;

  state_ptr->wl_surface = wl_compositor_create_surface(state_ptr->compositor);
  state_ptr->xdg_surface =
      xdg_wm_base_get_xdg_surface(state_ptr->wm_base, state_ptr->wl_surface);
  xdg_surface_add_listener(state_ptr->xdg_surface, &xdg_surface_listener,
                           state);
  state_ptr->xdg_toplevel = xdg_surface_get_toplevel(state_ptr->xdg_surface);
  xdg_toplevel_set_title(state_ptr->xdg_toplevel, config.application_name);
  if (getenv("KFLOATING") == nullptr) {
    xdg_toplevel_set_app_id(state_ptr->xdg_toplevel, "roho");
  }
  xdg_toplevel_add_listener(state_ptr->xdg_toplevel, &xdg_toplevel_listener,
                            state);

  wl_surface_commit(state_ptr->wl_surface);

  kinfo("Using wayland platform");

  return true;
}

void wl_platform_shutdown(void) {
  xkb_context_unref(state_ptr->xkb_context);
  xkb_state_unref(state_ptr->xkb_state);
  xkb_keymap_unref(state_ptr->xkb_keymap);
  free(state_ptr->pointer);
  free(state_ptr->keyboard);
  free(state_ptr->xdg_toplevel);
  free(state_ptr->xdg_surface);
  free(state_ptr->wl_surface);
  free(state_ptr->compositor);
  free(state_ptr->wm_base);
  free(state_ptr->shm);
  free(state_ptr->seat);
  free(state_ptr->registry);
  state_ptr->fns.wl_display_disconnect(state_ptr->display);
  dlclose(state_ptr->wl_client_handle);
  state_ptr->wl_client_handle = nullptr;
  state_ptr->xkb_context = nullptr;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

bool wl_platform_pump_messages(void) {
  while (state_ptr->fns.wl_display_prepare_read(state_ptr->display) != 0) {
    state_ptr->fns.wl_display_dispatch_pending(state_ptr->display);
  }
  state_ptr->fns.wl_display_flush(state_ptr->display);

  i32 ret = poll(&state_ptr->wl_pollfd, 1, 0);
  if (ret <= 0) {
    state_ptr->fns.wl_display_cancel_read(state_ptr->display);
  } else {
    state_ptr->fns.wl_display_read_events(state_ptr->display);
  }

  while (state_ptr->fns.wl_display_dispatch_pending(state_ptr->display) > 0) {
  }

  state_ptr->wl_pollfd.revents = 0;
  return (bool)!state_ptr->quit_flagged;
}
