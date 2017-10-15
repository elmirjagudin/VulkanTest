#include <string.h>

#include "gpu_buf.h"
#include "utils.h"


static uint32_t
findMemoryType(handles_t *handles, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(handles->phyDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

static void
createBuffer(handles_t *handles, VkDeviceSize size, VkBufferUsageFlags usage,
             VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check_res(
        vkCreateBuffer(handles->device, &bufferInfo, NULL, &buffer),
        "vkCreateBuffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(handles->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(handles, memRequirements.memoryTypeBits, properties);

    check_res(
        vkAllocateMemory(handles->device, &allocInfo, nullptr, &bufferMemory),
        "vkAllocateMemory");

    vkBindBufferMemory(handles->device, buffer, bufferMemory, 0);
}

static void
copyBuffer(handles_t *handles, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = handles->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    check_res(
        vkAllocateCommandBuffers(handles->device, &allocInfo, &commandBuffer),
        "vkAllocateCommandBuffers");

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    check_res(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "vkBeginCommandBuffer");

        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    check_res(
        vkEndCommandBuffer(commandBuffer),
        "vkEndCommandBuffer");

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    check_res(
        vkQueueSubmit(handles->gfxQueue, 1, &submitInfo, VK_NULL_HANDLE),
        "vkQueueSubmit");

    check_res(
        vkQueueWaitIdle(handles->gfxQueue),
        "vkQueueWaitIdle");

    vkFreeCommandBuffers(handles->device, handles->commandPool, 1, &commandBuffer);
}

void
create_vertex_buffer(handles_t *handles, std::vector<Vertex> vertices)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(handles,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(handles->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(handles->device, stagingBufferMemory);

    createBuffer(handles,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 handles->vertexBuffer,
                 handles->vertexBufferMemory);

    copyBuffer(handles, stagingBuffer, handles->vertexBuffer, bufferSize);

    vkDestroyBuffer(handles->device, stagingBuffer, nullptr);
    vkFreeMemory(handles->device, stagingBufferMemory, nullptr);
}

void
create_index_buffer(handles_t *handles, std::vector<uint16_t> indices)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(handles,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(handles->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(handles->device, stagingBufferMemory);

    createBuffer(handles,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 handles->indexBuffer,
                 handles->indexBufferMemory);

    copyBuffer(handles, stagingBuffer, handles->indexBuffer, bufferSize);

    vkDestroyBuffer(handles->device, stagingBuffer, nullptr);
    vkFreeMemory(handles->device, stagingBufferMemory, nullptr);
}