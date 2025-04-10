#include "vulkan_image.h"
#include "core/logger.h"

void vulkan_image_create(vulkan_context *context, VkImageType image_type,
                         u32 width, u32 height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags memory_flags, bool create_view,
                         VkImageAspectFlags view_aspect_flags,
                         vulkan_image *out_image) {
  out_image->width = width;
  out_image->height = height;

  VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = image_type,
      .format = format,
      .mipLevels = 1,
      .arrayLayers = 1,
      .usage = usage,
      .tiling = tiling,
      .extent =
          {
              .width = width,
              .height = height,
              .depth = 1,
          },
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VK_CHECK(vkCreateImage(context->device.logical_device, &image_create_info,
                         context->allocator, &out_image->handle));

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(context->device.logical_device,
                               out_image->handle, &memory_requirements);

  i32 memory_type = context->find_memory_index(
      memory_requirements.memoryTypeBits, memory_flags);
  if (memory_type == -1) {
    kerror("Required memory type not found. Image not valid");
    return;
  }

  VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type,
  };
  VK_CHECK(vkAllocateMemory(context->device.logical_device,
                            &memory_allocate_info, context->allocator,
                            &out_image->memory));

  VK_CHECK(vkBindImageMemory(context->device.logical_device, out_image->handle,
                             out_image->memory, 0));

  if (create_view) {
    out_image->view = 0;
    vulkan_image_view_create(context, format, out_image, view_aspect_flags);
  }
}

void vulkan_image_view_create(vulkan_context *context, VkFormat format,
                              vulkan_image *image,
                              VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .format = format,
      .image = image->handle,
      .components =
          {
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY,

          },
      .flags = 0,
      .subresourceRange =
          {
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
              .aspectMask = aspect_flags,
          },
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
  };

  VK_CHECK(vkCreateImageView(context->device.logical_device, &view_create_info,
                             context->allocator, &image->view));
}

void vulkan_image_destroy(vulkan_context *context, vulkan_image *image) {
  if (image->view) {
    vkDestroyImageView(context->device.logical_device, image->view,
                       context->allocator);
    image->view = 0;
  }
  if (image->memory) {
    vkFreeMemory(context->device.logical_device, image->memory,
                 context->allocator);
    image->memory = 0;
  }
  if (image->handle) {
    vkDestroyImage(context->device.logical_device, image->handle,
                   context->allocator);
    image->handle = 0;
  }
}
