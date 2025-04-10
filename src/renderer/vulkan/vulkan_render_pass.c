#include "vulkan_render_pass.h"

#define ATTACHMENT_COUNT 2

void vulkan_render_pass_create(vulkan_context *context,
                               vulkan_render_pass *out_render_pass, f32 x,
                               f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a,
                               f32 depth, u32 stencil) {

  out_render_pass->x = x;
  out_render_pass->y = y;
  out_render_pass->w = w;
  out_render_pass->h = h;

  out_render_pass->r = r;
  out_render_pass->g = g;
  out_render_pass->b = b;
  out_render_pass->a = a;

  out_render_pass->state = NOT_ALLOCATED;
  out_render_pass->depth = depth;
  out_render_pass->stencil = stencil;

  VkAttachmentDescription attachment_descriptions[ATTACHMENT_COUNT] = {
      {
          .format = context->swapchain.image_format.format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          .flags = 0,
      },
      {
          .format = context->device.depth_format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      }};

  VkAttachmentReference color_attachment_reference = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkAttachmentReference depth_attachment_reference = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_reference,
      .pDepthStencilAttachment = &depth_attachment_reference,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr,
      .pResolveAttachments = 0,
  };

  VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dependencyFlags = 0,
  };

  VkRenderPassCreateInfo render_pass_create_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = ATTACHMENT_COUNT,
      .pAttachments = attachment_descriptions,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
  };

  VK_CHECK(vkCreateRenderPass(context->device.logical_device,
                              &render_pass_create_info, context->allocator,
                              &out_render_pass->handle));
}

void vulkan_render_pass_destroy(vulkan_context *context,
                                vulkan_render_pass *render_pass) {
  if (render_pass && render_pass->handle) {
    vkDestroyRenderPass(context->device.logical_device, render_pass->handle,
                        context->allocator);
  }
}

#define CLEAR_VALUE_COUNT 2

void vulkan_render_pass_begin(vulkan_command_buffer *command_buffer,
                              vulkan_render_pass *render_pass,
                              VkFramebuffer framebuffer) {
  VkClearValue clear_values[CLEAR_VALUE_COUNT] = {
      {.color = {.float32 = {render_pass->g, render_pass->b, render_pass->r,
                             render_pass->a}}},
      {.depthStencil = {.depth = render_pass->depth,
                        .stencil = render_pass->stencil}}};
  VkRenderPassBeginInfo render_pass_begin_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = render_pass->handle,
      .framebuffer = framebuffer,
      .renderArea = {.extent =
                         {
                             .width = render_pass->w,
                             .height = render_pass->h,
                         },
                     .offset =
                         {
                             .x = render_pass->x,
                             .y = render_pass->y,
                         }},
      .clearValueCount = CLEAR_VALUE_COUNT,
      .pClearValues = clear_values,
  };
  vkCmdBeginRenderPass(command_buffer->handle, &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);
  command_buffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkan_render_pass_end(vulkan_command_buffer *command_buffer,
                            vulkan_render_pass *render_pass) {
  vkCmdEndRenderPass(command_buffer->handle);
  command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
  (void)render_pass;
}
