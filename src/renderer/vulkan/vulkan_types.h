#pragma once

#include "core/asserts.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                         \
  {                                                                            \
    kassert(expr == VK_SUCCESS);                                               \
  }

typedef struct vulkan_swapchain_support_info {
  VkSurfaceCapabilitiesKHR capabilities;
  /// darray
  VkSurfaceFormatKHR *formats;
  /// darray
  VkPresentModeKHR *present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_image {
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
  u32 width;
  u32 height;
} vulkan_image;

typedef struct vulkan_device {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;
  vulkan_swapchain_support_info swapchain_support;
  i32 graphics_queue_index;
  i32 present_queue_index;
  i32 transfer_queue_index;

  VkQueue graphics_queue;
  VkQueue present_queue;
  VkQueue transfer_queue;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;

  VkCommandPool graphics_command_pool;

  VkFormat depth_format;
} vulkan_device;

typedef enum vulkan_render_pass_state {
  RENDER_PASS_STATE_READY,
  RENDER_PASS_STATE_RECORDING,
  RENDER_PASS_STATE_IN_RENDER_PASS,
  RENDER_PASS_STATE_RECORDING_ENDED,
  RENDER_PASS_STATE_SUBMITTED,
  RENDER_PASS_STATE_NOT_ALLOCATED,
} vulkan_render_pass_state;

typedef struct vulkan_render_pass {
  VkRenderPass handle;
  f32 x, y, w, h;
  f32 r, g, b, a;

  f32 depth;
  u32 stencil;

  vulkan_render_pass_state state;
} vulkan_render_pass;

typedef struct vulkan_framebuffer {
  VkFramebuffer handle;
  u32 attachment_count;
  VkImageView *attachments;
  vulkan_render_pass *render_pass;
} vulkan_framebuffer;

typedef struct vulkan_swapchain {
  VkSurfaceFormatKHR image_format;
  u8 max_frames_in_flight;
  VkSwapchainKHR handle;
  u32 image_count;
  VkImage *images;
  VkImageView *views;

  vulkan_image depth_attachment;

  /// darray
  vulkan_framebuffer *framebuffers;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
  COMMAND_BUFFER_STATE_READY,
  COMMAND_BUFFER_STATE_RECORDING,
  COMMAND_BUFFER_STATE_IN_RENDER_PASS,
  COMMAND_BUFFER_STATE_RECORDING_ENDED,
  COMMAND_BUFFER_STATE_SUBMITTED,
  COMMAND_BUFFER_STATE_NOT_ALLOCATED,
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
  VkCommandBuffer handle;

  vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_fence {
  VkFence handle;
  bool is_signaled;
} vulkan_fence;

typedef struct vulkan_context {
  u32 framebuffer_width;
  u32 framebuffer_height;

  VkInstance instance;
  VkAllocationCallbacks *allocator;
  VkSurfaceKHR surface;

#if defined(_DEBUG)
  VkDebugUtilsMessengerEXT debug_messenger;
#endif

  vulkan_device device;
  vulkan_swapchain swapchain;
  vulkan_render_pass main_render_pass;

  /// darray
  vulkan_command_buffer *graphics_command_buffers;
  /// darray
  VkSemaphore *image_available_semaphores;
  /// darray
  VkSemaphore *queue_complete_semaphores;

  u32 in_flight_fence_count;
  vulkan_fence *in_flight_fences;
  vulkan_fence **images_in_flight;
  u32 image_index;
  u32 current_frame;

  bool recreating_swapchain;

  i32 (*find_memory_index)(u32 type_filter,
                           VkMemoryPropertyFlags property_flags);

} vulkan_context;
