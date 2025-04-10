#include "vulkan_backend.h"

#include "containers/darray.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_types.h"

#include "vulkan_device.h"
#include "vulkan_platform.h"
#include "vulkan_render_pass.h"
#include "vulkan_swapchain.h"

static vulkan_context context;

#if defined(_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);
#endif

i32 find_memory_index(u32 type_filter, u32 property_flags);

bool vulkan_renderer_backend_initialize(struct renderer_backend *backend,
                                        const char *application_name,
                                        struct platform_state *plat_state) {
  (void)backend;
  (void)plat_state;
  // TODO: custom allocator
  context.allocator = nullptr;
  context.find_memory_index = find_memory_index;

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

  kinfo("Vulkan renderer initialized :)");
  return true;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend *backend) {
  (void)backend;

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
