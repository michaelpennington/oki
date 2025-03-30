#include "renderer_backend.h"
#include "core/asserts.h"
#include "core/logger.h"
#include "vulkan/vulkan_backend.h"

static const char *renderer_strings[3] = {"VULKAN", "OPENGL", "DIRECTX"};

bool renderer_backend_create(renderer_backend_type type,
                             struct platform_state *plat_state,
                             renderer_backend *out_renderer_backend) {
  out_renderer_backend->plat_state = plat_state;

  if (type == RENDERER_BACKEND_TYPE_VULKAN) {
    out_renderer_backend->initialize = vulkan_renderer_backend_initialize;
    out_renderer_backend->shutdown = vulkan_renderer_backend_shutdown;
    out_renderer_backend->begin_frame = vulkan_renderer_backend_begin_frame;
    out_renderer_backend->end_frame = vulkan_renderer_backend_end_frame;
    out_renderer_backend->resized = vulkan_renderer_backend_resized;

    return true;
  }

  kassert(type < 3);

  kfatal("{} is not a supported renderer backend!", renderer_strings[type]);
  return false;
}

void renderer_backend_destroy(renderer_backend *renderer_backend) {
  renderer_backend->initialize = nullptr;
  renderer_backend->shutdown = nullptr;
  renderer_backend->begin_frame = nullptr;
  renderer_backend->end_frame = nullptr;
  renderer_backend->resized = nullptr;
}
