#include "frame_buf.h"
#include "utils.h"

#include <stdio.h>

void
create_framebuffers(handles_t *handles)
{
    size_t image_num = handles->swapChainImageViews.size();
    handles->swapChainFramebuffers.resize(image_num);

    for (size_t i = 0; i < image_num; i++)
    {
        VkImageView attachments[] = {
            handles->swapChainImageViews[i]
        };
    
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = handles->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = handles->swapchainExtend.width;
        framebufferInfo.height = handles->swapchainExtend.height;
        framebufferInfo.layers = 1;
    
        check_res(
            vkCreateFramebuffer(handles->device, &framebufferInfo,
                 NULL, &(handles->swapChainFramebuffers[i])),
                 "vkCreateFramebuffer error");
    }
}

void
cleanup_framebuffers(handles_t *handles)
{
    for (size_t i = 0; i < handles->swapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(handles->device,
                             handles->swapChainFramebuffers[i],
                             NULL);
    }
}