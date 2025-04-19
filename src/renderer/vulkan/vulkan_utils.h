#pragma once

#include "vulkan/vulkan_core.h"

const char *vulkan_result_string(VkResult result, bool get_extended);

bool vulkan_result_is_success(VkResult result);
