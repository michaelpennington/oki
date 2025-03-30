#include "core/application.h"
#include "core/clock.h"
#include "core/event.h"
#include "core/input.h"
#include "core/kmemory.h"
#include "core/logger.h"
#include "game_types.h"
#include "platform/platform.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state {
  game *game_inst;
  bool is_running;
  bool is_suspended;
  void *platform_state;
  i16 width;
  i16 height;
  f64 last_time;
  clock clock;
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

  if (!renderer_initialize(game_inst->app_config.name,
                           app_state.platform_state)) {
    kfatal("Failed to initialize renderer!");
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

#define TARGET_FPS 60

bool application_run() {
  clock_start(&app_state.clock);
  clock_update(&app_state.clock);
  app_state.last_time = app_state.clock.elapsed;
  /* f64 running_time = 0; */
  /* u8 frame_count = 0; */
  f64 target_frame_seconds = 1.0F / TARGET_FPS;

  print_memory_usage_str();
  while (app_state.is_running) {
    if (!platform_pump_messages()) {
      app_state.is_running = false;
    }

    if (!app_state.is_suspended) {
      clock_update(&app_state.clock);
      f64 current_time = app_state.clock.elapsed;
      f64 delta = (current_time - app_state.last_time);
      f64 frame_start_time = platform_get_absolute_time();

      if (!app_state.game_inst->update(app_state.game_inst, (f32)delta)) {
        kfatal("Game update failed, shutting down");
        app_state.is_running = false;
        break;
      }

      if (!app_state.game_inst->render(app_state.game_inst, 0.0F)) {
        kfatal("Game render failed, shutting down");
        app_state.is_running = false;
        break;
      }

      render_packet packet;
      packet.delta_time = delta;
      renderer_draw_frame(&packet);

      f64 frame_end_time = platform_get_absolute_time();
      f64 frame_elapsed_time = frame_end_time - frame_start_time;
      /* running_time += frame_elapsed_time; */
      f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

      if (remaining_seconds > 0) {
        u64 remaining_ms = (remaining_seconds * 1000);

        bool limit_frames = false;
        if (remaining_ms > 0 && limit_frames) {
          platform_sleep(remaining_ms - 1);
        }

        /* frame_count++; */
      }

      input_update(delta);

      app_state.last_time = current_time;
    }
  }
  app_state.is_running = false;
  event_unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, application_on_event);
  event_unregister(EVENT_CODE_KEY_PRESSED, nullptr, application_on_key);
  event_unregister(EVENT_CODE_KEY_RELEASED, nullptr, application_on_key);
  renderer_shutdown();
  event_shutdown();
  input_shutdown();
  shutdown_logging();
  shutdown_memory();

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
