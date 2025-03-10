#pragma once

#include "defines.h"
#include <X11/XKBlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

// NOLINTBEGIN(readability-identifier-length)
typedef Display *(*PFN_x_open_display)(char *display_name);
typedef i32 (*PFN_x_autorepeat_off)(Display *display);
typedef i32 (*PFN_x_autorepeat_on)(Display *display);
typedef void (*PFN_x_close_display)(Display *display);
typedef xcb_connection_t *(*PFN_x_get_xcb_connection)(Display *display);
typedef i32 (*PFN_xcb_connection_has_error)(xcb_connection_t *connection);
typedef const struct xcb_setup_t *(*PFN_xcb_get_setup)(
    xcb_connection_t *connection);
typedef xcb_screen_iterator_t (*PFN_xcb_setup_roots_iterator)(
    const xcb_setup_t *setup);
typedef void (*PFN_xcb_screen_next)(xcb_screen_iterator_t *iter);
typedef xcb_window_t (*PFN_xcb_generate_id)(xcb_connection_t *connection);
typedef xcb_void_cookie_t (*PFN_xcb_create_window)(
    xcb_connection_t *connection, u8 depth, xcb_window_t wid,
    xcb_window_t parent, i16 x, i16 y, u16 width, u16 height, u16 border,
    u16 class, xcb_visualid_t visual, u32 value_mask, const void *value_list);
typedef xcb_void_cookie_t (*PFN_xcb_change_property)(
    xcb_connection_t *c, u8 mode, xcb_window_t window, xcb_atom_t property,
    xcb_atom_t type, u8 format, u32 data_len, const void *data);
typedef xcb_intern_atom_cookie_t (*PFN_xcb_intern_atom)(xcb_connection_t *c,
                                                        u8 only_if_exists,
                                                        u16 name_len,
                                                        const char *name);
typedef xcb_intern_atom_reply_t *(*PFN_xcb_intern_atom_reply)(
    xcb_connection_t *c, xcb_intern_atom_cookie_t cookie,
    xcb_generic_error_t **e);
typedef xcb_void_cookie_t (*PFN_xcb_map_window)(xcb_connection_t *c,
                                                xcb_window_t window);
typedef i32 (*PFN_xcb_flush)(xcb_connection_t *c);
typedef xcb_void_cookie_t (*PFN_xcb_destroy_window)(xcb_connection_t *c,
                                                    xcb_window_t window);
typedef xcb_generic_event_t *(*PFN_xcb_poll_for_event)(xcb_connection_t *c);
typedef KeySym (*PFN_xkb_keycode_to_keysym)(Display *display, KeyCode keycode,
                                            u32 group, u32 level);
typedef i32 (*PFN_xkb_x11_get_core_keyboard_device_id)(xcb_connection_t *c);
typedef struct xkb_keymap *(*PFN_xkb_x11_keymap_new_from_device)(
    struct xkb_context *context, xcb_connection_t *connection, i32 device_id,
    enum xkb_keymap_compile_flags flags);
typedef struct xkb_state *(*PFN_xkb_x11_state_new_from_device)(
    struct xkb_keymap *keymap, xcb_connection_t *connection, i32 device_id);
// NOLINTEND(readability-identifier-length)

typedef struct xpfns {
  // x11 functions
  PFN_x_open_display x_open_display;
  PFN_x_autorepeat_off x_autorepeat_off;
  PFN_x_autorepeat_on x_autorepeat_on;
  PFN_x_close_display x_close_display;
  PFN_xkb_keycode_to_keysym xkb_keycode_to_keysym;

  // xcb_x11 functions
  PFN_x_get_xcb_connection x_get_xcb_connection;

  // xcb functions
  PFN_xcb_connection_has_error xcb_connection_has_error;
  PFN_xcb_get_setup xcb_get_setup;
  PFN_xcb_setup_roots_iterator xcb_setup_roots_iterator;
  PFN_xcb_screen_next xcb_screen_next;
  PFN_xcb_generate_id xcb_generate_id;
  PFN_xcb_create_window xcb_create_window;
  PFN_xcb_change_property xcb_change_property;
  PFN_xcb_intern_atom xcb_intern_atom;
  PFN_xcb_intern_atom_reply xcb_intern_atom_reply;
  PFN_xcb_map_window xcb_map_window;
  PFN_xcb_flush xcb_flush;
  PFN_xcb_destroy_window xcb_destroy_window;
  PFN_xcb_poll_for_event xcb_poll_for_event;

  // xkb_x11 functions
  PFN_xkb_x11_get_core_keyboard_device_id xkb_x11_get_core_keyboard_device_id;
  PFN_xkb_x11_keymap_new_from_device xkb_x11_keymap_new_from_device;
  PFN_xkb_x11_state_new_from_device xkb_x11_state_new_from_device;
} xpfns;

typedef struct x11_state {
  void *x11_handle;
  void *x11_xcb_handle;
  void *xcb_handle;
  void *xkb_x11_handle;
  Display *display;
  xcb_connection_t *connection;
  xcb_window_t window;
  xcb_screen_t *screen;
  xcb_atom_t wm_protocols;
  xcb_atom_t wm_delete_win;

  struct xkb_context *xkb_context;
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;
  i32 device_id;

  xpfns fns;
} x11_state;
