#include "vulkan_framebuffer.h"
#include "core/kmemory.h"

void vulkan_framebuffer_create(vulkan_context *context,
                               vulkan_render_pass *render_pass, u32 width,
                               u32 height, u32 attachment_count,
                               VkImageView *attachments,
                               vulkan_framebuffer *out_framebuffer) {
  if (attachment_count > 0) {
    out_framebuffer->attachments =
        kallocate(sizeof(VkImageView) * attachment_count, MEMORY_TAG_RENDERER);
  }
  for (u32 i = 0; i < attachment_count; i++) {
    out_framebuffer->attachments[i] = attachments[i];
  }
  out_framebuffer->attachment_count = attachment_count;
  out_framebuffer->render_pass = render_pass;
  VkFramebufferCreateInfo framebuffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = render_pass->handle,
      .attachmentCount = attachment_count,
      .pAttachments = out_framebuffer->attachments,
      .width = width,
      .height = height,
      .layers = 1,
  };
  VK_CHECK(vkCreateFramebuffer(context->device.logical_device,
                               &framebuffer_create_info, context->allocator,
                               &out_framebuffer->handle));
}

void vulkan_framebuffer_destroy(vulkan_context *context,
                                vulkan_framebuffer *framebuffer) {
  vkDestroyFramebuffer(context->device.logical_device, framebuffer->handle,
                       context->allocator);
  if (framebuffer->attachments) {
    FREE(framebuffer->attachments);
  }
  framebuffer->handle = nullptr;
  framebuffer->attachment_count = 0;
  framebuffer->render_pass = 0;
}
