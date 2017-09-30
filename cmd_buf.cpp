#include "cmd_buf.h"
#include "utils.h"

void
create_command_pool(handles_t *handles)
{
    VkCommandPoolCreateInfo poolInfo = {};

    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = handles->gfxFamilyIndex;

    check_res(
        vkCreateCommandPool(handles->device, &poolInfo, NULL, &(handles->commandPool)),
        "vkCreateCommandPool");
}

void
create_command_buffers(handles_t *handles)
{
    handles->commandBuffers.resize(handles->swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = handles->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) handles->commandBuffers.size();

    check_res(
        vkAllocateCommandBuffers(handles->device, &allocInfo, handles->commandBuffers.data()),
        "vkAllocateCommandBuffers");



    for (size_t i = 0; i < handles->commandBuffers.size(); i++)
    {
        float clr = ((float)i) * 0.5f;
        VkClearValue clearColor = {clr, 1-clr, 0.2f, 1.0f};

        /* begin command buffer recoding */
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(handles->commandBuffers[i], &beginInfo);

        /* begin render pass */
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = handles->renderPass;
        renderPassInfo.framebuffer = handles->swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = handles->swapchainExtend;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(handles->commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* bind gfx pipeline */
        vkCmdBindPipeline(handles->commandBuffers[i],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          handles->gfxPipeline);

        /* draw call */
        vkCmdDraw(handles->commandBuffers[i], 3, 1, 0, 0);

        /* end draw call */
        vkCmdEndRenderPass(handles->commandBuffers[i]);

        /* end command buffer recording */
        check_res(
            vkEndCommandBuffer(handles->commandBuffers[i]),
            "vkEndCommandBuffer");
    }

}