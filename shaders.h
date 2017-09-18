#pragma once

#include <vulkan/vulkan.h>
#include <string>

#include "main.h"

VkShaderModule
load_shader(handles_t *handles, const std::string& filename);