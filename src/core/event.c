#include "core/event.h"
#include "containers/darray.h"
#include "core/kmemory.h"
#include "core/logger.h"

typedef struct registered_event {
  void *listener;
  PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
  // darray
  registered_event *events;
} event_code_entry;

#define MAX_MESSAGE_CODES 16384

typedef struct event_system_state {
  event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

/**
 * Event system internal state.
 */
static bool is_initialized = false;
static event_system_state state;

bool event_initialize() {
  if (is_initialized) {
    return false;
  }

  kzero_memory(&state, sizeof(state));

  is_initialized = true;

  return true;
}
void event_shutdown() {
  for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
    if (state.registered[i].events) {
      darray_destroy(state.registered[i].events);
      state.registered[i].events = nullptr;
    }
  }
}

bool event_register(u16 code, void *listener, PFN_on_event on_event) {
  if (!is_initialized) {
    return false;
  }

  if (!state.registered[code].events) {
    state.registered[code].events = darray_create(registered_event);
  }

  registered_event *e;
  darray_for_each(state.registered[code].events, e) {
    if (e->listener == listener && e->callback == on_event) {
      kwarn("Tried to register event already registered");
      return false;
    }
  }

  registered_event event;
  event.listener = listener;
  event.callback = on_event;

  darray_push(&state.registered[code].events, event);

  return true;
}

bool event_unregister(u16 code, void *listener, PFN_on_event on_event) {
  if (!is_initialized) {
    return false;
  }

  if (!state.registered[code].events) {
    return false;
  }

  registered_event *e;
  u64 i;
  darray_enum_for_each(state.registered[code].events, e, i) {
    if (e->listener == listener && e->callback == on_event) {
      registered_event popped_event;
      darray_remove(state.registered[code].events, i, &popped_event);
      return true;
    }
  }

  return false;
}

bool event_fire(u16 code, void *sender, event_context data) {
  if (!is_initialized) {
    return false;
  }

  if (!state.registered[code].events) {
    return false;
  }

  registered_event *e;
  darray_for_each(state.registered[code].events, e) {
    if (e->callback(code, sender, e->listener, data)) {
      return true;
    }
  }

  return false;
}
