#include "vulkan_fence.h"
#include "core/logger.h"

void vulkan_fence_create(vulkan_context *context, bool is_signaled,
                         vulkan_fence *out_vulkan_fence) {
  VkFenceCreateInfo fence_create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = is_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
  };
  VK_CHECK(vkCreateFence(context->device.logical_device, &fence_create_info,
                         context->allocator, &out_vulkan_fence->handle));
  out_vulkan_fence->is_signaled = is_signaled;
}

void vulkan_fence_destroy(vulkan_context *context, vulkan_fence *fence) {
  if (fence->handle) {
    vkDestroyFence(context->device.logical_device, fence->handle,
                   context->allocator);
    fence->handle = 0;
  }
  fence->is_signaled = false;
}

bool vulkan_fence_wait(vulkan_context *context, vulkan_fence *fence,
                       u64 timeout_ns) {
  if (!fence->is_signaled) {
    VkResult result = vkWaitForFences(context->device.logical_device, 1,
                                      &fence->handle, true, timeout_ns);
    switch (result) {
    case VK_SUCCESS:
      fence->is_signaled = true;
      return true;
    case VK_TIMEOUT:
      kwarn("vk_fence_wait - Timed out");
      return false;
    case VK_ERROR_DEVICE_LOST:
      kerror("vulkan_fence_wait - VK_ERROR_DEVICE_LOST");
      return false;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      kerror("vulkan_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
      return false;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      kerror("vulkan_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY");
      return false;
    default:
      kerror("vulkan_fence_wait failed with unknown error");
      return false;
    }
  } else {
    return true;
  }
}

void vulkan_fence_reset(vulkan_context *context, vulkan_fence *fence) {
  if (fence->is_signaled) {
    vkResetFences(context->device.logical_device, 1, &fence->handle);
    fence->is_signaled = false;
  }
}
