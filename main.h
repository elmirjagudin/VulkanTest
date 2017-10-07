#pragma once

#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <array>

#define FRAME_BUF_FORMAT VK_FORMAT_B8G8R8A8_UNORM

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

typedef struct handles_s
{
    GLFWwindow* window;
    VkInstance instance;
    VkDebugReportCallbackEXT debug_cb;
    VkSurfaceKHR surface;
    uint32_t gfxFamilyIndex;
    uint32_t presentationFamilyIndex;
    VkQueue gfxQueue;
    VkQueue presentationQueue;
    VkPhysicalDevice phyDevice;
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtend;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkPipeline gfxPipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
} handles_t;

