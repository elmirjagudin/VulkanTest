#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#pragma once

typedef struct handles_s
{
	GLFWwindow* window;
	VkInstance instance;
	VkDebugReportCallbackEXT debug_cb;
	VkSurfaceKHR surface;
	VkQueue gfxQueue;
	VkQueue presentationQueue;
} handles_t;

