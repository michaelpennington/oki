#include "core/application.h"
#include "core/event.h"
#include "core/input.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "game_types.h"
#include "platform/platform.h"

typedef struct application_state {
  game *game_inst;
  bool is_running;
  bool is_suspended;
  void *platform_state;
  i16 width;
  i16 height;
  f64 last_time;
} application_state;

static application_state app_state;
static bool initialized = false;

bool application_on_event(u16 code, void *sender, void *listener_inst,
                          event_context context);
bool application_on_key(u16 code, void *sender, void *listener_inst,
                        event_context context);

bool application_create(game *game_inst) {
  if (initialized) {
    kfatal("application_create called more than once");
    return false;
  }

  app_state.game_inst = game_inst;

  if (!initialize_logging()) {
    kfatal("Failed to initialize logging");
    return false;
  }

  input_initialize();

  // TODO: Remove
  const float pi = 3.14F;
  ktrace("Pi = %f", pi);
  kdebug("Pi = %f", pi);
  kinfo("Pi = %f", pi);
  kwarn("Pi = %f", pi);
  kerror("Pi = %f", pi);
  kfatal("Pi = %f", pi);

  app_state.is_running = true;
  app_state.is_suspended = false;

  if (!event_initialize()) {
    kerror("Event system failed initialization. Cannot continue");
    return false;
  }

  ktrace("Length of `%s` is %d", "hello", string_length("hello"));

  event_register(EVENT_CODE_APPLICATION_QUIT, nullptr, application_on_event);
  event_register(EVENT_CODE_KEY_PRESSED, nullptr, application_on_key);
  event_register(EVENT_CODE_KEY_RELEASED, nullptr, application_on_key);

  platform_config conf = {
      .width = game_inst->app_config.start_width,
      .height = game_inst->app_config.start_height,
      .application_name = game_inst->app_config.name,
      .x = game_inst->app_config.start_pos_x,
      .y = game_inst->app_config.start_pos_y,
  };

  if (!platform_startup(&app_state.platform_state, conf)) {
    kfatal("Platform failed to start. Shutting down");
    return false;
  }

  if (!app_state.game_inst->initialize(app_state.game_inst)) {
    kfatal("Game failed to initialize");
    return false;
  }

  app_state.game_inst->on_resize(app_state.game_inst, app_state.width,
                                 app_state.height);

  initialized = true;

  return true;
}

bool application_run() {
  print_memory_usage_str();
  while (app_state.is_running) {
    if (!platform_pump_messages()) {
      app_state.is_running = false;
    }

    if (!app_state.is_suspended) {
      if (!app_state.game_inst->update(app_state.game_inst, 0.0F)) {
        kfatal("Game update failed, shutting down");
        app_state.is_running = false;
        break;
      }

      if (!app_state.game_inst->render(app_state.game_inst, 0.0F)) {
        kfatal("Game render failed, shutting down");
        app_state.is_running = false;
        break;
      }

      input_update(0);
    }
  }
  app_state.is_running = false;
  event_unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, application_on_event);
  event_unregister(EVENT_CODE_KEY_PRESSED, nullptr, application_on_key);
  event_unregister(EVENT_CODE_KEY_RELEASED, nullptr, application_on_key);
  event_shutdown();

  platform_shutdown();
  return true;
}

bool application_on_event(u16 code, void *sender, void *listener_inst,
                          event_context context) {
  (void)sender;
  (void)listener_inst;
  (void)context;
  switch (code) {
  case EVENT_CODE_APPLICATION_QUIT: {
    kinfo("EVENT_CODE_APPLICATION_QUIT received, shutting down.");
    app_state.is_running = false;
    return true;
  }
  default:
    return false;
  }
}

bool application_on_key(u16 code, void *sender, void *listener_inst,
                        event_context context) {
  (void)sender;
  (void)listener_inst;
  (void)context;
  if (code == EVENT_CODE_KEY_PRESSED) {
    u16 key_code = context.data.u16[0];
    if (key_code == KKEY_ESCAPE) {
      event_context data = {};
      event_fire(EVENT_CODE_APPLICATION_QUIT, nullptr, data);
      return true;
    }
    if (key_code == KKEY_A) {
      kdebug("Explicit - A key pressed!");
    } else {
      kdebug("'%c' key pressed in window.", key_code);
    }
  } else if (code == EVENT_CODE_KEY_RELEASED) {
    u16 key_code = context.data.u16[0];
    if (key_code == KKEY_B) {
      kdebug("Explicit - B key released!");
    } else {
      kdebug("'%c' key released in window.", key_code);
    }
  }
  return false;
}
