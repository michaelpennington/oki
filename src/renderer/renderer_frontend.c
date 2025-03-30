#include "renderer_frontend.h"

#include "renderer/renderer_types.h"
#include "renderer_backend.h"

#include "core/kmemory.h"
#include "core/logger.h"

static renderer_backend *backend = nullptr;

bool renderer_initialize(const char *application_name,
                         struct platform_state *plat_state) {
  backend = kallocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);
  backend->frame_number = 0;

  if (!renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state,
                               backend)) {
    kfatal("Failed to create renderer backend!");
    return false;
  }

  if (!backend->initialize(backend, application_name, plat_state)) {
    kfatal("Renderer backend failed to initialize!");
    return false;
  }

  return true;
}

bool renderer_begin_frame(f32 delta_time) {
  return backend->begin_frame(backend, delta_time);
}

bool renderer_end_frame(f32 delta_time) {
  bool result = backend->end_frame(backend, delta_time);
  backend->frame_number++;
  return result;
}

void renderer_shutdown() {
  backend->shutdown(backend);
  kfree(backend);
  backend = nullptr;
}

bool renderer_draw_frame(render_packet *packet) {
  if (renderer_begin_frame(packet->delta_time)) {
    bool result = renderer_end_frame(packet->delta_time);

    if (!result) {
      kerror("Renderer end frame failed!");
      return false;
    }
  }

  return true;
}
