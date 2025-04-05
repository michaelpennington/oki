#pragma once

#include <vulkan/vulkan.h>

struct platform_state;
struct vulkan_context;

bool platform_create_vulkan_surface(VkInstance instance,
                                    VkAllocationCallbacks *allocator,
                                    VkSurfaceKHR *surface);

void platform_get_required_extension_names(const char ***names);
