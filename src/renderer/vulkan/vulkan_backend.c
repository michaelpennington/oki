#include "vulkan_backend.h"

#include "containers/darray.h"
#include "core/application.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "vulkan/vulkan_core.h"

#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_fence.h"
#include "vulkan_framebuffer.h"
#include "vulkan_platform.h"
#include "vulkan_render_pass.h"
#include "vulkan_swapchain.h"
#include "vulkan_types.h"

static vulkan_context context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;

#if defined(_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);
#endif

i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers();
void free_command_buffers();
void regenerate_framebuffers(vulkan_swapchain *swapchain,
                             vulkan_render_pass *render_pass);
void initialize_sync_objects();
void destroy_sync_objects();

bool vulkan_renderer_backend_initialize(struct renderer_backend *backend,
                                        const char *application_name,
                                        struct platform_state *plat_state) {
  (void)plat_state;
  (void)backend;
  // TODO: custom allocator
  context.allocator = nullptr;
  context.find_memory_index = find_memory_index;

  application_get_framebuffer_size(&cached_framebuffer_width,
                                   &cached_framebuffer_height);
  context.framebuffer_width =
      cached_framebuffer_width != 0 ? cached_framebuffer_width : 800;
  context.framebuffer_height =
      cached_framebuffer_height != 0 ? cached_framebuffer_height : 600;
  cached_framebuffer_width = 0;
  cached_framebuffer_height = 0;

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_4,
      .pApplicationName = application_name,
      .pEngineName = "Oki Engine",
      .engineVersion =
          VK_MAKE_VERSION(KVERSION_MAJOR, KVERSION_MINOR, KVERSION_PATCH),
  };

  const char **required_extensions = darray_create(const char *);
  darray_push(&required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
  platform_get_required_extension_names(&required_extensions);
#if defined(_DEBUG)
  darray_push(&required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  kdebug("Required extensions:");
  const char **extension;
  darray_for_each(required_extensions, extension) { kdebug(*extension); }
#endif

  const char **required_validation_layer_names = nullptr;

#if defined(_DEBUG)
  kinfo("Validation layers enabled, enumerating...");

  required_validation_layer_names = darray_create(const char *);
  darray_push(&required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");

  u32 available_layer_count = 0;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
  VkLayerProperties *available_layers =
      darray_with_capacity(VkLayerProperties, available_layer_count);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count,
                                              available_layers));
  darray_length_set(available_layers, available_layer_count);

  const char **needed_layer;
  darray_for_each(required_validation_layer_names, needed_layer) {
    kinfo("Searching for layer `%s`...", *needed_layer);
    bool found = false;
    VkLayerProperties *found_layer;
    darray_for_each(available_layers, found_layer) {
      kdebug("Found layer %s", found_layer->layerName);
      if (strings_equal(*needed_layer, found_layer->layerName)) {
        found = true;
        kinfo("Found.");
        break;
      }
    }

    if (!found) {
      kfatal("Required validation layer `%s` is missing!", *needed_layer);
      return false;
    }
  }
#endif

  VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = darray_length(required_extensions),
      .ppEnabledExtensionNames = required_extensions,
      .enabledLayerCount = required_validation_layer_names
                               ? darray_length(required_validation_layer_names)
                               : 0,
      .ppEnabledLayerNames = required_validation_layer_names,
  };

  VK_CHECK(
      vkCreateInstance(&create_info, context.allocator, &context.instance));
  kinfo("Vulkan instance created!");
  darray_destroy(required_extensions);

#if defined(_DEBUG)
  darray_destroy(available_layers);
  darray_destroy(required_validation_layer_names);

  kdebug("Creating vulkan debugger...");
  u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#if defined(LOG_WARN_ENABLED)
  log_severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
#endif
#if defined(LOG_INFO_ENABLED)
  log_severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#endif
#if defined(LOG_DEBUG_ENABLED)
  log_severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = log_severity,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = vk_debug_callback,
  };

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          context.instance, "vkCreateDebugUtilsMessengerEXT");
  kassert_msg(func, "Failed to create debug messenger");
  VK_CHECK(func(context.instance, &debug_create_info, context.allocator,
                &context.debug_messenger));
  kdebug("Vulkan debugger created!");

#endif

  kdebug("Creating Vulkan surface...");
  if (!platform_create_vulkan_surface(context.instance, context.allocator,
                                      &context.surface)) {
    kfatal("Failed to create platform surface!");
    return false;
  }
  kdebug("Vulkan surface created!");

  if (!vulkan_device_create(&context)) {
    kfatal("Failed to create device!");
    return false;
  }
  kdebug("Vulkan device created");

  vulkan_swapchain_create(&context, context.framebuffer_width,
                          context.framebuffer_height, &context.swapchain);
  kdebug("Vulkan swapchain created");

  vulkan_render_pass_create(
      &context, &context.main_render_pass, 0, 0, context.framebuffer_width,
      context.framebuffer_height, 0.0, 4.0, 4.0, 1.0, 1.0, 0);
  kdebug("Vulkan render pass created.");

  context.swapchain.framebuffers =
      darray_with_capacity(vulkan_framebuffer, context.swapchain.image_count);
  regenerate_framebuffers(&context.swapchain, &context.main_render_pass);
  kdebug("Vulkan framebuffers created.");

  create_command_buffers();
  kdebug("Command buffers created");

  initialize_sync_objects();

  kinfo("Vulkan renderer initialized :)");
  return true;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend *backend) {
  (void)backend;
  vkDeviceWaitIdle(context.device.logical_device);

  destroy_sync_objects();
  free_command_buffers();

  if (context.swapchain.framebuffers) {
    for (u32 i = 0; i < context.swapchain.image_count; i++) {
      vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }
    darray_destroy(context.swapchain.framebuffers);
  }

  if (context.main_render_pass.handle) {
    vulkan_render_pass_destroy(&context, &context.main_render_pass);
  }

  if (context.swapchain.handle) {
    vulkan_swapchain_destroy(&context, &context.swapchain);
  }

  kdebug("Destroying Vulkan device...");
  vulkan_device_destroy(&context);

  kdebug("Destroying Vulkan Surface...");
  if (context.surface) {
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    context.surface = nullptr;
  }

#if defined(_DEBUG)
  kdebug("Destroying debug messenger");
  if (context.debug_messenger) {
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            context.instance, "vkDestroyDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger,
                                    context.allocator);
  }
#endif

  kdebug("Destroying Vulkan instance...");
  vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_resized(struct renderer_backend *backend,
                                     u16 width, u16 height) {
  (void)backend;
  (void)width;
  (void)height;
}

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

#if defined(_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                  VkDebugUtilsMessageTypeFlagsEXT message_types,
                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                  void *user_data) {
  (void)user_data;

  char *message;
  bool general = message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
  bool performance =
      message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  bool validation =
      message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  if (general) {
    if (performance) {
      if (validation) {
        message = " (General; Performance; Validation) %s";
      } else {
        message = " (General; Performance) %s";
      }
    } else {
      if (validation) {
        message = " (General; Validation) %s";
      } else {
        message = " (General) %s";
      }
    }
  } else if (performance) {
    if (validation) {
      message = " (Performance; Validation) %s";
    } else {
      message = " (Performance) %s";
    }
  } else if (validation) {
    message = " (Validation) %s";
  } else {
    message = " %s";
  }

  switch (message_severity) {
  default:
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    kerror(message, callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    kwarn(message, callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    kinfo(message, callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    ktrace(message, callback_data->pMessage);
    break;
  }

  return VK_FALSE;
}
#endif

i32 find_memory_index(u32 type_filter, u32 property_flags) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(context.device.physical_device,
                                      &memory_properties);

  for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
    if (type_filter & (1 << i) &&
        (memory_properties.memoryTypes[i].propertyFlags & property_flags)) {
      return i;
    }
  }

  kwarn("Unable to find suitable memory type!");
  return -1;
}

void create_command_buffers() {
  if (!context.graphics_command_buffers) {
    context.graphics_command_buffers = darray_with_capacity(
        vulkan_command_buffer, context.swapchain.image_count);
    darray_length_set(context.graphics_command_buffers,
                      context.swapchain.image_count);
    kzero_memory(context.graphics_command_buffers,
                 sizeof(vulkan_command_buffer) * context.swapchain.image_count);
  }

  for (u32 i = 0; i < context.swapchain.image_count; i++) {
    if (context.graphics_command_buffers[i].handle) {
      vulkan_command_buffer_free(&context, context.device.graphics_command_pool,
                                 &context.graphics_command_buffers[i]);
    }
    kzero_memory(&context.graphics_command_buffers[i],
                 sizeof(vulkan_command_buffer));
    vulkan_command_buffer_allocate(&context,
                                   context.device.graphics_command_pool, true,
                                   &context.graphics_command_buffers[i]);
  }
}

void free_command_buffers() {
  vulkan_command_buffer *buffer;
  darray_for_each(context.graphics_command_buffers, buffer) {
    if (buffer && buffer->handle) {
      vulkan_command_buffer_free(&context, context.device.graphics_command_pool,
                                 buffer);
      buffer->handle = 0;
    }
  }
  darray_destroy(context.graphics_command_buffers);
}

#define ATTACHMENT_COUNT 2
void regenerate_framebuffers(vulkan_swapchain *swapchain,
                             vulkan_render_pass *render_pass) {
  for (u32 i = 0; i < swapchain->image_count; i++) {
    VkImageView attachments[ATTACHMENT_COUNT] = {
        swapchain->views[i], swapchain->depth_attachment.view};

    vulkan_framebuffer_create(&context, render_pass, context.framebuffer_width,
                              context.framebuffer_height, ATTACHMENT_COUNT,
                              attachments, &context.swapchain.framebuffers[i]);
  }
}

void initialize_sync_objects() {
  context.image_available_semaphores =
      darray_with_capacity(VkSemaphore, context.swapchain.max_frames_in_flight);
  context.queue_complete_semaphores =
      darray_with_capacity(VkSemaphore, context.swapchain.max_frames_in_flight);
  context.in_flight_fences = darray_with_capacity(
      vulkan_fence, context.swapchain.max_frames_in_flight);

  for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
        .pNext = nullptr,
    };
    vkCreateSemaphore(context.device.logical_device, &semaphore_create_info,
                      context.allocator,
                      &context.image_available_semaphores[i]);
    vkCreateSemaphore(context.device.logical_device, &semaphore_create_info,
                      context.allocator, &context.queue_complete_semaphores[i]);

    vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
  }
  darray_length_set(context.image_available_semaphores,
                    context.swapchain.max_frames_in_flight);
  darray_length_set(context.in_flight_fences,
                    context.swapchain.max_frames_in_flight);
  darray_length_set(context.queue_complete_semaphores,
                    context.swapchain.max_frames_in_flight);

  context.images_in_flight =
      darray_with_capacity(vulkan_fence *, context.swapchain.image_count);
  for (u32 i = 0; i < context.swapchain.image_count; i++) {
    context.images_in_flight[i] = 0;
  }
  darray_length_set(context.images_in_flight, context.swapchain.image_count);
}

void destroy_sync_objects() {
  darray_destroy(context.images_in_flight);
  for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
    vkDestroySemaphore(context.device.logical_device,
                       context.image_available_semaphores[i],
                       context.allocator);
    vkDestroySemaphore(context.device.logical_device,
                       context.queue_complete_semaphores[i], context.allocator);

    vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
  }
  darray_destroy(context.image_available_semaphores);
  darray_destroy(context.queue_complete_semaphores);
  darray_destroy(context.in_flight_fences);
}
