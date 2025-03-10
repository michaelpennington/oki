#pragma once

#include "defines.h"

#include <sys/poll.h>

#include <wayland-client-core.h>

typedef struct pointer_event {
  u32 event_mask;
  wl_fixed_t surface_x, surface_y;
  u32 button;
  u32 state;
  u32 time;
  u32 serial;
  struct {
    bool valid;
    wl_fixed_t value;
    i32 discrete;
  } axes[2];
  u32 axis_source;
} pointer_event;

typedef void *(*PFN_wl_display_connect)(const char *name);
typedef struct wl_proxy *(*PFN_wl_proxy_marshal_flags)(
    struct wl_proxy *proxy, u32 opcode, const struct wl_interface *interface,
    u32 version, u32 flags, ...);
typedef u32 (*PFN_wl_proxy_get_version)(struct wl_proxy *proxy);
typedef i32 (*PFN_wl_proxy_add_listener)(struct wl_proxy *proxy,
                                         void (**implementation)(void),
                                         void *data);
typedef i32 (*PFN_wl_display_roundtrip)(struct wl_display *display);
typedef void (*PFN_wl_display_disconnect)(struct wl_display *display);
typedef i32 (*PFN_wl_display_dispatch_pending)(struct wl_display *display);
typedef i32 (*PFN_wl_display_dispatch)(struct wl_display *display);
typedef i32 (*PFN_wl_display_flush)(struct wl_display *display);
typedef i32 (*PFN_wl_display_get_fd)(struct wl_display *display);
typedef i32 (*PFN_wl_display_prepare_read)(struct wl_display *display);
typedef i32 (*PFN_wl_display_read_events)(struct wl_display *display);
typedef void (*PFN_wl_display_cancel_read)(struct wl_display *display);

typedef struct wpfns {
  PFN_wl_display_connect wl_display_connect;
  PFN_wl_proxy_marshal_flags wl_proxy_marshal_flags;
  PFN_wl_proxy_get_version wl_proxy_get_version;
  PFN_wl_proxy_add_listener wl_proxy_add_listener;
  PFN_wl_display_roundtrip wl_display_roundtrip;
  PFN_wl_display_disconnect wl_display_disconnect;
  PFN_wl_display_dispatch_pending wl_display_dispatch_pending;
  PFN_wl_display_dispatch wl_display_dispatch;
  PFN_wl_display_flush wl_display_flush;
  PFN_wl_display_get_fd wl_display_get_fd;
  PFN_wl_display_prepare_read wl_display_prepare_read;
  PFN_wl_display_read_events wl_display_read_events;
  PFN_wl_display_cancel_read wl_display_cancel_read;
} wpfns;

typedef struct wayland_state {
  // Handles
  void *wl_client_handle;
  void *wl_cursor_handle;

  // Globals
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct xdg_wm_base *wm_base;
  struct wl_shm *shm;
  struct wl_seat *seat;

  // Objects
  struct wl_surface *wl_surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct wl_pointer *pointer;
  struct wl_keyboard *keyboard;
  i32 wl_fd;
  struct pollfd wl_pollfd;
  pointer_event pointer_event;
  struct xkb_context *xkb_context;
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;
  struct wl_cursor_theme *wl_cursor_theme;
  struct wl_cursor *default_cursor;

  i32 width;
  i32 height;
  i32 x;
  i32 y;
  bool xdg_surface_configured;
  bool quit_flagged;

  wpfns fns;
} wayland_state;
