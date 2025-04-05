#include "vulkan_device.h"
#include "containers/darray.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "vulkan/vulkan_core.h"

typedef struct vulkan_physical_device_requirements {
  bool graphics;
  bool present;
  bool compute;
  bool transfer;

  // darrya
  const char **device_extension_names;
  bool sampler_anisotropy;
  bool discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
  u32 graphics_family_index;
  u32 present_family_index;
  u32 compute_family_index;
  u32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

bool select_physical_device(vulkan_context *context);
bool physical_device_meets_requirements(
    VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties,
    const VkPhysicalDeviceFeatures *features,
    const vulkan_physical_device_requirements *requirements,
    vulkan_physical_device_queue_family_info *out_queue_family_info,
    vulkan_swapchain_support_info *out_swapchain_support);

bool vulkan_device_create(vulkan_context *context) {
  if (!select_physical_device(context)) {
    return false;
  }

  return true;
}

void vulkan_device_destroy(vulkan_context *context) {}

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    vulkan_swapchain_support_info *out_support_info) {
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physical_device, surface, &out_support_info->capabilities));

  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physical_device, surface, &out_support_info->format_count, nullptr));

  if (out_support_info->format_count != 0) {
    if (!out_support_info->formats) {
      out_support_info->formats =
          kallocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count,
                    MEMORY_TAG_RENDERER);
    }
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &out_support_info->format_count,
        out_support_info->formats));
  }

  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &out_support_info->present_mode_count,
      nullptr));
  if (out_support_info->present_mode_count != 0) {
    if (!out_support_info->present_modes) {
      out_support_info->present_modes = kallocate(
          sizeof(VkPresentModeKHR) * out_support_info->present_mode_count,
          MEMORY_TAG_RENDERER);
    }
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &out_support_info->present_mode_count,
        out_support_info->present_modes));
  }
}

bool select_physical_device(vulkan_context *context) {
  u32 physical_device_count = 0;
  VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count,
                                      nullptr));
  if (physical_device_count == 0) {
    kfatal("No devices which support Vulkan were found!");
    return false;
  }

  VkPhysicalDevice *physical_devices =
      darray_with_capacity(VkPhysicalDevice, physical_device_count);
  VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count,
                                      physical_devices));

  darray_length_set(physical_devices, physical_device_count);
  vulkan_physical_device_requirements requirements = {
      .graphics = true,
      .present = true,
      .transfer = true,
      .sampler_anisotropy = true,
      .discrete_gpu = true,
      .device_extension_names = darray_create(const char *),
  };
  darray_push(&requirements.device_extension_names,
              &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkPhysicalDevice *current_device;
  kinfo("Checking requiremnets");
  darray_for_each(physical_devices, current_device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(*current_device, &properties);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(*current_device, &features);

    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(*current_device, &memory);

    vulkan_physical_device_queue_family_info queue_info = {};
    bool result = physical_device_meets_requirements(
        *current_device, context->surface, &properties, &features,
        &requirements, &queue_info, &context->device.swapchain_support);

    if (result) {
      kinfo("Selected device: '%s'.", properties.deviceName);
      switch (properties.deviceType) {
      default:
      case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        kinfo("GPU type is Unknown.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        kinfo("GPU type is Integrated.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        kinfo("GPU type is Discrete.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        kinfo("GPU type is Virtual.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        kinfo("GPU type is CPU.");
        break;
      }

      kinfo("GPU Driver version: %d.%d.%d",
            VK_VERSION_MAJOR(properties.driverVersion),
            VK_VERSION_MINOR(properties.driverVersion),
            VK_VERSION_PATCH(properties.driverVersion));
      kinfo("Vulkan API version: %d.%d.%d",
            VK_VERSION_MAJOR(properties.apiVersion),
            VK_VERSION_MINOR(properties.apiVersion),
            VK_VERSION_PATCH(properties.apiVersion));

      for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
        f32 memory_size_gib =
            (((f32)memory.memoryHeaps[j].size) / 1024.0F / 1024.0F / 1024.0F);
        if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
          kinfo("Local GPU memory: %.2f GiB", memory_size_gib);
        } else {
          kinfo("Shared system memory: %.2f GiB", memory_size_gib);
        }
      }

      context->device.physical_device = *current_device;
      context->device.graphics_queue_index = queue_info.graphics_family_index;
      context->device.present_queue_index = queue_info.present_family_index;
      context->device.transfer_queue_index = queue_info.transfer_family_index;

      context->device.properties = properties;
      context->device.features = features;
      context->device.memory = memory;
      darray_destroy(physical_devices);
      darray_destroy(requirements.device_extension_names);
      return true;
    }
  }
  kerror("No physical devices found which meet the requirements");
  darray_destroy(requirements.device_extension_names);
  darray_destroy(physical_devices);
  return false;
}

bool physical_device_meets_requirements(
    VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties,
    const VkPhysicalDeviceFeatures *features,
    const vulkan_physical_device_requirements *requirements,
    vulkan_physical_device_queue_family_info *out_queue_family_info,
    vulkan_swapchain_support_info *out_swapchain_support) {
  out_queue_family_info->graphics_family_index = -1;
  out_queue_family_info->present_family_index = -1;
  out_queue_family_info->compute_family_index = -1;
  out_queue_family_info->transfer_family_index = -1;

  kinfo("Checking requiremnets");

  if (requirements->discrete_gpu) {
    if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      kinfo("Device is not a discrete GPU, skipping");
      return false;
    }
  }

  u32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           nullptr);
  VkQueueFamilyProperties queue_families[queue_family_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);

  kinfo("Graphics | Present | Compute | Transfer | Name");
  u8 min_transfer_score = 255;
  for (u32 i = 0; i < queue_family_count; ++i) {
    u8 current_transfer_score = 0;

    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      out_queue_family_info->graphics_family_index = i;
      ++current_transfer_score;
    }

    if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      out_queue_family_info->compute_family_index = i;
      ++current_transfer_score;
    }

    if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      if (current_transfer_score <= min_transfer_score) {
        min_transfer_score = current_transfer_score;
        out_queue_family_info->transfer_family_index = i;
      }
    }

    VkBool32 supports_present = VK_FALSE;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                                  &supports_present));
    if (supports_present) {
      out_queue_family_info->present_family_index = i;
    }
  }

  kinfo("      %d |      %d |      %d |      %d | %s",
        (i32)out_queue_family_info->graphics_family_index != -1,
        (i32)out_queue_family_info->present_family_index != -1,
        (i32)out_queue_family_info->compute_family_index != -1,
        (i32)out_queue_family_info->transfer_family_index != -1,
        properties->deviceName);

  if ((requirements->graphics &&
       (i32)out_queue_family_info->graphics_family_index == -1) ||
      (requirements->present &&
       (i32)out_queue_family_info->present_family_index == -1) ||
      (requirements->compute &&
       (i32)out_queue_family_info->compute_family_index == -1) ||
      (requirements->transfer &&
       (i32)out_queue_family_info->transfer_family_index == -1)) {
    return false;
  }
  kinfo("Device meets queue requirements.");
  ktrace("Graphics Family Index: %i",
         out_queue_family_info->graphics_family_index);
  ktrace("Compute Family Index: %i",
         out_queue_family_info->compute_family_index);
  ktrace("Transfer Family Index: %i",
         out_queue_family_info->transfer_family_index);
  ktrace("Present Family Index: %i",
         out_queue_family_info->present_family_index);

  vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);

  if (out_swapchain_support->format_count < 1 ||
      out_swapchain_support->present_mode_count < 1) {
    if (out_swapchain_support->formats) {
      kfree(out_swapchain_support->formats);
    }
    if (out_swapchain_support->present_modes) {
      kfree(out_swapchain_support->present_modes);
    }
    kinfo("Required swapchain support not present, skipping device");
    return false;
  }

  kfree(out_swapchain_support->formats);
  kfree(out_swapchain_support->present_modes);

  if (requirements->device_extension_names) {
    u32 available_extension_count = 0;
    VkExtensionProperties *available_extensions = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(
        device, 0, &available_extension_count, nullptr));
    if (available_extension_count != 0) {
      available_extensions = darray_with_capacity(VkExtensionProperties,
                                                  available_extension_count);
      VK_CHECK(vkEnumerateDeviceExtensionProperties(
          device, 0, &available_extension_count, available_extensions));
      darray_length_set(available_extensions, available_extension_count);

      const char **name;
      darray_for_each(requirements->device_extension_names, name) {
        bool found = false;
        VkExtensionProperties *extension;
        darray_for_each(available_extensions, extension) {
          if (strings_equal(*name, extension->extensionName)) {
            found = true;
            break;
          }
        }

        if (!found) {
          kinfo("Required extension not found: '%s', skipping device.");
          darray_destroy(available_extensions);
          return false;
        }
      }
      darray_destroy(available_extensions);
    }
  }
  if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {
    kinfo("Device does not support sampler anisotropy, skipping.");
    return false;
  }

  return true;
}
