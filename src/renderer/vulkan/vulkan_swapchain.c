#include "vulkan_swapchain.h"

#include "containers/darray.h"
#include "core/asserts.h"
#include "core/kmemory.h"
#include "core/logger.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_device.h"
#include "vulkan_image.h"

void create(vulkan_context *context, u32 width, u32 height,
            vulkan_swapchain *swapchain);
void destroy(vulkan_context *context, vulkan_swapchain *swapchain);

void vulkan_swapchain_create(vulkan_context *context, u32 width, u32 height,
                             vulkan_swapchain *out_swapchain) {
  create(context, width, height, out_swapchain);
}

void vulkan_swapchain_recreate(vulkan_context *context, u32 width, u32 height,
                               vulkan_swapchain *swapchain) {
  destroy(context, swapchain);
  create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context *context,
                              vulkan_swapchain *swapchain) {
  destroy(context, swapchain);
}

bool vulkan_swapchain_acquire_next_image_index(
    vulkan_context *context, vulkan_swapchain *swapchain, u64 timeout_ns,
    VkSemaphore image_available_semaphore, VkFence fence,
    u32 *out_image_index) {
  VkResult result = vkAcquireNextImageKHR(
      context->device.logical_device, swapchain->handle, timeout_ns,
      image_available_semaphore, fence, out_image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    vulkan_swapchain_recreate(context, context->framebuffer_width,
                              context->framebuffer_height, swapchain);
    return false;
  } else if (!(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)) {
    kfatal("Failed to acquire swapchain image!");
    return false;
  }

  return true;
}

void vulkan_swapchain_present(vulkan_context *context,
                              vulkan_swapchain *swapchain,
                              VkQueue graphics_queue, VkQueue present_queue,
                              VkSemaphore render_complete_semaphore,
                              u32 present_image_index) {
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &render_complete_semaphore,
      .swapchainCount = 1,
      .pSwapchains = &swapchain->handle,
      .pImageIndices = &present_image_index,
      .pResults = nullptr,
  };
  (void)graphics_queue;
  VkResult result = vkQueuePresentKHR(present_queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    vulkan_swapchain_recreate(context, context->framebuffer_width,
                              context->framebuffer_height, swapchain);
  } else if (result != VK_SUCCESS) {
    kfatal("Failed to present swapchain image!");
  }

  context->current_frame =
      (context->current_frame + 1) % swapchain->max_frames_in_flight;
}

void create(vulkan_context *context, u32 width, u32 height,
            vulkan_swapchain *swapchain) {
  VkSurfaceCapabilitiesKHR capabilities =
      context->device.swapchain_support.capabilities;

  swapchain->max_frames_in_flight = 2;

  u32 max_image_count = capabilities.maxImageCount;
  u32 min_image_count = capabilities.minImageCount;
  u32 image_count;
  if (max_image_count == 0 || max_image_count > min_image_count) {
    image_count = min_image_count + 1;
  } else {
    image_count = min_image_count;
  }

  kassert(darray_length(context->device.swapchain_support.formats) > 0);
  swapchain->image_format = context->device.swapchain_support.formats[0];
  VkSurfaceFormatKHR *supported_surface_format;
  darray_for_each(context->device.swapchain_support.formats,
                  supported_surface_format) {
    if (supported_surface_format->format == VK_FORMAT_B8G8R8A8_SRGB &&
        supported_surface_format->colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapchain->image_format = *supported_surface_format;
      break;
    }
  }

  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  VkPresentModeKHR *supported_present_mode;

  kassert(darray_length(context->device.swapchain_support.present_modes) > 0);
  darray_for_each(context->device.swapchain_support.present_modes,
                  supported_present_mode) {
    if (*supported_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }

  vulkan_device_query_swapchain_support(context->device.physical_device,
                                        context->surface,
                                        &context->device.swapchain_support);
  VkExtent2D extent = {.width = width, .height = height};
  if (capabilities.currentExtent.width != UINT32_MAX) {
    extent = capabilities.currentExtent;
  }

  extent.width = KCLAMP(extent.width, capabilities.minImageExtent.width,
                        capabilities.maxImageExtent.width);
  extent.height = KCLAMP(extent.height, capabilities.minImageExtent.height,
                         capabilities.maxImageExtent.height);

  bool present_shares_graphics = context->device.present_queue_index ==
                                 context->device.graphics_queue_index;
  VkSharingMode sharing_mode = VK_SHARING_MODE_CONCURRENT;
  u32 queue_family_index_count = 2;
  u32 indices[2] = {context->device.graphics_queue_index,
                    context->device.present_queue_index};

  u32 *queue_family_indices = indices;
  if (present_shares_graphics) {
    sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    queue_family_index_count = 0;
    queue_family_indices = nullptr;
  }

  VkSwapchainCreateInfoKHR swapchain_create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = context->surface,
      .minImageCount = image_count,
      .imageFormat = swapchain->image_format.format,
      .imageColorSpace = swapchain->image_format.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = sharing_mode,
      .queueFamilyIndexCount = queue_family_index_count,
      .pQueueFamilyIndices = queue_family_indices,
      .preTransform =
          context->device.swapchain_support.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = present_mode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
  };

  VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device,
                                &swapchain_create_info, context->allocator,
                                &swapchain->handle));

  context->current_frame = 0;
  swapchain->image_count = 0;
  VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device,
                                   swapchain->handle, &swapchain->image_count,
                                   0));
  if (!swapchain->images) {
    swapchain->images = kallocate(sizeof(VkImage) * swapchain->image_count,
                                  MEMORY_TAG_RENDERER);
  }
  VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device,
                                   swapchain->handle, &swapchain->image_count,
                                   swapchain->images));
  if (!swapchain->views) {
    swapchain->views = kallocate(sizeof(VkImageView) * swapchain->image_count,
                                 MEMORY_TAG_RENDERER);
  }

  for (u32 i = 0; i < image_count; i++) {

    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = swapchain->images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain->image_format.format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    VK_CHECK(vkCreateImageView(context->device.logical_device,
                               &image_view_create_info, context->allocator,
                               &swapchain->views[i]));
  }

  if (!vulkan_device_detect_depth_format(&context->device)) {
    context->device.depth_format = VK_FORMAT_UNDEFINED;
    kfatal("Failed to find a supported depth buffer format!");
  }

  vulkan_image_create(context, VK_IMAGE_TYPE_2D, extent.width, extent.height,
                      context->device.depth_format, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true,
                      VK_IMAGE_ASPECT_DEPTH_BIT, &swapchain->depth_attachment);
}

void destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
  vulkan_image_destroy(context, &swapchain->depth_attachment);

  for (u32 i = 0; i < swapchain->image_count; i++) {
    vkDestroyImageView(context->device.logical_device, swapchain->views[i],
                       context->allocator);
  }
  if (swapchain->images) {
    FREE(swapchain->images);
  }
  if (swapchain->views) {
    FREE(swapchain->views);
  }
  vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle,
                        context->allocator);
}
