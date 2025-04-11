#pragma once

#include "vulkan_types.h"

void vulkan_fence_create(vulkan_context *context, bool is_signaled,
                         vulkan_fence *out_vulkan_fence);

void vulkan_fence_destroy(vulkan_context *context, vulkan_fence *fence);

bool vulkan_fence_wait(vulkan_context *context, vulkan_fence *fence,
                       u64 timeout_ns);

void vulkan_fence_reset(vulkan_context *context, vulkan_fence *fence);
