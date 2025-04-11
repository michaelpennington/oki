#include "vulkan_command_buffer.h"

void vulkan_command_buffer_allocate(vulkan_context *context, VkCommandPool pool,
                                    bool is_primary,
                                    vulkan_command_buffer *out_command_buffer) {
  VkCommandBufferAllocateInfo command_buffer_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = pool,
      .level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                          : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
      .commandBufferCount = 1,
  };

  VK_CHECK(vkAllocateCommandBuffers(context->device.logical_device,
                                    &command_buffer_allocate_info,
                                    &out_command_buffer->handle));
  out_command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_free(vulkan_context *context, VkCommandPool pool,
                                vulkan_command_buffer *command_buffer) {
  vkFreeCommandBuffers(context->device.logical_device, pool, 1,
                       &command_buffer->handle);
  command_buffer->handle = nullptr;
  command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkan_command_buffer_begin(vulkan_command_buffer *command_buffer,
                                 bool is_single_use,
                                 bool is_render_pass_continue,
                                 bool is_simultaneous_use) {
  VkCommandBufferUsageFlags flags = 0;
  if (is_single_use) {
    flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  }
  if (is_render_pass_continue) {
    flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  }
  if (is_simultaneous_use) {
    flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  }
  VkCommandBufferBeginInfo command_buffer_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = flags,
      .pInheritanceInfo = nullptr,
  };
  VK_CHECK(
      vkBeginCommandBuffer(command_buffer->handle, &command_buffer_begin_info));
  command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void vulkan_command_buffer_end(vulkan_command_buffer *command_buffer) {
  VK_CHECK(vkEndCommandBuffer(command_buffer->handle));
  command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void vulkan_command_buffer_update_submitted(
    vulkan_command_buffer *command_buffer) {
  command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkan_command_buffer_reset(vulkan_command_buffer *command_buffer) {
  command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_allocate_and_begin_single_use(
    vulkan_context *context, VkCommandPool pool,
    vulkan_command_buffer *out_command_buffer) {
  vulkan_command_buffer_allocate(context, pool, true, out_command_buffer);
  vulkan_command_buffer_begin(out_command_buffer, true, false, false);
}

void vulkan_command_buffer_end_single_use(vulkan_context *context,
                                          VkCommandPool pool,
                                          vulkan_command_buffer *command_buffer,
                                          VkQueue queue) {
  vulkan_command_buffer_end(command_buffer);

  VkCommandBufferSubmitInfo buffer_submit_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .pNext = nullptr,
      .commandBuffer = command_buffer->handle,
      .deviceMask = 0,
  };
  VkSubmitInfo2 submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .pNext = nullptr,
      .flags = 0,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &buffer_submit_info,
      .waitSemaphoreInfoCount = 0,
      .pWaitSemaphoreInfos = nullptr,
      .signalSemaphoreInfoCount = 0,
      .pSignalSemaphoreInfos = nullptr,
  };
  VK_CHECK(vkQueueSubmit2(queue, 1, &submit_info, VK_NULL_HANDLE));

  VK_CHECK(vkQueueWaitIdle(queue));

  vulkan_command_buffer_free(context, pool, command_buffer);
}
