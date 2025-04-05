#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "platform/platform_linux_x11.h"
#include "vulkan/vulkan_xcb.h"

static x11_state *state_ptr;

bool init_xpfns(void) {
  xpfns *fns = &state_ptr->fns;

  state_ptr->x11_xcb_handle = dlopen("libX11-xcb.so", RTLD_LAZY | RTLD_LOCAL);
  if (state_ptr->x11_xcb_handle == NULL) {
    kerror("Failed to find x11-xcb library");
    return false;
  }

  state_ptr->x11_handle = dlopen("libX11.so", RTLD_LAZY | RTLD_LOCAL);
  if (state_ptr->x11_handle == NULL) {
    kerror("Failed to find x11 library");
    return false;
  }

  state_ptr->xcb_handle = dlopen("libxcb.so", RTLD_LAZY | RTLD_LOCAL);
  if (state_ptr->xcb_handle == NULL) {
    kerror("Failed to find x11-xcb library");
    return false;
  }

  state_ptr->xkb_x11_handle =
      dlopen("libxkbcommon-x11.so", RTLD_LAZY | RTLD_LOCAL);
  if (state_ptr->xkb_x11_handle == NULL) {
    kerror("Failed to find xkb-x11 library");
    return false;
  }

  fns->x_open_display =
      (PFN_x_open_display)dlsym(state_ptr->x11_handle, "XOpenDisplay");
  fns->x_autorepeat_off =
      (PFN_x_autorepeat_off)dlsym(state_ptr->x11_handle, "XAutoRepeatOff");
  fns->x_autorepeat_on =
      (PFN_x_autorepeat_on)dlsym(state_ptr->x11_handle, "XAutoRepeatOn");
  fns->x_close_display =
      (PFN_x_close_display)dlsym(state_ptr->x11_handle, "XCloseDisplay");
  fns->xkb_keycode_to_keysym = (PFN_xkb_keycode_to_keysym)dlsym(
      state_ptr->x11_handle, "XkbKeycodeToKeysym");

  fns->x_get_xcb_connection = (PFN_x_get_xcb_connection)dlsym(
      state_ptr->x11_xcb_handle, "XGetXCBConnection");

  fns->xcb_connection_has_error = (PFN_xcb_connection_has_error)dlsym(
      state_ptr->xcb_handle, "xcb_connection_has_error");
  fns->xcb_get_setup =
      (PFN_xcb_get_setup)dlsym(state_ptr->xcb_handle, "xcb_get_setup");
  fns->xcb_setup_roots_iterator = (PFN_xcb_setup_roots_iterator)dlsym(
      state_ptr->xcb_handle, "xcb_setup_roots_iterator");
  fns->xcb_screen_next =
      (PFN_xcb_screen_next)dlsym(state_ptr->xcb_handle, "xcb_screen_next");
  fns->xcb_generate_id =
      (PFN_xcb_generate_id)dlsym(state_ptr->xcb_handle, "xcb_generate_id");
  fns->xcb_create_window =
      (PFN_xcb_create_window)dlsym(state_ptr->xcb_handle, "xcb_create_window");
  fns->xcb_change_property = (PFN_xcb_change_property)dlsym(
      state_ptr->xcb_handle, "xcb_change_property");
  fns->xcb_intern_atom =
      (PFN_xcb_intern_atom)dlsym(state_ptr->xcb_handle, "xcb_intern_atom");
  fns->xcb_intern_atom_reply = (PFN_xcb_intern_atom_reply)dlsym(
      state_ptr->xcb_handle, "xcb_intern_atom_reply");
  fns->xcb_map_window =
      (PFN_xcb_map_window)dlsym(state_ptr->xcb_handle, "xcb_map_window");
  fns->xcb_flush = (PFN_xcb_flush)dlsym(state_ptr->xcb_handle, "xcb_flush");
  fns->xcb_destroy_window = (PFN_xcb_destroy_window)dlsym(state_ptr->xcb_handle,
                                                          "xcb_destroy_window");
  fns->xcb_poll_for_event = (PFN_xcb_poll_for_event)dlsym(state_ptr->xcb_handle,
                                                          "xcb_poll_for_event");

  fns->xkb_x11_get_core_keyboard_device_id =
      (PFN_xkb_x11_get_core_keyboard_device_id)dlsym(
          state_ptr->xkb_x11_handle, "xkb_x11_get_core_keyboard_device_id");
  fns->xkb_x11_keymap_new_from_device =
      (PFN_xkb_x11_keymap_new_from_device)dlsym(
          state_ptr->xkb_x11_handle, "xkb_x11_keymap_new_from_device");
  fns->xkb_x11_state_new_from_device = (PFN_xkb_x11_state_new_from_device)dlsym(
      state_ptr->xkb_x11_handle, "xkb_x11_state_new_from_device");

  if (!fns->x_open_display || !fns->x_autorepeat_off || !fns->x_autorepeat_on ||
      !fns->x_get_xcb_connection || !fns->xcb_connection_has_error ||
      !fns->xcb_get_setup || !fns->xcb_setup_roots_iterator ||
      !fns->xcb_screen_next || !fns->xcb_generate_id ||
      !fns->xcb_create_window || !fns->xcb_change_property ||
      !fns->xcb_intern_atom || !fns->xcb_intern_atom_reply ||
      !fns->xcb_map_window || !fns->xcb_flush || !fns->xcb_destroy_window ||
      !fns->xcb_poll_for_event || !fns->xkb_keycode_to_keysym ||
      !fns->xkb_x11_get_core_keyboard_device_id ||
      !fns->xkb_x11_keymap_new_from_device ||
      !fns->xkb_x11_state_new_from_device) {
    kerror("Failed to find needed X11 functions");
    return false;
  }

  return true;
}

bool x11_platform_startup(void *state, platform_config config) {
  // Create internal state
  state_ptr = state;
  xpfns *fns = &state_ptr->fns;

  if (!init_xpfns()) {
    return false;
  }

  // Connect to X
  state_ptr->display = fns->x_open_display(nullptr);
  if (!state_ptr->display) {
    kfatal("Could not open display");
    return false;
  }

  fns->x_autorepeat_off(state_ptr->display);

  state_ptr->connection = fns->x_get_xcb_connection(state_ptr->display);

  if (fns->xcb_connection_has_error(state_ptr->connection)) {
    kfatal("Failed to connect to X server via XCB.");
    return false;
  }

  const struct xcb_setup_t *setup = fns->xcb_get_setup(state_ptr->connection);

  state_ptr->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  state_ptr->device_id =
      fns->xkb_x11_get_core_keyboard_device_id(state_ptr->connection);
  state_ptr->xkb_keymap = fns->xkb_x11_keymap_new_from_device(
      state_ptr->xkb_context, state_ptr->connection, state_ptr->device_id,
      XKB_KEYMAP_COMPILE_NO_FLAGS);
  state_ptr->xkb_state = fns->xkb_x11_state_new_from_device(
      state_ptr->xkb_keymap, state_ptr->connection, state_ptr->device_id);

  // Loop through screens
  xcb_screen_iterator_t it = fns->xcb_setup_roots_iterator(setup);
  i32 screen_p = 0;
  for (i32 k = screen_p; k > 0; k--) {
    fns->xcb_screen_next(&it);
  }

  // After screens have been looped through, assign it
  state_ptr->screen = it.data;

  // Allocate XID for window to be created
  state_ptr->window = fns->xcb_generate_id(state_ptr->connection);

  // Register event types
  u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

  // Listen for keyboard and mouse buttons
  u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS |
                     XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
                     XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE |
                     XCB_EVENT_MASK_POINTER_MOTION |
                     XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  u32 value_list[] = {state_ptr->screen->black_pixel, event_values};

  // Create the window
  // xcb_void_cookie_t cookie =
  fns->xcb_create_window(
      state_ptr->connection, XCB_COPY_FROM_PARENT, state_ptr->window,
      state_ptr->screen->root, (i16)config.x, (i16)config.y, config.width,
      config.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
      state_ptr->screen->root_visual, event_mask, value_list);

  // Change the title
  fns->xcb_change_property(state_ptr->connection, XCB_PROP_MODE_REPLACE,
                           state_ptr->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                           8, strlen(config.application_name),
                           config.application_name);
  fns->xcb_change_property(state_ptr->connection, XCB_PROP_MODE_REPLACE,
                           state_ptr->window, XCB_ATOM_WM_CLASS,
                           XCB_ATOM_STRING, 8, strlen("roho"), "roho");

  // Tell the server to notify when the window manager attempts to destroy the
  // window
  xcb_intern_atom_cookie_t wm_delete_cookie = fns->xcb_intern_atom(
      state_ptr->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
  xcb_intern_atom_cookie_t wm_protocols_cookie = fns->xcb_intern_atom(
      state_ptr->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
  xcb_intern_atom_reply_t *wm_delete_reply = fns->xcb_intern_atom_reply(
      state_ptr->connection, wm_delete_cookie, nullptr);
  xcb_intern_atom_reply_t *wm_protocols_reply = fns->xcb_intern_atom_reply(
      state_ptr->connection, wm_protocols_cookie, nullptr);
  state_ptr->wm_delete_win = wm_delete_reply->atom;
  state_ptr->wm_protocols = wm_protocols_reply->atom;

  free(wm_delete_reply);
  free(wm_protocols_reply);

  fns->xcb_change_property(state_ptr->connection, XCB_PROP_MODE_REPLACE,
                           state_ptr->window, state_ptr->wm_protocols, 4, 32, 1,
                           &state_ptr->wm_delete_win);

  // Map the window to the screen
  fns->xcb_map_window(state_ptr->connection, state_ptr->window);

  // Flush the stream
  i32 stream_result = fns->xcb_flush(state_ptr->connection);
  if (stream_result <= 0) {
    kfatal("Error occured when flushing stream: %d", stream_result);
    return false;
  }

  return true;
}

void x11_platform_shutdown(void) {
  // Turn key repeats back on
  state_ptr->fns.x_autorepeat_on(state_ptr->display);

  xkb_state_unref(state_ptr->xkb_state);
  xkb_keymap_unref(state_ptr->xkb_keymap);
  xkb_context_unref(state_ptr->xkb_context);

  state_ptr->fns.xcb_destroy_window(state_ptr->connection, state_ptr->window);

  state_ptr->fns.x_close_display(state_ptr->display);

  state_ptr->display = nullptr;

  dlclose(state_ptr->xkb_x11_handle);
  dlclose(state_ptr->x11_handle);
  dlclose(state_ptr->x11_xcb_handle);
  dlclose(state_ptr->xcb_handle);
  state_ptr->xkb_x11_handle = nullptr;
  state_ptr->x11_handle = nullptr;
  state_ptr->x11_xcb_handle = nullptr;
  state_ptr->xcb_handle = nullptr;
}

bool x11_platform_pump_messages(void) {
  xcb_generic_event_t *event;
  xcb_client_message_event_t *cm;

  bool quit_flagged = false;

  do {
    event = state_ptr->fns.xcb_poll_for_event(state_ptr->connection);
    if (event == nullptr) {
      break;
    }

    switch (event->response_type & ~0x80) {
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE: {
      xcb_key_press_event_t *kb_event = (xcb_key_press_event_t *)event;
      bool pressed = (bool)(event->response_type == XCB_KEY_PRESS);
      xkb_keycode_t code = kb_event->detail;
      xkb_keysym_t key_sym =
          xkb_state_key_get_one_sym(state_ptr->xkb_state, code);
      keys key = translate_keycode(key_sym);
      input_process_key(key, pressed);
    } break;
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE: {
      xcb_button_press_event_t *mouse_event = (xcb_button_press_event_t *)event;
      bool pressed = (bool)(event->response_type = XCB_BUTTON_PRESS);
      buttons mouse_button = BUTTON_MAX_BUTTONS;
      switch (mouse_event->detail) {
      case XCB_BUTTON_INDEX_1:
        mouse_button = BUTTON_LEFT;
        break;
      case XCB_BUTTON_INDEX_2:
        mouse_button = BUTTON_MIDDLE;
        break;
      case XCB_BUTTON_INDEX_3:
        mouse_button = BUTTON_RIGHT;
        break;
      default:
        break;
      }

      if (mouse_button != BUTTON_MAX_BUTTONS) {
        input_process_button(mouse_button, pressed);
      }
    } break;
    case XCB_MOTION_NOTIFY: {
      xcb_motion_notify_event_t *move_event =
          (xcb_motion_notify_event_t *)event;
      input_process_mouse_move(move_event->event_x, move_event->event_y);
    } break;
    case XCB_CONFIGURE_NOTIFY: {
      xcb_configure_notify_event_t *configure_event =
          (xcb_configure_notify_event_t *)event;

      (void)configure_event;
      event_context context;
      context.data.u16[0] = configure_event->width;
      context.data.u16[1] = configure_event->height;
      event_fire(EVENT_CODE_RESIZED, nullptr, context);
    } break;

    case XCB_CLIENT_MESSAGE: {
      cm = (xcb_client_message_event_t *)event;

      if (cm->data.data32[0] == state_ptr->wm_delete_win) {
        quit_flagged = true;
      }
    } break;
    default:
      break;
    }

    free(event);
  } while (event != nullptr);

  return (bool)!quit_flagged;
}

bool x11_platform_create_vulkan_surface(VkInstance instance,
                                        VkAllocationCallbacks *allocator,
                                        VkSurfaceKHR *surface) {
  VkXcbSurfaceCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .connection = state_ptr->connection,
      .window = state_ptr->window,
  };

  VkResult result =
      vkCreateXcbSurfaceKHR(instance, &create_info, allocator, surface);
  if (result != VK_SUCCESS) {
    kfatal("Vulkan surface creation failed!");
    return false;
  }

  state_ptr->vk_surface = *surface;

  return true;
}
