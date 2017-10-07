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

void
create_vertex_buffer(handles_t *handles, std::vector<Vertex> vertices)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check_res(
        vkCreateBuffer(handles->device, &bufferInfo, NULL, &(handles->vertexBuffer)),
        "vkCreateBuffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(handles->device, handles->vertexBuffer, &memRequirements);

    auto memTypeIdx = findMemoryType(
        handles,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeIdx;

    check_res(
        vkAllocateMemory(handles->device, &allocInfo, NULL, &(handles->vertexBufferMemory)),
        "vkAllocateMemory");

    check_res(
        vkBindBufferMemory(handles->device, handles->vertexBuffer, handles->vertexBufferMemory, 0),
        "vkBindBufferMemory");

    /* upload vertex data to GPU */
    void* data;
    check_res(
        vkMapMemory(handles->device, handles->vertexBufferMemory, 0, bufferInfo.size, 0, &data),
        "vkMapMemory");

    memcpy(data, vertices.data(), (size_t) bufferInfo.size);
    vkUnmapMemory(handles->device, handles->vertexBufferMemory);
}