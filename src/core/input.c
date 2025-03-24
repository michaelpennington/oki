#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"

typedef struct keyboard_state {
  bool keys[256];
} keyboard_state;

typedef struct mouse_state {
  i16 x;
  i16 y;
  bool buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state {
  keyboard_state keyboard_current;
  keyboard_state keyboard_previous;
  mouse_state mouse_current;
  mouse_state mouse_previous;
} input_state;

static bool initialized = false;
static input_state state = {};

void input_initialize() {
  kzero_memory(&state, sizeof(input_state));
  initialized = true;
  kinfo("Input subsystem initialized");
}

void input_shutdown() { initialized = false; }

void input_update(f64 delta_time) {
  (void)delta_time;
  if (!initialized) {
    return;
  }

  kcopy_memory(&state.keyboard_previous, &state.keyboard_current,
               sizeof(keyboard_state));
  kcopy_memory(&state.mouse_previous, &state.mouse_current,
               sizeof(mouse_state));
}

bool input_is_key_down(keys key) { return state.keyboard_current.keys[key]; }

bool input_is_key_up(keys key) { return !state.keyboard_current.keys[key]; }

bool input_was_key_down(keys key) { return state.keyboard_previous.keys[key]; }

bool input_was_key_up(keys key) { return !state.keyboard_previous.keys[key]; }

void input_process_key(keys key, bool pressed) {
  if (state.keyboard_current.keys[key] != pressed) {
    state.keyboard_current.keys[key] = pressed;

    event_context context;
    context.data.u16[0] = key;
    event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED,
               nullptr, context);
  }
}

bool input_is_button_down(buttons button) {
  return state.mouse_current.buttons[button];
}

bool input_is_button_up(buttons button) {
  return !state.mouse_current.buttons[button];
}

bool input_was_button_down(buttons button) {
  return state.mouse_previous.buttons[button];
}

bool input_was_button_up(buttons button) {
  return !state.mouse_previous.buttons[button];
}

void input_get_mouse_position(i32 *x, i32 *y) {
  *x = state.mouse_current.x;
  *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(i32 *x, i32 *y) {
  *x = state.mouse_previous.x;
  *y = state.mouse_previous.y;
}

void input_process_button(buttons button, bool pressed) {
  if (state.mouse_current.buttons[button] != pressed) {
    state.mouse_current.buttons[button] = pressed;

    event_context context;
    context.data.u16[0] = button;
    event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED,
               nullptr, context);
  }
}
void input_process_mouse_move(i16 x, i16 y) {
  if (state.mouse_current.x != x || state.mouse_current.y != y) {
    kdebug("Mouse pos: %d, %d", x, y);
    state.mouse_current.x = x;
    state.mouse_current.y = y;

    event_context context;
    context.data.u16[0] = x;
    context.data.u16[1] = y;
    event_fire(EVENT_CODE_MOUSE_MOVED, nullptr, context);
  }
}
void input_process_mouse_wheel(i8 z_delta) {
  event_context context;
  context.data.i8[0] = z_delta;
  event_fire(EVENT_CODE_MOUSE_WHEEL, nullptr, context);
}
