#include "vulkan_backend.h"

#include "core/logger.h"
#include "vulkan_types.h"

static vulkan_context context;

bool vulkan_renderer_backend_initialize(struct renderer_backend *backend,
                                        const char *application_name,
                                        struct platform_state *plat_state) {
  // TODO: custom allocator
  context.allocator = nullptr;

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_4,
      .pApplicationName = application_name,
      .pEngineName = "Oki Engine",
      .engineVersion =
          VK_MAKE_VERSION(KVERSION_MAJOR, KVERSION_MINOR, KVERSION_PATCH),
  };

  VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = 0,
      .ppEnabledExtensionNames = nullptr,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
  };

  VkResult result =
      vkCreateInstance(&create_info, context.allocator, &context.instance);
  if (result != VK_SUCCESS) {
    kfatal("vkCreateInstance failed with result: %u", result);
    return false;
  }

  kinfo("Vulkan renderer initialized :)");
  return true;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend *backend) {
  (void)backend;
  vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_resized(struct renderer_backend *backend,
                                     u16 width, u16 height) {}

bool vulkan_renderer_backend_begin_frame(struct renderer_backend *backend,
                                         f32 delta_time) {
  (void)backend;
  (void)delta_time;
  return true;
}

bool vulkan_renderer_backend_end_frame(struct renderer_backend *backend,
                                       f32 delta_time) {
  (void)backend;
  (void)delta_time;
  return true;
}
