#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#pragma once

#define FRAME_BUF_FORMAT VK_FORMAT_B8G8R8A8_UNORM

typedef struct handles_s
{
	GLFWwindow* window;
	VkInstance instance;
	VkDebugReportCallbackEXT debug_cb;
	VkSurfaceKHR surface;
	VkQueue gfxQueue;
    VkQueue presentationQueue;
    VkPhysicalDevice phyDevice;
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtend;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkPipeline gfxPipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
} handles_t;

